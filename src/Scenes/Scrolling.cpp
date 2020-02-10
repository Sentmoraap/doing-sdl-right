#include <cstring>
#include "imgui/imgui.h"
#include "Scenes/Scrolling.hpp"
#include "Renderer.hpp"

Scrolling::Scrolling()
{
    strcpy(name, "Scrolling");
}

void Scrolling::update(uint64_t microseconds, Inputs::State inputs)
{
    cur.scrollX += (inputs.x << 16) * static_cast<int64_t>(microseconds) * scrollSpeed / 32767000000;
    cur.scrollY += (inputs.y << 16) * static_cast<int64_t>(microseconds) * scrollSpeed / 32767000000;
}

void Scrolling::saveState()
{
    saved = cur;
}

void Scrolling::loadState()
{
    cur = saved;
}

void Scrolling::displayImGuiSettings()
{
    ImGui::DragInt("Scroll speed", &scrollSpeed, 0.25, 1, 2048);
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
        int posX = static_cast<int>(x * SQUARES_SIZE - (cur.scrollX >> 16));
        posX %= sizeX;
        if(posX < 0) posX += sizeX;
        posX -= SQUARES_SIZE;
        int posY = static_cast<int>(y * SQUARES_SIZE - (cur.scrollY >> 16));
        posY %= sizeY;
        if(posY < 0) posY += sizeY;
        posY -= SQUARES_SIZE;
        renderer.rect(posX, posY, posX + SQUARES_SIZE - 1, posY + SQUARES_SIZE - 1);
    }
}
