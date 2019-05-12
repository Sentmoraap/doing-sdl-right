#include <chrono>
#include <cstdint>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

int64_t getTimeMicroseconds()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count();
}

int main(int argc, char **argv)
{
    static constexpr uint16_t NATIVE_RES_X=1024;
    static constexpr uint16_t NATIVE_RES_Y=768;
    uint16_t frameRate = 120;

    // Init SDL
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);
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
    SDL_GL_SetSwapInterval(0); // V-sync OFF

    glewInit();

    // Init ImGui
    ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDL2_InitForOpenGL(window, windowContext);
    ImGui_ImplOpenGL3_Init("#version 150");
    ImGui::StyleColorsDark();

    int64_t uSeconds = getTimeMicroseconds();
    int64_t prevUseconds = uSeconds;
    int64_t toUpdate = 0;

    while(true)
    {
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
        ImGui::Text("Settings and scene selection here");
        ImGui::End();

        // Update
        uSeconds = getTimeMicroseconds();
        toUpdate += (uSeconds - prevUseconds) * frameRate;
        // Update loop here

        // Draw
        ImGui::Render();
        SDL_GL_MakeCurrent(window, windowContext);
        glViewport(0, 0, NATIVE_RES_X, NATIVE_RES_Y);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
}
