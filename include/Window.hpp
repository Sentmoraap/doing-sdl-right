#include <SDL2/SDL.h>
#include <GL/glew.h>

class Window
{
public:

    enum WindowMode : int8_t
    {
        windowed,
        borderless,
        fullscreen
    };

    static const char windowModeNames[WindowMode::fullscreen + 1][18];

    WindowMode windowMode = WindowMode::windowed;
    SDL_Window *sdlWindow;
    SDL_GLContext context;
    GLuint program, vbo, vao;
    int fullscreenDisplay = 0, displayMode = 0;

    void create();
    void useContext();
    //void initImGui();
    void destroy();
};