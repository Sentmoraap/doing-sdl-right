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

    enum ScalingFilter : int8_t
    {
        nearestNeighbour,
        bilinear,
        pixelAverage,
        catmullRom,
        lanczos3
    };

    static const char windowModeNames[WindowMode::fullscreen + 1][18];
    static const char syncModeNames[DisplayWindow::SyncMode::vSync + 1][40];
    static const char scalingFilterNames[DisplayWindow::ScalingFilter::lanczos3 + 1][40];

    WindowMode windowMode = WindowMode::windowed;
    SDL_Window *sdlWindow;
    int fullscreenDisplay = 0, displayMode = 0;
    bool tripleBuffer;

private:
    SyncMode syncMode = SyncMode::noVSync;
    DisplayWindow::ScalingFilter scalingFilter = DisplayWindow::ScalingFilter::nearestNeighbour;
    SDL_GLContext context;
    GLuint program, vbo, vao;
    bool canVSync, canNoVSync, canAdaptiveSync;

public:
    void create();
    void useContext();
    bool isSyncModeAvailable(SyncMode syncMode);
    SyncMode getSyncMode() const;
    void setSyncMode(SyncMode syncMode);
    ScalingFilter getScalingFilter() const;
    void setScalingFilter(ScalingFilter filter);
    void draw();
    void swap();
    void destroy();

};
