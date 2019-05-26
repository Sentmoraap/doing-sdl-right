#pragma once
#include <cstdint>
#include <SDL2/SDL.h>
#include <GL/glew.h>

static constexpr uint16_t NATIVE_RES_X = 1024;
static constexpr uint16_t NATIVE_RES_Y = 768;

class Renderer
{
    private:
        GLuint fbo;
        SDL_GLContext context;
        SDL_Window *window;
        GLuint textureProgram, textureVbo, textureVao;

    public:
        GLuint texture;

        void init();
        void useContext();
        void beginDrawFrame();
        void endDrawFrame();
        GLuint loadTexture(const char* path);
        void rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
        void textureRect(GLuint texture, int16_t x0, int16_t y0, int16_t x1, int16_t y1);
        static GLuint loadShaders(const char* vert, const char* frag);
};

extern Renderer renderer;
