#include <unistd.h>
#include <chrono>
#include <vector>
#include <array>
#include <cstdint>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "Renderer.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/AccurateInputLag.hpp"

int64_t getTimeMicroseconds()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count();
}

int main(int argc, char **argv)
{
    static constexpr uint16_t NATIVE_RES_X = 1024;
    static constexpr uint16_t NATIVE_RES_Y = 768;
    static constexpr uint8_t  UPDATE_TIMING_WINDOW = 1;
    static constexpr uint8_t  MAX_UPDATE_FRAMES = 10;
    int updateRate = 120;
    int simulatedUpdateTime = 0; // * 100µs
    int simulatedDrawTime = 0; // * 100µs

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
    SDL_Window* window = SDL_CreateWindow("SDL test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            NATIVE_RES_X, NATIVE_RES_Y, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext windowContext = SDL_GL_CreateContext(window);
    Renderer::init();
    SDL_GL_SetSwapInterval(0); // V-sync OFF

    glewInit();

    // Init ImGui
    ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDL2_InitForOpenGL(window, windowContext);
    ImGui_ImplOpenGL3_Init("#version 150");
    ImGui::StyleColorsDark();

    // Scenes
    Scene dummy0;
    AccurateInputLag accurateInputLag;
    std::array<Scene*, 2> scenes {{&dummy0, &accurateInputLag}};
    Scene *currentScene = scenes[1];

    // Chrono
    int64_t uSeconds = getTimeMicroseconds();
    int64_t prevUseconds = uSeconds;
    int64_t toUpdate = 0;
    std::array<int64_t, 16> frameTimes;
    std::array<int64_t, frameTimes.size()> iterationTimes;
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
            frameRate = frameTimes.size() * 1000000 / (startTime - prevTime);
        }
        uint32_t iterationTime = 0;
        for(int64_t frameIterationTime : iterationTimes) iterationTime += frameIterationTime;
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
        ImGui::DragInt("Update rate (Hz)", &updateRate, 1, 1, 300);
        ImGui::DragInt("Update time *100 µs", &simulatedUpdateTime, 1, 0, 1000);
        ImGui::DragInt("Draw time *100 µs", &simulatedDrawTime, 1, 0, 1000);
        ImGui::End();

        // Update
        uSeconds = getTimeMicroseconds();
        toUpdate += (uSeconds - prevUseconds) * updateRate;
        prevUseconds = uSeconds;

        if(toUpdate > 1000000 * MAX_UPDATE_FRAMES) toUpdate = 1000000 * MAX_UPDATE_FRAMES;
        uint8_t nbFramesUpdated = 0;
        if(toUpdate > -1000000 * UPDATE_TIMING_WINDOW) do
        {
            currentScene->update(frameRate);
            toUpdate -= 1000000;
            nbFramesUpdated++;
        }
        while(toUpdate > 1000000 * UPDATE_TIMING_WINDOW);
        while(getTimeMicroseconds() < uSeconds + nbFramesUpdated * simulatedUpdateTime * 100);

        // Draw
        int64_t startDrawTime = getTimeMicroseconds();
        ImGui::Render();
        SDL_GL_MakeCurrent(window, windowContext);
        glViewport(0, 0, NATIVE_RES_X, NATIVE_RES_Y);
        glScissor(0, 0, NATIVE_RES_X, NATIVE_RES_Y);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        currentScene->draw();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
        while(getTimeMicroseconds() < startDrawTime + simulatedDrawTime * 100);
        iterationTimes[currentFrame] = getTimeMicroseconds() - startTime;
    }
}
