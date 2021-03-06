#include <iostream>
#include <chrono>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <thread>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "Inputs.hpp"
#include "Renderer.hpp"
#include "DisplayWindow.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/AccurateInputLag.hpp"
#include "Scenes/GhettoInputLag.hpp"
#include "Scenes/PixelArt.hpp"
#include "Scenes/Scrolling.hpp"

#ifdef main
#undef main
#endif

enum InputLagMitigation : int8_t
{
    none,
    gpuSync,
    frameDelay
};

enum Timestep : int8_t
{
    fixed,
    interpolation,
    loose,
    looseInterpolation
};

int64_t getTimeMicroseconds()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch())
                .count();
}

void enumCombo(const char *comboName, const char (*enumNames)[40], int8_t &value, int8_t max)
{
    if(ImGui::BeginCombo(comboName, enumNames[value], 0))
    {
        for(int8_t i = 0; i <= max; i++)
            if(ImGui::Selectable(enumNames[i], i == value)) value = i;
        ImGui::EndCombo();
    }
}

void computeWindowSize(int &posX, int &posY, int &sizeX, int &sizeY)
{
    if(NATIVE_RES_Y * sizeX > NATIVE_RES_X * sizeY)
    {
        int oldSizeX = sizeX;
        sizeX = sizeY * NATIVE_RES_X / NATIVE_RES_Y;
        posX = (oldSizeX - sizeX) / 2;
    }
    else
    {
        int oldSizeY = sizeY;
        sizeY = sizeX * NATIVE_RES_Y / NATIVE_RES_X;
        posY = (oldSizeY - sizeY) / 2;
    }
}

void gpuHardSync()
{
    uint8_t col[3];
    glReadPixels(0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, col);
}

int main(int argc, char **argv)
{
    static constexpr uint8_t MAX_UPDATE_FRAMES_DIV = 10;
    static constexpr int AUTO_FRAME_DELAY_MARGIN = 1000;
    static constexpr int SLEEP_MARGIN = 2000;
    bool missedSync = true;
    int updateRate = 120;
    int simulatedUpdateTime = 0; // * 100µs
    int randomUpdateTime = 0;
    int simulatedDrawTime = 0; // Arbitrary units
    int randomDrawTime = 0;
    int sizeX = NATIVE_RES_X, sizeY = NATIVE_RES_Y, posX, posY;
    InputLagMitigation inputLagMitigation = InputLagMitigation::none;
    Timestep timestep = Timestep::fixed;
#ifdef _WINDOWS
    //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetProcessDPIAware();
#endif

    char inputLagMitigationNames[InputLagMitigation::frameDelay + 1][40] =
    {
        "None",
        "GPU hard sync",
        "Predictive waiting"
    };

    char timestepNames[Timestep::looseInterpolation + 1][40] =
    {
        "Fixed",
        "Interpolation",
        "Loose",
        "Loose + interpolation"
    };

    // Init SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_RELEASE_BEHAVIOR, 0);
    //SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);

    // Init render context
    renderer.init();
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    // Init Inputs
    Inputs inputs;
    bool useSavedInputs = false;
    Inputs::State savedInputs;
    inputs.init();

    // Init window and it's context
    DisplayWindow window;
    DisplayWindow::SyncMode nextSyncMode = DisplayWindow::noVSync;
    window.create();

    // Scenes
    AccurateInputLag accurateInputLag;
    GhettoInputLag ghettoInputLag;
    PixelArt pixelArt;
    Scrolling scrolling;
    renderer.useContext();
    pixelArt.init();
    std::array<Scene*, 4> scenes {{&accurateInputLag, &ghettoInputLag, &pixelArt, &scrolling}};
    Scene *currentScene = scenes[0];

    // Chrono
    int64_t uSeconds = getTimeMicroseconds();
    int64_t prevUseconds = uSeconds;
    int64_t toUpdate = 0;
    int64_t addToUpdate = 0;
    uint8_t currentFrameUpdate = 0, currentFrameDraw = 0;
    std::array<int64_t, 16> frameTimes;
    std::array<int64_t, frameTimes.size()> iterationTimes;
    std::array<int64_t, frameTimes.size()> remainTimes;
    std::array<int64_t, 6> singleFrameTimes;
    std::array<int64_t, singleFrameTimes.size()> drawTimes;
    for(unsigned int i = 0; i < singleFrameTimes.size(); i++)
    {
        singleFrameTimes[i] = 1000000;
        drawTimes[i] = 1000000;
    }
    for(int64_t &time : remainTimes) time = 0;
    uint8_t currentFrame = 0;

    // Text
    char text[32] = { 0 };
    SDL_StartTextInput();

    // Auto test
    int8_t testNumber = -1;
    uint16_t testRates[] = {24, 30, 48, 60, 72, 120, 144, 300};
    uint16_t testDrawTimes[] = {150, 350, 500, 0};

    std::ofstream testOutput;

    // Main loop
    while(true)
    {
        // Measure frame rate
        uint16_t frameRate;
        int64_t startTime = getTimeMicroseconds();
        {
            int64_t prevTime = frameTimes[currentFrame];
            frameTimes[currentFrame] = startTime;
            ++currentFrame %= frameTimes.size();
            frameRate = static_cast<int16_t>(frameTimes.size() * 1000000 / (startTime - prevTime));
        }
        uint32_t iterationTime = 0;
        for(int64_t frameIterationTime : iterationTimes) iterationTime += static_cast<uint32_t>(frameIterationTime);
        iterationTime /= iterationTimes.size();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch(event.type)
            {
                case SDL_QUIT:
                    return 0;
                case SDL_WINDOWEVENT:
                    if(event.window.event == SDL_WINDOWEVENT_CLOSE
                        && event.window.windowID == SDL_GetWindowID(window.sdlWindow))
                        return 0;
                    break;
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_ESCAPE) return 0;
                    if(event.key.keysym.sym == SDLK_RETURN) text[0] = 0;
                    break;
                case SDL_TEXTINPUT:
                    strncat(text, event.text.text, sizeof(text) - strlen(text) - 1);
                    break;
            }
        }

        // In case of the game update rate is a multiple or a divider of the monitor refresh rate,
        // sync them to have constent measurements.
        bool resync = false;
        
        // Auto test
        Inputs::State prevTestInputs;
        {
            Inputs::State inputsState = inputs.getState();
            if(inputsState.test && !prevTestInputs.test && testNumber < 0)
            {
                currentScene = &accurateInputLag;
                updateRate = testRates[0];
                simulatedDrawTime = testDrawTimes[0];
                randomDrawTime = simulatedDrawTime / 5;
                timestep = fixed;
                nextSyncMode = DisplayWindow::noVSync;
                resync = true;
                inputLagMitigation = none;
                testNumber = 0;
                testOutput.open("out.csv", std::ofstream::out | std::ofstream::app);
            }
            if(testNumber >= 0 && inputsState.reset && !prevTestInputs.reset)
            {
                resync = true;
                DisplayWindow::SyncMode curMode = window.getSyncMode();
                testOutput << simulatedDrawTime << "," << updateRate << "," << curMode << "," << inputLagMitigation << ","
                        << timestep << "," << text << "," << accurateInputLag.getInputLag() << std::endl;
                switch(inputLagMitigation)
                {
                    case none:
                        inputLagMitigation = gpuSync;
                        break;
                    case gpuSync:
                        if(curMode == DisplayWindow::noVSync)
                        {
                            nextSyncMode = DisplayWindow::vSync;
                            inputLagMitigation = none;
                        }
                        else inputLagMitigation = frameDelay;
                        break;
                    case frameDelay:
                        nextSyncMode = DisplayWindow::noVSync;
                        inputLagMitigation = none;
                        testNumber++;
                        uint8_t nbRates = sizeof(testRates) / sizeof(testRates[0]);
                        uint8_t nbTimes = sizeof(testDrawTimes) / sizeof(testDrawTimes[0]);
                        if(testNumber >= nbRates * nbTimes)
                        {
                            testNumber = -1;
                            testOutput.close();
                        }
                        else
                        {
                            updateRate = testRates[testNumber % nbRates];
                            simulatedDrawTime = testDrawTimes[testNumber / nbRates];
                            randomDrawTime = simulatedDrawTime / 5;
                        }
                }

            }
            prevTestInputs = inputsState;
        }

        // ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window.sdlWindow);
        ImGui::NewFrame();
        ImGui::Begin("Stuff");
        ImGui::Text("%6d FPS", frameRate);
        ImGui::Text("%6d µs", iterationTime);
        //if(missedSync) ImGui::Text("VBL missed");
        ImGui::Separator();
        ImGui::Text("Scene");
        if(ImGui::BeginCombo("", currentScene->getName(), 0))
        {
            for(Scene *scene : scenes)
            {
                if(ImGui::Selectable(scene->getName(), scene == currentScene))
                {
                    currentScene = scene;
                }
            }
            ImGui::EndCombo();
        }
        currentScene->displayImGuiSettings();
        ImGui::Separator();
        ImGui::Text((std::string("Keyboard input: ") + text).c_str());
        ImGui::Separator();
        ImGui::Text("Game loop");
        ImGui::DragInt("Update rate (Hz)", &updateRate, 0.25, 1, 300);
        enumCombo("Timestep", timestepNames, reinterpret_cast<int8_t&>(timestep), Timestep::looseInterpolation);
        ImGui::DragInt("Update time *100 µs", &simulatedUpdateTime, 0.25, 0, 1000);
        ImGui::DragInt("Random update time *100 µs", &randomUpdateTime, 0.25, 0, 1000);
        ImGui::DragInt("Draw time (arbitrary units)", &simulatedDrawTime, 0.25, 0, 1000);
        ImGui::DragInt("Random draw time", &randomDrawTime, 0.25, 0, 1000);
        ImGui::Separator();
        ImGui::Text("Settings");
        int nbDisplays = SDL_GetNumVideoDisplays();
        if(window.fullscreenDisplay >= nbDisplays) window.fullscreenDisplay = 0;
        char itemName[32];
        snprintf(itemName, 32, "%s on display %hhd", DisplayWindow::windowModeNames[window.windowMode],
                window.fullscreenDisplay);
        bool recreateWindow = false;
        if(ImGui::BeginCombo("Window mode", window.windowMode == DisplayWindow::WindowMode::windowed
                ? window.windowModeNames[DisplayWindow::WindowMode::windowed]
                : itemName, 0))
        {
            for(int8_t wm = DisplayWindow::WindowMode::windowed; wm <= DisplayWindow::WindowMode::fullscreen; wm++)
            {
                if(wm == DisplayWindow::WindowMode::windowed)
                {
                    if(ImGui::Selectable(DisplayWindow::windowModeNames[DisplayWindow::WindowMode::windowed],
                        window.windowMode == DisplayWindow::WindowMode::windowed))
                    {
                        window.windowMode = DisplayWindow::WindowMode::windowed;
                        recreateWindow = true;
                    }
                }
                else for(int display = 0; display < nbDisplays; display++)
                {
                    snprintf(itemName, 32, "%s on display %hhd", window.windowModeNames[wm], display);
                    if(ImGui::Selectable(itemName, wm == window.windowMode && display == window.fullscreenDisplay))
                    {
                        window.windowMode = static_cast<DisplayWindow::WindowMode>(wm);
                        window.fullscreenDisplay = display;
                        recreateWindow = true;
                    }
                }
            }
            ImGui::EndCombo();
        }

        SDL_DisplayMode dm;
        SDL_GetDisplayMode(window.fullscreenDisplay, window.displayMode, &dm);
        snprintf(itemName, 32, "%dx%d@%d", dm.w, dm.h, dm.refresh_rate);
        if(ImGui::BeginCombo("Display mode", itemName, 0))
        {
            int nbDisplayModes = SDL_GetNumDisplayModes(window.fullscreenDisplay);
            for(int i = 0; i < nbDisplayModes; i++)
            {
                SDL_GetDisplayMode(window.fullscreenDisplay, i, &dm);
                snprintf(itemName, 32, "%dx%d@%d", dm.w, dm.h, dm.refresh_rate);
                if(ImGui::Selectable(itemName, i == window.displayMode))
                {
                    window.displayMode = i;
                    if(window.windowMode == DisplayWindow::WindowMode::fullscreen)
                        recreateWindow = true;
                }
            }
            ImGui::EndCombo();
        }

        if(recreateWindow)
        {
            resync = true;
            window.destroy();
            renderer.useContext();
            SDL_Rect rect;
            SDL_GetDisplayBounds(window.fullscreenDisplay, &rect);
            switch(window.windowMode)
            {
                case DisplayWindow::windowed:
                    sizeX = NATIVE_RES_X;
                    sizeY = NATIVE_RES_Y;
                break;
                case DisplayWindow::borderless:
                    posX = 0; posY = 0; sizeX = rect.w; sizeY = rect.h;
                    computeWindowSize(posX, posY, sizeX, sizeY);
                break;
                case DisplayWindow::fullscreen:
                    SDL_DisplayMode dm;
                    SDL_GetDisplayMode(window.fullscreenDisplay, window.displayMode, &dm);
                    posX = 0; posY = 0; sizeX = dm.w; sizeY = dm.h;
                    computeWindowSize(posX, posY, sizeX, sizeY);
                break;
            }
            window.create();
            SDL_SetWindowInputFocus(window.sdlWindow);
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(window.sdlWindow);
            ImGui::NewFrame();
            ImGui::Begin("Stuff");
        }

        if(window.windowMode == DisplayWindow::WindowMode::windowed)
                SDL_GetWindowPosition(window.sdlWindow, &posX, &posY);
        ImGui::DragInt("Position X", &posX, 0.25, -16384, 16384);
        ImGui::DragInt("Position Y", &posY, 0.25, -16384, 16384);
        if(window.windowMode == DisplayWindow::WindowMode::windowed)
        {
            SDL_SetWindowPosition(window.sdlWindow, posX, posY);
            SDL_GetWindowSize(window.sdlWindow, &sizeX, &sizeY);
        }
        ImGui::DragInt("Size X", &sizeX, 0.25, 1, NATIVE_RES_X * 10);
        ImGui::DragInt("Size Y", &sizeY, 0.25, 1, NATIVE_RES_Y * 10);
        if(window.windowMode == DisplayWindow::WindowMode::windowed) SDL_SetWindowSize(window.sdlWindow, sizeX, sizeY);
        DisplayWindow::ScalingFilter oldScalingFilter = window.getScalingFilter(), newScalingFilter = oldScalingFilter;

        enumCombo("Scaling filter", DisplayWindow::scalingFilterNames, reinterpret_cast<int8_t&>(newScalingFilter),
            DisplayWindow::ScalingFilter::lanczos3);
        if(oldScalingFilter != newScalingFilter) window.setScalingFilter(newScalingFilter);
        ImGui::DragInt("Sharpness", &window.sharpness, 0.25, 0, 100);


        if(window.tripleBuffer) ImGui::Text("Triple buffer detected. This program may not behave as intended.");
        DisplayWindow::SyncMode syncMode = window.getSyncMode();
        if(ImGui::BeginCombo("V-Sync", DisplayWindow::syncModeNames[syncMode], 0))
        {
            for(int i = 0; i <= DisplayWindow::SyncMode::vSync; i++)
                if(window.isSyncModeAvailable(static_cast<DisplayWindow::SyncMode>(i))
                    && ImGui::Selectable(DisplayWindow::syncModeNames[i], i == syncMode))
            {
                nextSyncMode = static_cast<DisplayWindow::SyncMode>(i);
                window.setSyncMode(DisplayWindow::vSync);
                resync = true;
            }
            ImGui::EndCombo();
        }
        enumCombo("Input lag mitigation", inputLagMitigationNames, reinterpret_cast<int8_t&>(inputLagMitigation),
                syncMode == DisplayWindow::SyncMode::noVSync ? InputLagMitigation::gpuSync
                                                             : InputLagMitigation::frameDelay);
        ImGui::End();

        syncMode = window.getSyncMode();
        if(syncMode == DisplayWindow::SyncMode::noVSync && inputLagMitigation == InputLagMitigation::frameDelay)
            inputLagMitigation = InputLagMitigation::gpuSync;

        // Update
        uSeconds = startTime;// getTimeMicroseconds();
        int64_t dToUpdate = (uSeconds - prevUseconds) * updateRate;
        toUpdate += dToUpdate;
        prevUseconds = uSeconds;

        uint8_t maxUpdateFrames = (updateRate / MAX_UPDATE_FRAMES_DIV) + 2;
        if(toUpdate > 1000000 * maxUpdateFrames) toUpdate = 1000000 * maxUpdateFrames;
        uint8_t nbFramesToUpdate = 0;
        if(window.getSyncMode() == DisplayWindow::noVSync && timestep == fixed)
        {
            while(toUpdate <= 1000000)
            {
                int64_t sleepTime = (1000000 - toUpdate) / updateRate - SLEEP_MARGIN;
                if(sleepTime > 0) std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
                uSeconds = getTimeMicroseconds();
                dToUpdate = (uSeconds - prevUseconds) * updateRate;
                toUpdate += dToUpdate;
                prevUseconds = uSeconds;
                if(inputLagMitigation == InputLagMitigation::gpuSync) gpuHardSync();
            }
        }

        SDL_DisplayMode displayMode;
        SDL_GetWindowDisplayMode(window.sdlWindow, &displayMode);
        int64_t displayRefreshPeriod = 1000000 / displayMode.refresh_rate;
        int64_t waitTime = *std::min_element(remainTimes.cbegin(), remainTimes.cend()) - AUTO_FRAME_DELAY_MARGIN;
        if(!missedSync && inputLagMitigation == InputLagMitigation::frameDelay && waitTime > 0)
        {
            while(true)
            {
                int64_t micros = getTimeMicroseconds();
                if(micros < uSeconds + waitTime)
                {
                    int64_t sleepTime = uSeconds + waitTime - micros - SLEEP_MARGIN;              
                    if(sleepTime > 0) std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
                }
                else break;
            }
        }
        else waitTime = 0;

        int64_t updateStartTime = uSeconds = getTimeMicroseconds();
        switch(timestep)
        {
            case Timestep::fixed:
                while(toUpdate > 1000000)
                {
                    int64_t frameTime = getTimeMicroseconds();
                    currentScene->update(1000000 / updateRate, inputs.getState());
                    frameTime = getTimeMicroseconds() - frameTime;
                    if(frameTime < simulatedUpdateTime * 100) frameTime = simulatedUpdateTime * 100;
                    singleFrameTimes[currentFrameUpdate] = frameTime;
                    ++currentFrameUpdate %= singleFrameTimes.size();
                    toUpdate -= 1000000;
                    nbFramesToUpdate++;
                }
                currentScene->saveState();
                break;
            case Timestep::interpolation:
                currentScene->loadState();
                while(toUpdate > 1000000)
                {
                    int64_t frameTime = getTimeMicroseconds();
                    currentScene->update(1000000 / updateRate, useSavedInputs ? savedInputs : inputs.getState());
                    useSavedInputs = false;
                    frameTime = getTimeMicroseconds() - frameTime;
                    if(frameTime < simulatedUpdateTime * 100) frameTime = simulatedUpdateTime * 100;
                    singleFrameTimes[currentFrameUpdate] = frameTime;
                    ++currentFrameUpdate %= singleFrameTimes.size();
                    toUpdate -= 1000000;
                    nbFramesToUpdate++;
                }
                currentScene->saveState();
                if(toUpdate > 0)
                {
                    if(!useSavedInputs)
                    {
                        savedInputs = inputs.getState();
                        useSavedInputs = true;
                    }
                    currentScene->update(toUpdate / updateRate, savedInputs);
                    nbFramesToUpdate++;
                }
                break;
            case Timestep::loose:
                currentScene->loadState();
                while(toUpdate > 1000000)
                {
                    int64_t frameTime = getTimeMicroseconds();
                    currentScene->update((1000000 + addToUpdate) / updateRate, useSavedInputs ? savedInputs : inputs.getState());
                    useSavedInputs = false;
                    addToUpdate = 0;
                    frameTime = getTimeMicroseconds() - frameTime;
                    if(frameTime < simulatedUpdateTime * 100) frameTime = simulatedUpdateTime * 100;
                    singleFrameTimes[currentFrameUpdate] = frameTime;
                    ++currentFrameUpdate %= singleFrameTimes.size();
                    toUpdate -= 1000000;
                    nbFramesToUpdate++;
                }
                currentScene->saveState();
                if(toUpdate > 0 && addToUpdate < 0 && toUpdate + dToUpdate >= 1000000)
                {
                    currentScene->update((toUpdate + addToUpdate) / updateRate, useSavedInputs ? savedInputs : inputs.getState());
                    useSavedInputs = false;
                    addToUpdate = 1000000 - (toUpdate + addToUpdate);
                    toUpdate -= 1000000;
                    nbFramesToUpdate++;
                    currentScene->saveState();
                }
                break;
            case Timestep::looseInterpolation:
                currentScene->loadState();
                while(toUpdate > 1000000)
                {
                    int64_t frameTime = getTimeMicroseconds();
                    currentScene->update((1000000 + addToUpdate) / updateRate, useSavedInputs ? savedInputs : inputs.getState());
                    useSavedInputs = false;
                    addToUpdate = 0;
                    frameTime = getTimeMicroseconds() - frameTime;
                    if(frameTime < simulatedUpdateTime * 100) frameTime = simulatedUpdateTime * 100;
                    singleFrameTimes[currentFrameUpdate] = frameTime;
                    ++currentFrameUpdate %= singleFrameTimes.size();
                    toUpdate -= 1000000;
                    nbFramesToUpdate++;
                }
                currentScene->saveState();
                if(addToUpdate > 0 || toUpdate + dToUpdate < 1000000)
                {
                    if(!useSavedInputs)
                    {
                        savedInputs = inputs.getState();
                        useSavedInputs = true;
                    }
                    currentScene->update((toUpdate + addToUpdate) / updateRate, savedInputs);
                    nbFramesToUpdate++;
                }
                else if(toUpdate > 0)
                {
                    currentScene->update(toUpdate / updateRate, useSavedInputs ? savedInputs : inputs.getState());
                    useSavedInputs = false;
                    addToUpdate = 1000000 - toUpdate;
                    toUpdate -= 1000000;
                    nbFramesToUpdate++;
                    currentScene->saveState();
                }
                break;
        }
        {
            int64_t endTime = uSeconds + nbFramesToUpdate
                    * (simulatedUpdateTime + (randomUpdateTime ? rand() % randomUpdateTime : 0)) * 100;
            while(getTimeMicroseconds() < endTime);
        }

        // Draw
        int64_t startDrawTime = getTimeMicroseconds();
        GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        renderer.beginDrawFrame(sync);
        renderer.longDraw(simulatedDrawTime + (randomDrawTime ? rand() % randomDrawTime : 0));
        currentScene->draw();
        renderer.endDrawFrame();
        sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

        int32_t err=glGetError();
        if(err)
            std::cerr << "Error frame render " << gluErrorString(err) << std::endl;

        window.useContext();
        glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
        if(testNumber < 0) ImGui::Render();

        if(window.windowMode == DisplayWindow::WindowMode::windowed)
        {
            glViewport(0, 0, sizeX, sizeY);
            glScissor(0, 0, sizeX, sizeY);
        }
        else
        {
            int wY;
            SDL_GetWindowSize(window.sdlWindow, nullptr, &wY);
            glViewport(posX, -posY + wY - sizeY, sizeX, sizeY);
            glScissor(posX, -posY + wY - sizeY, sizeX, sizeY);
        }
        window.draw();

        if(testNumber < 0) ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        int64_t drawTime = getTimeMicroseconds() - startDrawTime;
        //if(drawTime < simulatedDrawTime * 100) drawTime = simulatedDrawTime * 100;
        drawTimes[currentFrameDraw] = drawTime;
        ++currentFrameDraw %= drawTimes.size();
        if(inputLagMitigation >= InputLagMitigation::frameDelay) gpuHardSync();
        int64_t beforeSwapTime = getTimeMicroseconds();
        window.swap();
        if(resync || inputLagMitigation >= InputLagMitigation::gpuSync) gpuHardSync();
        if(resync)
        {
            toUpdate = 0;
            window.setSyncMode(nextSyncMode);
        }
        int64_t afterSwapTime = getTimeMicroseconds();
        if(inputLagMitigation >= InputLagMitigation::gpuSync) switch(window.getSyncMode())
        {
            case DisplayWindow::SyncMode::noVSync:
                missedSync = false;
                break;
            case DisplayWindow::SyncMode::adaptiveSync:
            case DisplayWindow::SyncMode::vSync:
                int64_t remainingTime = displayRefreshPeriod - (beforeSwapTime - startTime) + waitTime;
                remainTimes[currentFrame] = remainingTime;
                missedSync = (afterSwapTime - startTime) >= displayRefreshPeriod * 1.1;
                break;
        }
        else missedSync = false;
        iterationTimes[currentFrame] = afterSwapTime - updateStartTime;
    }
}
