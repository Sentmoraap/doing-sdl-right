#include <algorithm>
#include "Scenes/GhettoInputLag.hpp"
#include "Renderer.hpp"
#include "imgui/imgui.h"

GhettoInputLag::GhettoInputLag()
{
    strcpy(name, "Ghetto input lag");
}

void GhettoInputLag::displayImGuiSettings()
{
    if(cur.time == 0)
    {
        ImGui::Text("%6d µs", cur.lag);
    }
}

void GhettoInputLag::update(uint64_t microseconds, Inputs::State inputs)
{
    if(cur.time == 0)
    {
        if(inputs.pressed && !cur.prevInput) cur.time = BEAT_TIME * (NB_BEATS + 2);
        cur.beat = 0;
    }
    else
    {
        if(inputs.pressed && !cur.prevInput && cur.beat < NB_BEATS)
        {
            int64_t diff = BEAT_TIME * (NB_BEATS - cur.beat) - cur.time;
            cur.diffs[cur.beat++] = diff;
        }
        if(cur.time <= microseconds)
        {
            cur.time = 0;
            std::sort(cur.diffs.begin(), cur.diffs.end());
            int64_t total = 0;
            for(uint8_t i = NB_BEATS / 2 - SPREAD; i <= NB_BEATS / 2 + SPREAD; i++) total += cur.diffs[i];
            cur.lag = static_cast<int32_t>(total / (2 * SPREAD + 1));

        }
        else cur.time -= microseconds;
    }
    cur.prevInput = inputs.pressed;
}

void GhettoInputLag::saveState()
{
    saved = cur;
}

void GhettoInputLag::loadState()
{
    cur = saved;
}

void GhettoInputLag::draw()
{
    renderer.rect(100, 354, 100, 416);
    renderer.rect(99, 322, 101, 353);
    renderer.rect(99, 417, 101, 448);
    if(cur.time) for(uint8_t i = 0; i < NB_BEATS; i++)
    {
        int16_t posX = static_cast<int16_t>(100 + (static_cast<int64_t>(cur.time)
                - static_cast<int64_t>(BEAT_TIME * (NB_BEATS - i))) * BEAT_SPEED / static_cast<int64_t>(BEAT_TIME));
        renderer.rect(posX, 354, posX, 416);
    }
}
