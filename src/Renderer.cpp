#include "Renderer.hpp"
#include <GL/glew.h>

void Renderer::init()
{
    glEnable(GL_SCISSOR_TEST);
}

void Renderer::rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // Unoptimized but short way to draw rectangle.
    // It should be fast enough for this program.
    glScissor(x0, y0, x1 - x0, y1 - y0);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}
