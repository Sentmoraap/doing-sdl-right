#include <SDL2/SDL.h>
#include <GL/glew.h>

class DisplayWindow
{
public:

    enum WindowMode : int8_t
    {
        windowed,
        borderless,
        fullscreen
    };

    enum SyncMode : int8_t
    {
        noVSync,
        adaptiveSync,
        vSync
    };

    static const char windowModeNames[WindowMode::fullscreen + 1][18];

    WindowMode windowMode = WindowMode::windowed;
    SDL_Window *sdlWindow;
    SDL_GLContext context;
    GLuint program, vbo, vao;
    int fullscreenDisplay = 0, displayMode = 0;
    bool canVSync, canNoVSync, canAdaptiveSync, tripleBuffer;

    void create();
    void useContext();
    bool isSyncModeAvailable(SyncMode syncMode);
    SyncMode getSyncMode();
    void setSyncMode(SyncMode syncMode);
    void swap();
    void destroy();

private:
    SyncMode syncMode = SyncMode::noVSync;
};
