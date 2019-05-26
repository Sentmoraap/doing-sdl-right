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
#include "Scenes/Scene.hpp"
#include "Scenes/AccurateInputLag.hpp"
#include "Scenes/PixelArt.hpp"
#include "Scenes/Scrolling.hpp"

#ifdef main
#undef main
#endif

enum SyncMode
{
    noVSync,
    noVSyncTripleBuffer,
    adaptiveSync,
    vSync,
    vSyncWait,
    num
};

int64_t getTimeMicroseconds()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch())
                .count();
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
    SyncMode syncMode = SyncMode::noVSync;

    char syncModeNames[SyncMode::num][32] =
    {
        "No v-sync",
        "No v-sync + triple buffer",
        "Adaptive sync",
        "V-sync",
        "V-sync + wait"
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

    // Init render context
    renderer.init();
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    // Init joysticks
    joysticks.init();

    // Init window and it's context
    SDL_Window* window = SDL_CreateWindow("SDL test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            NATIVE_RES_X, NATIVE_RES_Y, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext windowContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(0); // V-sync OFF

    // Init window rendering
    GLuint program = Renderer::loadShaders("assets/basic_texture.vert", "assets/basic_texture.frag");
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "tex"), 0);
    GLuint vbo, vao;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    {
        GLuint attrib = glGetAttribLocation(program ,"pos");
        glVertexAttribPointer(attrib, 2, GL_FLOAT, false, 16, (void*)0);
        glEnableVertexAttribArray(attrib);
        attrib = glGetAttribLocation(program ,"textureCoord");
        glVertexAttribPointer(attrib, 2, GL_FLOAT, false, 16, (void*)8);
        glEnableVertexAttribArray(attrib);
        glBindFragDataLocation(program, 0, "fragPass");
        float data[16] =
        {
            -1, -1, 0, 1,
            -1,  1, 0, 0,
             1,  1, 1, 0,
             1, -1, 1, 1
        };
        glBufferData(GL_ARRAY_BUFFER, 16 * 4, data, GL_STATIC_DRAW);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    {
        int32_t err=glGetError();
        if(err)
            std::cerr << "Error window render " << gluErrorString(err) << std::endl;
    }

    // Init ImGui
    ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDL2_InitForOpenGL(window, windowContext);
    ImGui_ImplOpenGL3_Init("#version 150");
    ImGui::StyleColorsDark();

    // Scenes
    AccurateInputLag accurateInputLag;
    PixelArt pixelArt;
    Scrolling scrolling;
    renderer.useContext();
    pixelArt.init();
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
                && event.window.windowID == SDL_GetWindowID(window))
                return 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) return 0;
        }

        // ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
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
        ImGui::Text("Settings");
        ImGui::DragInt("Update rate (Hz)", &updateRate, 0.25, 1, 300);
        ImGui::DragInt("Update time *100 µs", &simulatedUpdateTime, 0.25, 0, 1000);
        ImGui::DragInt("Draw time *100 µs", &simulatedDrawTime, 0.25, 0, 1000);
        SyncMode oldSyncMode = syncMode;
        if(ImGui::BeginCombo("Sync mode", syncModeNames[syncMode], 0))
        {
            for(int8_t i = 0; i < SyncMode::num; i++)
                if(ImGui::Selectable(syncModeNames[i], i == syncMode)) syncMode = static_cast<SyncMode>(i);
            ImGui::EndCombo();
        }
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
            // Tiple buffer only settable in X server?

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
        SDL_GetWindowDisplayMode(window, &displayMode);
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

        SDL_GL_MakeCurrent(window, windowContext);
        ImGui::Render();
        glViewport(0, 0, NATIVE_RES_X, NATIVE_RES_Y);
        glScissor(0, 0, NATIVE_RES_X, NATIVE_RES_Y);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderer.texture);
        glUseProgram(program);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
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
        SDL_GL_SwapWindow(window);
        if(gpuSync) glFinish();
        int64_t lastIterationTime = getTimeMicroseconds() - startTime;
        missedSync = gpuSync && lastIterationTime >= displayRefreshPeriod * 1.5;
        iterationTimes[currentFrame] = lastIterationTime;
    }
}
