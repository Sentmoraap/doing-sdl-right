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
}

void AccurateInputLag::update(uint64_t microseconds, Inputs::State inputs)
{
    cur.display = inputs.pressed;
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
