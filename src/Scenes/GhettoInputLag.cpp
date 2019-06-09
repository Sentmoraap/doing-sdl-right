#include <algorithm>
#include "Scenes/GhettoInputLag.hpp"
#include "Renderer.hpp"
#include "Joysticks.hpp"
#include "imgui/imgui.h"

GhettoInputLag::GhettoInputLag()
{
    strcpy(name, "Ghetto input lag");
}

void GhettoInputLag::displayImGuiSettings()
{
    if(time == 0)
    {
        ImGui::Text("%6d µs", lag);
    }
}

#include <iostream>

void GhettoInputLag::update(uint16_t frameRate)
{
    beatTime = frameRate * 3 / 4;

    bool input = false;
    int nbKeys;
    const Uint8* keys = SDL_GetKeyboardState(&nbKeys);
    for(int i = 0; i < nbKeys; i++) if(keys[i]) input = true;
    input |= joysticks.isAnyInputPressed();

    if(time == 0)
    {
        if(input && !prevInput) time = beatTime * (NB_BEATS + 1);
    }
    else
    {
        if(input && !prevInput && beat < NB_BEATS)
        {
            int16_t diff = beatTime * (NB_BEATS - beat) - time;
            diffs[beat] = diff;
            beat++;
        }
        time--;
        if(time == 0)
        {
            std::sort(diffs.begin(), diffs.end());
            int16_t total = 0;
            for(uint8_t i = NB_BEATS / 2 - SPREAD; i <= NB_BEATS / 2 + SPREAD; i++) total += diffs[i];
            lag = total * 1000000 / (frameRate * (2 * SPREAD + 1));
        }
    }
    prevInput = input;
}

void GhettoInputLag::draw()
{
    renderer.rect(100, 354, 100, 416);
    renderer.rect(99, 322, 101, 353);
    renderer.rect(99, 417, 101, 448);
    if(time) for(uint8_t i = 0; i < NB_BEATS; i++)
    {
        uint16_t posX = 100 + (time - beatTime * (NB_BEATS - i)) * BEAT_SPEED / beatTime;
        renderer.rect(posX, 354, posX, 416);
    }
}
