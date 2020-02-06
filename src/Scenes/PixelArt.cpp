#include "Scenes/PixelArt.hpp"
#include "Renderer.hpp"
#include <cstring>

PixelArt::PixelArt()
{
    strcpy(name, "Pixel art");
}

void PixelArt::init()
{
    texture = renderer.loadTexture("assets/map.png");
}

void PixelArt::draw()
{
    renderer.textureRect(texture, 0, 0, NATIVE_RES_X - 1, NATIVE_RES_Y - 1);
}
