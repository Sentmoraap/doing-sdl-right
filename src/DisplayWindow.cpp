#include <iostream>
#include <algorithm>
#include <SDL2/SDL_syswm.h>
#include "DisplayWindow.hpp"
#include "Renderer.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

const char DisplayWindow::windowModeNames[WindowMode::fullscreen + 1][18] =
{
    "Windowed",
    "Borderless",
    "Fullscreen"
};

const char DisplayWindow::syncModeNames[DisplayWindow::SyncMode::vSync + 1][40] =
{
    "Off",
    "Adaptive",
    "On",
};

const char DisplayWindow::scalingFilterNames[DisplayWindow::ScalingFilter::lanczos3 + 1][40] =
{
    "Bilinear",
    "Pixel average",
    "Bicubic",
    "Lanczos-3"
};

void DisplayWindow::create()
{
    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
    int sizeX, sizeY, posX, posY;

    SDL_Rect rect;
    SDL_GetDisplayBounds(fullscreenDisplay, &rect);
    SDL_DisplayMode dm;
    SDL_GetDisplayMode(fullscreenDisplay, displayMode, &dm);

    switch(windowMode)
    {
        case WindowMode::windowed:
            flags |= SDL_WINDOW_RESIZABLE;
            posX = SDL_WINDOWPOS_UNDEFINED;
            posY = SDL_WINDOWPOS_UNDEFINED;
            sizeX = NATIVE_RES_X;
            sizeY = NATIVE_RES_Y;
            break;

        case WindowMode::borderless:
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
            posX = rect.x;
            posY = rect.y;
            sizeX = rect.w;
            sizeY = rect.h;
            break;

        case WindowMode::fullscreen:
            flags |= SDL_WINDOW_FULLSCREEN;
            posX = rect.x;
            posY = rect.y;
            sizeX = dm.w;
            sizeY = dm.h;
            break;
    }
    sdlWindow = SDL_CreateWindow("SDL test", posX, posY, sizeX, sizeY, flags);
    if(windowMode == fullscreen) SDL_SetWindowDisplayMode(sdlWindow, &dm);
    context = SDL_GL_CreateContext(sdlWindow);
    SDL_GL_MakeCurrent(sdlWindow, context);

    // Init rendering
    glEnable(GL_FRAMEBUFFER_SRGB);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    {
        float data[16] =
        {
            -1, -1, 0, 1,
            -1,  1, 0, 0,
            1,  1, 1, 0,
            1, -1, 1, 1
        };
        glBufferData(GL_ARRAY_BUFFER, 16 * 4, data, GL_STATIC_DRAW);
    }
    auto loadProgram = [this](ProgramIds &programIds, const char *vert, const char *frag)
    {
        programIds.program = Renderer::loadShaders(vert, frag);
        glUseProgram(programIds.program);
        glUniform1i(glGetUniformLocation(programIds.program, "tex"), 0);
        glGenVertexArrays(1, &programIds.vao);
        glBindVertexArray(programIds.vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        {
            GLuint attrib = glGetAttribLocation(programIds.program, "pos");
            glVertexAttribPointer(attrib, 2, GL_FLOAT, false, 16, (void*)0);
            glEnableVertexAttribArray(attrib);
            attrib = glGetAttribLocation(programIds.program, "textureCoord");
            glVertexAttribPointer(attrib, 2, GL_FLOAT, false, 16, (void*)8);
            glEnableVertexAttribArray(attrib);
            glBindFragDataLocation(programIds.program, 0, "fragColor");
        }
    };
    loadProgram(bilinearProgram, "assets/basic_texture.vert", "assets/tunable_bilinear.frag");
    glUniform2i(glGetUniformLocation(bilinearProgram.program, "sourceSize"), NATIVE_RES_X, NATIVE_RES_Y);
    blurynessUniform = glGetUniformLocation(bilinearProgram.program, "bluryness");
    loadProgram(pixelAverageProgram, "assets/basic_texture.vert", "assets/pixel_coverage.frag");
    glUniform2i(glGetUniformLocation(pixelAverageProgram.program, "sourceSize"), NATIVE_RES_X, NATIVE_RES_Y);
    nbIterationsUniform = glGetUniformLocation(pixelAverageProgram.program, "nbIterations");
    windowSizeUniform = glGetUniformLocation(pixelAverageProgram.program, "destSize");
    coverageMultUniform = glGetUniformLocation(pixelAverageProgram.program, "coverageMult");
    loadProgram(bicubicProgram, "assets/basic_texture.vert", "assets/bicubic.frag");
    glUniform2i(glGetUniformLocation(bicubicProgram.program, "sourceSize"), NATIVE_RES_X, NATIVE_RES_Y);
    bcUniform = glGetUniformLocation(bicubicProgram.program, "bc");
    loadProgram(lanczos3Program, "assets/basic_texture.vert", "assets/lanczos3.frag");
    glUniform2i(glGetUniformLocation(lanczos3Program.program, "sourceSize"), NATIVE_RES_X, NATIVE_RES_Y);
    setScalingFilter(bilinear);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    {
        int32_t err=glGetError();
        if(err)
            std::cerr << "Error window render " << gluErrorString(err) << std::endl;
    }

    // Get avaiable sync modes
    if(SDL_GL_SetSwapInterval(-1) == 0) canAdaptiveSync = true;
    if(SDL_GL_GetSwapInterval() != -1) canAdaptiveSync = false;
    if(SDL_GL_SetSwapInterval(0) == 0) canNoVSync = true;
    if(SDL_GL_GetSwapInterval() != 0) canNoVSync = false;
    if(SDL_GL_SetSwapInterval(1) == 0) canVSync = true;
    if(SDL_GL_GetSwapInterval() != 1) canVSync = false;

    // Detect triple buffer
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(sdlWindow);
    glClearColor(0.1f, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(sdlWindow);
    glClearColor(0, 0.1f, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(sdlWindow);
    glClearColor(0, 0, 0.1f, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(sdlWindow);
    uint8_t col[3];
    glReadPixels(0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, col);
    tripleBuffer = col[1] == 0;
    if(!isSyncModeAvailable(syncMode))
    {
        int i = 0;
        while(!isSyncModeAvailable(static_cast<SyncMode>(i))) i++;
        syncMode = static_cast<SyncMode>(i);
    }
    setSyncMode(syncMode);

    // Init ImGui
    ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDL2_InitForOpenGL(sdlWindow, context);
    ImGui_ImplOpenGL3_Init("#version 150");
    ImGui::StyleColorsDark();
}

void DisplayWindow::useContext()
{
    SDL_GL_MakeCurrent(sdlWindow, context);
}

bool DisplayWindow::isSyncModeAvailable(SyncMode syncMode)
{
    switch(syncMode)
    {
        case noVSync:
            return canNoVSync;
        case adaptiveSync:
            return canAdaptiveSync;
        case vSync:
            return canVSync;
        default:
            return false;
    }
}

DisplayWindow::SyncMode DisplayWindow::getSyncMode() const
{
    return syncMode;
}

void DisplayWindow::setSyncMode(SyncMode syncMode)
{
    this->syncMode = syncMode;
    int8_t swapInterval = 0;
    switch(syncMode)
    {
        case noVSync:
            swapInterval = 0;
            break;
        case adaptiveSync:
            swapInterval = -1;
            break;
        case vSync:
            swapInterval = 1;
            break;
        default:
            break;
    }
    SDL_GL_SetSwapInterval(swapInterval);
}

DisplayWindow::ScalingFilter DisplayWindow::getScalingFilter() const
{
    return scalingFilter;
}

void DisplayWindow::setScalingFilter(ScalingFilter filter)
{
    switch(filter)
    {
        case pixelAverage:
        case bicubic:
        case lanczos3:
            glBindTexture(GL_TEXTURE_2D, renderer.texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);
            break;
        case bilinear:
            glBindTexture(GL_TEXTURE_2D, renderer.texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
            break;
    }
    scalingFilter = filter;
}

void DisplayWindow::draw()
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer.texture);
    switch(scalingFilter)
    {
        case bilinear:
            glUseProgram(bilinearProgram.program);
            glBindVertexArray(bilinearProgram.vao);
            glUniform1f(blurynessUniform, 1.f - sharpness * 0.01f);
            break;
        case pixelAverage:
        {
            int sizeX, sizeY;
            glUseProgram(pixelAverageProgram.program);
            glBindVertexArray(pixelAverageProgram.vao);
            SDL_GetWindowSize(sdlWindow, &sizeX, &sizeY);
            glUniform2i(windowSizeUniform, sizeX, sizeY);
            float mult = sharpness == 100 ? 1000000 : 0.5f / (1 - sharpness * 0.01f);
            glUniform1f(coverageMultUniform, mult);
            glUniform1i(nbIterationsUniform, std::max({2,
                    2 + NATIVE_RES_X / static_cast<int>(sizeX * mult),
                    2 + NATIVE_RES_Y / static_cast<int>(sizeY * mult)}));
            break;
        }
        case bicubic:
            glUseProgram(bicubicProgram.program);
            glBindVertexArray(bicubicProgram.vao);
            glUniform2f(bcUniform, 1.f - sharpness * 0.01f, sharpness * 0.005f);
            break;
        case lanczos3:
            glUseProgram(lanczos3Program.program);
            glBindVertexArray(lanczos3Program.vao);
            break;
    }
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    GLuint err = glGetError();
    if(err)
        std::cerr << "Error window render " << gluErrorString(err) << std::endl;
}

void DisplayWindow::swap()
{
    //std::cout << SDL_GL_GetSwapInterval() << std::endl;
    SDL_GL_SwapWindow(sdlWindow);
}

void DisplayWindow::destroy()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyWindow(sdlWindow);
}
