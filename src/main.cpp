#include <iostream>
#include <chrono>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "Joysticks.hpp"
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

enum ScalingFilter : int8_t
{
    nearestNeighbour,
    bilinear,
    pixelAverage,
    catmullRom,
    lanczos3
};

enum InputLagMitigation : int8_t
{
    none,
    gpuSync,
    frameDelay
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
    static constexpr uint8_t  UPDATE_TIMING_WINDOW = 0;
    static constexpr uint8_t  MAX_UPDATE_FRAMES = 10;
    static constexpr int AUTO_FRAME_DELAY_MARGIN = 1000;
    bool missedSync = true;
    float frameDuration;
    int updateRate = 120;
    int simulatedUpdateTime = 0; // * 100µs
    int simulatedDrawTime = 0; // * 100µs
    int waitTime = 0;
    int sizeX = NATIVE_RES_X, sizeY = NATIVE_RES_Y, posX, posY;
    ScalingFilter scalingFilter = ScalingFilter::nearestNeighbour;
    InputLagMitigation inputLagMitigation = InputLagMitigation::none;

#ifdef _WINDOWS
    //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetProcessDPIAware();
#endif

    char syncModeNames[DisplayWindow::SyncMode::vSync + 1][40] =
    {
        "Off",
        "Adaptive",
        "On",
    };

    char scalingFilterNames[ScalingFilter::lanczos3 + 1][40] =
    {
        "Nearest neighbour",
        "Bilinear",
        "Pixel average",
        "Catmull-Rom",
        "Lanczos-3"
    };

    char inputLagMitigationNames[InputLagMitigation::frameDelay + 1][40] =
    {
        "None",
        "GPU hard sync",
        "GPU hard sync + predictive waiting"
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
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_RELEASE_BEHAVIOR, 0);

    // Init render context
    renderer.init();
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    // Init joysticks
    joysticks.init();

    // Init window and it's context
    DisplayWindow window;
    window.create();

    // Scenes
    AccurateInputLag accurateInputLag;
    GhettoInputLag ghettoInputLag;
    PixelArt pixelArt;
    Scrolling scrolling;
    renderer.useContext();
    #ifndef _WINDOWS
        pixelArt.init();
    #endif
    std::array<Scene*, 4> scenes {{&accurateInputLag, &ghettoInputLag, &pixelArt, &scrolling}};
    Scene *currentScene = scenes[0];

    // Chrono
    int64_t uSeconds = getTimeMicroseconds();
    int64_t prevUseconds = uSeconds;
    int64_t toUpdate = 0;
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
    uint8_t currentFrame = 0;

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
            //ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                return 0;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE
                && event.window.windowID == SDL_GetWindowID(window.sdlWindow))
                return 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) return 0;
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
        ImGui::Text("Simulated times");
        ImGui::DragInt("Update rate (Hz)", &updateRate, 0.25, 1, 300);
        ImGui::DragInt("Update time *100 µs", &simulatedUpdateTime, 0.25, 0, 1000);
        ImGui::DragInt("Draw time *100 µs", &simulatedDrawTime, 0.25, 0, 1000);
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
        ScalingFilter oldScalingFilter = scalingFilter;
        enumCombo("Scaling filter", scalingFilterNames, reinterpret_cast<int8_t&>(scalingFilter),
            ScalingFilter::lanczos3);
        if(oldScalingFilter != scalingFilter)
        {
            switch(scalingFilter)
            {
                case nearestNeighbour:
                    glBindTexture(GL_TEXTURE_2D, renderer.texture);
                    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    break;
                case bilinear:
                    glBindTexture(GL_TEXTURE_2D, renderer.texture);
                    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    break;
                default:
                    // Not implemented
                    break;
            }
        }
        if(window.tripleBuffer) ImGui::Text("Triple buffer detected. This program may not behave as intended.");
        DisplayWindow::SyncMode syncMode = window.getSyncMode();
        if(ImGui::BeginCombo("V-Sync", syncModeNames[syncMode], 0))
        {
            for(int i = 0; i <= DisplayWindow::SyncMode::vSync; i++)
                    if(window.isSyncModeAvailable(static_cast<DisplayWindow::SyncMode>(i))
                            && ImGui::Selectable(syncModeNames[i], i == syncMode))
                                    window.setSyncMode(static_cast<DisplayWindow::SyncMode>(i));
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
        toUpdate += (uSeconds - prevUseconds) * updateRate;
        prevUseconds = uSeconds;

        if(toUpdate > 1000000 * MAX_UPDATE_FRAMES) toUpdate = 1000000 * MAX_UPDATE_FRAMES;
        uint8_t nbFramesToUpdate;
        if(toUpdate > -1000000 * UPDATE_TIMING_WINDOW)
        {
            if(toUpdate > 100000 * UPDATE_TIMING_WINDOW)
                    nbFramesToUpdate = static_cast<uint8_t>(1 + (toUpdate - 100000 * UPDATE_TIMING_WINDOW) / 1000000);
            else nbFramesToUpdate = 1;
        }
        else nbFramesToUpdate = 0;
        
        SDL_DisplayMode displayMode;
        SDL_GetWindowDisplayMode(window.sdlWindow, &displayMode);
        int64_t displayRefreshPeriod = 1000000 / displayMode.refresh_rate;
        waitTime = *std::min_element(remainTimes.cbegin(), remainTimes.cend()) - AUTO_FRAME_DELAY_MARGIN;
        if(!missedSync && inputLagMitigation == InputLagMitigation::frameDelay && waitTime > 0)
                        while(getTimeMicroseconds() < uSeconds + waitTime);

        int64_t updateStartTime = uSeconds = getTimeMicroseconds();
        if(toUpdate > -1000000 * UPDATE_TIMING_WINDOW) do
        {
            int64_t frameTime = getTimeMicroseconds();
            SDL_PumpEvents();
            currentScene->update(updateRate);
            frameTime = getTimeMicroseconds() - frameTime;
            if(frameTime < simulatedUpdateTime * 100) frameTime = simulatedUpdateTime * 100;
            singleFrameTimes[currentFrameUpdate] = frameTime;
            ++currentFrameUpdate %= singleFrameTimes.size();
            toUpdate -= 1000000;
        }
        while(toUpdate > 1000000 * UPDATE_TIMING_WINDOW);
        while(getTimeMicroseconds() < uSeconds + nbFramesToUpdate * simulatedUpdateTime * 100);

        // Draw
        int64_t startDrawTime = getTimeMicroseconds();
        renderer.beginDrawFrame();
        currentScene->draw();
        renderer.endDrawFrame();
        int32_t err=glGetError();
        if(err)
            std::cerr << "Error frame render " << gluErrorString(err) << std::endl;

        window.useContext();
        ImGui::Render();
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
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderer.texture);
        glUseProgram(window.program);
        glBindVertexArray(window.vao);
        glBindBuffer(GL_ARRAY_BUFFER, window.vbo);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        err=glGetError();
        if(err)
            std::cerr << "Error window render " << gluErrorString(err) << std::endl;

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        int64_t drawTime = getTimeMicroseconds() - startDrawTime;
        if(drawTime < simulatedDrawTime * 100) drawTime = simulatedDrawTime * 100;
        drawTimes[currentFrameDraw] = drawTime;
        ++currentFrameDraw %= drawTimes.size();
        while(getTimeMicroseconds() < startDrawTime + simulatedDrawTime * 100);
        if(inputLagMitigation >= InputLagMitigation::gpuSync) gpuHardSync();
        int64_t beforeSwapTime = getTimeMicroseconds();
        window.swap();
        if(inputLagMitigation >= InputLagMitigation::gpuSync) gpuHardSync();
        int64_t afterSwapTime = getTimeMicroseconds();
        if(inputLagMitigation >= InputLagMitigation::gpuSync) switch(window.getSyncMode())
        {
            case DisplayWindow::SyncMode::noVSync:
                missedSync = false;
                break;
            case DisplayWindow::SyncMode::adaptiveSync:
            case DisplayWindow::SyncMode::vSync:
                int64_t remainingTime = displayRefreshPeriod - (beforeSwapTime - startTime);
                remainTimes[currentFrame] = remainingTime;
                missedSync = (afterSwapTime - startTime) >= displayRefreshPeriod * 1.1;
                break;
        }
        else missedSync = false;
        frameDuration = static_cast<float>(afterSwapTime - startTime) / displayRefreshPeriod;
        iterationTimes[currentFrame] = afterSwapTime - updateStartTime;
    }
}
