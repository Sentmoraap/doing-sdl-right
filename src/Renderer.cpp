#include "Renderer.hpp"
#include "SDL2/SDL_image.h"
#include <iostream>
#include <chrono>

Renderer renderer;

void Renderer::init()
{
    window = SDL_CreateWindow("Render context window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
       0, 0, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_SKIP_TASKBAR);
    context = SDL_GL_CreateContext(window);
    glewExperimental = GL_TRUE;
    glewInit();
    int32_t err=glGetError();
    //SDL_DestroyWindow(window);
    glEnable(GL_SCISSOR_TEST);

    // FBO
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, NATIVE_RES_X, NATIVE_RES_Y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Textures drawing
    textureProgram = Renderer::loadShaders("assets/basic_texture.vert", "assets/basic_texture_rgba.frag");
    glUseProgram(textureProgram);
    glUniform1i(glGetUniformLocation(textureProgram, "tex"), 0);
    glGenBuffers(1, &textureVbo);
    glGenVertexArrays(1, &textureVao);
    glBindVertexArray(textureVao);
    glBindBuffer(GL_ARRAY_BUFFER, textureVbo);
    {
        GLuint attrib = glGetAttribLocation(textureProgram ,"pos");
        glVertexAttribPointer(attrib, 2, GL_FLOAT, false, 16, (void*)0);
        glEnableVertexAttribArray(attrib);
        attrib = glGetAttribLocation(textureProgram ,"textureCoord");
        glVertexAttribPointer(attrib, 2, GL_FLOAT, false, 16, (void*)8);
        glEnableVertexAttribArray(attrib);
        glBindFragDataLocation(textureProgram, 0, "fragPass");
    }

    // Long drawing
    longProgram = Renderer::loadShaders("assets/long_rendering.vert", "assets/long_rendering.frag");
    glGenBuffers(1, &longVbo);
    glGenVertexArrays(1, &longVao);
    glBindVertexArray(longVao);
    glBindBuffer(GL_ARRAY_BUFFER, longVbo);
    {
        GLuint attrib = glGetAttribLocation(textureProgram, "pos");
        glVertexAttribPointer(attrib, 2, GL_FLOAT, false, 8, (void*)0);
        glEnableVertexAttribArray(attrib);
        float data[8] =
        {
            0, 0,
            0, 1,
            1, 1,
            1, 0
        };
        glBufferData(GL_ARRAY_BUFFER, 8 * 4, data, GL_STATIC_DRAW);
        glBindFragDataLocation(textureProgram, 0, "fragColor");

    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    longInstanceTime = 1;
    longDraw(100);
    {
        uint8_t col[3];
        glReadPixels(0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, col);
        int64_t startTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
        longDraw(1000000);
        glReadPixels(0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, col);
        int64_t endTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
        longInstanceTime = (endTime - startTime) / 1000;
    }
    err=glGetError();
    if(err)
        std::cerr << "Error init renderer" << gluErrorString(err) << std::endl;
}

void Renderer::useContext()
{
    SDL_GL_MakeCurrent(window, context);
}

void Renderer::beginDrawFrame(GLsync sync)
{
    SDL_GL_MakeCurrent(window, context);
    glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, NATIVE_RES_X, NATIVE_RES_Y);
    glScissor(0, 0, NATIVE_RES_X, NATIVE_RES_Y);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::endDrawFrame()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Renderer::loadTexture(const char* path)
{
    SDL_Surface *surface = IMG_Load(path);
    GLuint ret;
    glGenTextures(1, &ret);
    glBindTexture(GL_TEXTURE_2D, ret);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(surface);
    return ret;
}

void Renderer::rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    // Unoptimized but short way to draw rectangle.
    // It should be fast enough for this program.
    glScissor(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::textureRect(GLuint texture, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    glScissor(0, 0, NATIVE_RES_X, NATIVE_RES_Y);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUseProgram(textureProgram);
    glBindVertexArray(textureVao);
    glBindBuffer(GL_ARRAY_BUFFER, textureVbo);
    float fx0 = (static_cast<float>(x0) / (NATIVE_RES_X - 1)) * 2 - 1;
    float fy0 = (static_cast<float>(y0) / (NATIVE_RES_Y - 1)) * 2 - 1;
    float fx1 = (static_cast<float>(x1) / (NATIVE_RES_X - 1)) * 2 - 1;
    float fy1 = (static_cast<float>(y1) / (NATIVE_RES_Y - 1)) * 2 - 1;
    float data[16] =
    {
        fx0, fy0, 0, 0,
        fx0, fy1, 0, 1,
        fx1, fy1, 1, 1,
        fx1, fy0, 1, 0
    };
    glBufferData(GL_ARRAY_BUFFER, 16 * 4, data, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    int32_t err=glGetError();
    if(err)
        std::cerr << "Error texture rect " << gluErrorString(err) << std::endl;
}

void Renderer::longDraw(uint64_t microseconds)
{
    glScissor(0, 0, NATIVE_RES_X, NATIVE_RES_Y);
    glUseProgram(longProgram);
    glBindVertexArray(longVao);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<GLsizei>(microseconds / longInstanceTime));
    glBindVertexArray(0);
}

GLuint Renderer::loadShaders(const char* vert, const char* frag)
{
    GLuint vertexShader=glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader=glCreateShader(GL_FRAGMENT_SHADER);
    GLuint program=glCreateProgram();
    char buffer[16384];
    const char* buf=buffer;
    GLint length;
    FILE* f;

    // Load and compile the vertex shader
    f = fopen(vert, "rt");
    length=fread(buffer,1,16384,f);
    fclose(f);
    glShaderSource(vertexShader,1,&buf,&length);
    glCompileShader(vertexShader);
    glGetShaderInfoLog(vertexShader,16384,&length,buffer);
    if(length) std::cout << "Vertex shader: " << buffer << std::endl;

    // Load and compile the fragment shader
    f = fopen(frag, "rt");
    length=fread(buffer,1,16384,f);
    fclose(f);
    glShaderSource(fragmentShader,1,&buf,&length);
    glCompileShader(fragmentShader);
    glGetShaderInfoLog(fragmentShader,16384,&length,buffer);
    if(length) std::cout << "Fragment shader: " << buffer << std::endl;

    // Link shaders
    glAttachShader(program,vertexShader);
    glAttachShader(program,fragmentShader);
    glLinkProgram(program);
    glGetProgramInfoLog(program,16384,&length,buffer);
    if(length) std::cout << "Link: " << buffer << std::endl;

    return program;
}
