#include <cstring>
#include <tuple>
#include "imgui/imgui.h"
#include "Scenes/Scrolling.hpp"
#include "Renderer.hpp"
#include "Joysticks.hpp"

Scrolling::Scrolling()
{
    strcpy(name, "Scrolling");
}

void Scrolling::update(uint16_t frameRate)
{
    int x, y;
    std::tie(x, y) = joysticks.getXY();
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    if(keys[SDL_SCANCODE_LEFT])  x -= 32767;
    if(keys[SDL_SCANCODE_RIGHT]) x += 32767;
    if(keys[SDL_SCANCODE_UP])    y += 32767;
    if(keys[SDL_SCANCODE_DOWN])  y -= 32767;
    if(x < -32767) x = -32767; else if(x > 32767) x = 32767;
    if(y < -32767) y = -32767; else if(y > 32767) y = 32767;
    scrollX += x * scrollSpeed / 32767;
    scrollY += y * scrollSpeed / 32767;
}

void Scrolling::displayImGuiSettings()
{
    ImGui::DragInt("Scroll speed", &scrollSpeed, 0.25, 1, 64);
}

void Scrolling::draw()
{
    int sizeX = NATIVE_RES_X + 3 * SQUARES_SIZE - 1;
    sizeX -= (sizeX % SQUARES_SIZE);
    int sizeY = NATIVE_RES_Y + 3 * SQUARES_SIZE - 1;
    sizeY -= (sizeY % SQUARES_SIZE);
    int nbX = sizeX / SQUARES_SIZE, nbY = sizeY / SQUARES_SIZE;
    for(int y = 0; y < nbY; y++) for(int x = y % 2; x < nbX; x += 2)
    {
        int posX = x * SQUARES_SIZE - scrollX;
        posX %= sizeX;
        if(posX < 0) posX += sizeX;
        posX -= SQUARES_SIZE;
        int posY = y * SQUARES_SIZE - scrollY;
        posY %= sizeY;
        if(posY < 0) posY += sizeY;
        posY -= SQUARES_SIZE;
        renderer.rect(posX, posY, posX + SQUARES_SIZE, posY + SQUARES_SIZE);
    }
}
