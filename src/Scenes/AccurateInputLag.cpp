#include <cstring>
#include "Scenes/AccurateInputLag.hpp"
#include "Renderer.hpp"
#include "imgui/imgui.h"

AccurateInputLag::AccurateInputLag()
{
    strcpy(name, "Accurate input lag");
}

void AccurateInputLag::displayImGuiSettings()
{
    ImGui::Text("%2d:%02d.%06d", static_cast<int>(cur.time / 60000000),
            static_cast<int>((cur.time % 60000000) / 1000000), static_cast<int>(cur.time % 1000000));
    if(cur.presses != 0)
    {
        ImGui::Text("%d.%d frames", cur.totalFrames / cur.presses, (10 * cur.totalFrames / cur.presses) % 10);
    }
}

void AccurateInputLag::update(uint64_t microseconds, Inputs::State inputs)
{
    bool prev = cur.display;
    cur.display = inputs.pressed;
    if(inputs.reset)
    {
        if(cur.presses) cur.lastInputLag = static_cast<float>(cur.totalFrames) / cur.presses;
        cur.totalFrames = 0;
        cur.presses = 0;
    }
    if(cur.display)
    {
        if(!prev) cur.presses++;
        else cur.totalFrames++;
    }
    cur.time += microseconds;
}

void AccurateInputLag::saveState()
{
    saved = cur;
}

void AccurateInputLag::loadState()
{
    cur = saved;
}

void AccurateInputLag::draw()
{
    if(cur.display)
    {
        renderer.rect(64, 0, 960, 32);
        renderer.rect(64, 368, 960, 400);
        renderer.rect(64, 736, 960, 768);
    }
}

float AccurateInputLag::getInputLag() const
{
    return cur.presses ? static_cast<float>(cur.totalFrames) / cur.presses : cur.lastInputLag;
}
