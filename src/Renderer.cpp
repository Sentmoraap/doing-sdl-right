#include "Renderer.hpp"
#include <iostream>

Renderer renderer;

void Renderer::init()
{
    window = SDL_CreateWindow("Render context window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            0, 0, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_SKIP_TASKBAR);
    context = SDL_GL_CreateContext(window);
    glewInit();
    //SDL_DestroyWindow(window);
    glEnable(GL_SCISSOR_TEST);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, NATIVE_RES_X, NATIVE_RES_Y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int32_t err=glGetError();
    if(err)
        std::cerr << "Error init renderer" << std::string((const char*)gluErrorString(err)) << std::endl;
}

void Renderer::beginDrawFrame()
{
    SDL_GL_MakeCurrent(window, context);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glScissor(0, 0, NATIVE_RES_X, NATIVE_RES_Y);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::endDrawFrame()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // Unoptimized but short way to draw rectangle.
    // It should be fast enough for this program.
    glScissor(x0, y0, x1 - x0, y1 - y0);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
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
    f=fopen(vert,"rt");
    length=fread(buffer,1,16384,f);
    fclose(f);
    glShaderSource(vertexShader,1,&buf,&length);
    glCompileShader(vertexShader);
    glGetShaderInfoLog(vertexShader,16384,&length,buffer);
    if(length) std::cout << "Vertex shader: " << buffer << std::endl;

    // Load and compile the fragment shader
    f=fopen(frag,"rt");
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
