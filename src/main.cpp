#include <iostream>
#include <chrono>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "Joysticks.hpp"
#include "Renderer.hpp"
#include "Window.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/AccurateInputLag.hpp"
#include "Scenes/PixelArt.hpp"
#include "Scenes/Scrolling.hpp"

#ifdef main
#undef main
#endif

enum SyncMode : int8_t
{
    noVSync,
    noVSyncTripleBuffer,
    adaptiveSync,
    vSync,
    vSyncWait
};

enum ScalingFilter : int8_t
{
    nearestNeighbour,
    bilinear,
    pixelAverage,
    catmullRom,
    lanczos3
};


int64_t getTimeMicroseconds()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch())
                .count();
}

void enumCombo(const char *comboName, const char (*enumNames)[32], int8_t &value, int8_t max)
{
    if(ImGui::BeginCombo(comboName, enumNames[value], 0))
    {
        for(int8_t i = 0; i <= max; i++)
            if(ImGui::Selectable(enumNames[i], i == value)) value = i;
        ImGui::EndCombo();
    }
}

int main(int argc, char **argv)
{
    static constexpr uint8_t  UPDATE_TIMING_WINDOW = 0;
    static constexpr uint8_t  MAX_UPDATE_FRAMES = 10;
    bool missedSync = false;
    bool gpuSync = true;
    int updateRate = 120;
    int simulatedUpdateTime = 0; // * 100µs
    int simulatedDrawTime = 0; // * 100µs
    int sizeX = NATIVE_RES_X, sizeY = NATIVE_RES_Y, posX, posY;
    SyncMode syncMode = SyncMode::noVSync;
    ScalingFilter scalingFilter = ScalingFilter::nearestNeighbour;

    char syncModeNames[SyncMode::vSyncWait + 1][32] =
    {
        "No v-sync",
        "No v-sync + triple buffer",
        "Adaptive sync",
        "V-sync",
        "V-sync + wait"
    };

    char scalingFilterNames[ScalingFilter::lanczos3 + 1][32] =
    {
        "Nearest neighbour",
        "Bilinear",
        "Pixel average",
        "Catmull-Rom",
        "Lanczos-3"
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

    // Init render context
    renderer.init();
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    // Init joysticks
    joysticks.init();

    // Init window and it's context
    Window window;
    window.create();
    SDL_GL_SetSwapInterval(0); // V-sync OFF

    // Scenes
    AccurateInputLag accurateInputLag;
    PixelArt pixelArt;
    Scrolling scrolling;
    renderer.useContext();
    //pixelArt.init();
    std::array<Scene*, 3> scenes {{&accurateInputLag, &pixelArt, &scrolling}};
    Scene *currentScene = scenes[0];

    // Chrono
    int64_t uSeconds = getTimeMicroseconds();
    int64_t prevUseconds = uSeconds;
    int64_t toUpdate = 0;
    uint8_t currentFrameUpdate = 0, currentFrameDraw = 0;
    std::array<int64_t, 16> frameTimes;
    std::array<int64_t, frameTimes.size()> iterationTimes;
    std::array<int64_t, 6> singleFrameTimes;
    std::array<int64_t, 6> drawTimes;
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
        snprintf(itemName, 32, "%s on display %hhd", Window::windowModeNames[window.windowMode], window.fullscreenDisplay);
        Window::WindowMode oldVideoMode = window.windowMode;
        bool recreateWindow = false;
        if(ImGui::BeginCombo("Window mode", window.windowMode == Window::WindowMode::windowed 
                ? window.windowModeNames[Window::WindowMode::windowed]
                : itemName, 0))
        {
            for(int8_t wm = Window::WindowMode::windowed; wm <= Window::WindowMode::fullscreen; wm++)
            {
                if(wm == Window::WindowMode::windowed)
                {
                    if(ImGui::Selectable(Window::windowModeNames[Window::WindowMode::windowed],
                        window.windowMode == Window::WindowMode::windowed))
                    {
                        window.windowMode = Window::WindowMode::windowed;
                        recreateWindow = true;
                    }
                }
                else for(int display = 0; display < nbDisplays; display++)
                {
                    snprintf(itemName, 32, "%s on display %hhd", window.windowModeNames[wm], display);
                    if(ImGui::Selectable(itemName, wm == window.windowMode && display == window.fullscreenDisplay))
                    {
                        window.windowMode = static_cast<Window::WindowMode>(wm);
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
                    if(window.windowMode == Window::WindowMode::fullscreen)
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
                case Window::windowed:
                    sizeX = NATIVE_RES_X;
                    sizeY = NATIVE_RES_Y;
                break;
                case Window::borderless:
                    posX = 0; posY = 0; sizeX = rect.w; sizeY = rect.h;
                break;
                case Window::fullscreen:
                    SDL_DisplayMode dm;
                    SDL_GetDisplayMode(window.fullscreenDisplay, window.displayMode, &dm);
                    posX = 0; posY = 0; sizeX = dm.w; sizeY = dm.h;
                break;
            }
            window.create();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(window.sdlWindow);
            ImGui::NewFrame();
        }

        if(window.windowMode == Window::WindowMode::windowed) SDL_GetWindowPosition(window.sdlWindow, &posX, &posY);
        ImGui::DragInt("Position X", &posX, 0.25, -16384, 16384);
        ImGui::DragInt("Position Y", &posY, 0.25, -16384, 16384);
        if(window.windowMode == Window::WindowMode::windowed) SDL_SetWindowPosition(window.sdlWindow, posX, posY);
        if(window.windowMode == Window::WindowMode::windowed) SDL_GetWindowSize(window.sdlWindow, &sizeX, &sizeY);
        ImGui::DragInt("Size X", &sizeX, 0.25, 1, NATIVE_RES_X * 10);
        ImGui::DragInt("Size Y", &sizeY, 0.25, 1, NATIVE_RES_Y * 10);
        if(window.windowMode == Window::WindowMode::windowed) SDL_SetWindowSize(window.sdlWindow, sizeX, sizeY);
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
        SyncMode oldSyncMode = syncMode;
        enumCombo("Sync mode", syncModeNames, reinterpret_cast<int8_t&>(syncMode), SyncMode::vSyncWait);
        if(syncMode != oldSyncMode)
        {
            int8_t swapInterval = 0;
            switch(syncMode)
            {
                case noVSync:
                case noVSyncTripleBuffer:
                    swapInterval = 0;
                    break;
                case adaptiveSync:
                    swapInterval = -1;
                    break;
                case vSync:
                case vSyncWait:
                    swapInterval = 1;
                    break;
                default:
                    break;
            }
            // Triple buffer only settable in X server?

            if(SDL_GL_SetSwapInterval(swapInterval) == -1)
            {
                syncMode = vSync;
                SDL_GL_SetSwapInterval(1);
            }
        }
        ImGui::Checkbox("GPU sync", &gpuSync);
        ImGui::End();

        if(syncMode == SyncMode::vSyncWait) gpuSync = true;

        // Update
        uSeconds = getTimeMicroseconds();
        toUpdate += (uSeconds - prevUseconds) * updateRate;
        prevUseconds = uSeconds;

        if(toUpdate > 1000000 * MAX_UPDATE_FRAMES) toUpdate = 1000000 * MAX_UPDATE_FRAMES;
        uint8_t nbFramesToUpdate;
        if(toUpdate > -1000000 * UPDATE_TIMING_WINDOW)
        {
            if(toUpdate > 100000 * UPDATE_TIMING_WINDOW) nbFramesToUpdate = static_cast<uint8_t>(1 + (toUpdate - 100000 * UPDATE_TIMING_WINDOW) / 1000000);
            else nbFramesToUpdate = 1;
        }
        else nbFramesToUpdate = 0;
        int64_t totalTime = nbFramesToUpdate * *std::max_element(singleFrameTimes.cbegin(), singleFrameTimes.cend())
                + *std::max_element(drawTimes.cbegin(), drawTimes.cend());
        SDL_DisplayMode displayMode;
        SDL_GetWindowDisplayMode(window.sdlWindow, &displayMode);
        int64_t displayRefreshPeriod = 1000000 / displayMode.refresh_rate;
        totalTime = displayRefreshPeriod - totalTime - 1000;
        if(syncMode == SyncMode::vSyncWait && !missedSync) while(getTimeMicroseconds() < uSeconds + totalTime);

        startTime = uSeconds = getTimeMicroseconds();
        if(toUpdate > -1000000 * UPDATE_TIMING_WINDOW) do
        {
            int64_t frameTime = getTimeMicroseconds();
            currentScene->update(frameRate);
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
        if(window.windowMode == Window::WindowMode::windowed)
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
        SDL_GL_SwapWindow(window.sdlWindow);
        if(gpuSync) glFinish();
        int64_t lastIterationTime = getTimeMicroseconds() - startTime;
        missedSync = gpuSync && lastIterationTime >= displayRefreshPeriod * 1.5;
        iterationTimes[currentFrame] = lastIterationTime;
    }
}
