#include <cstring>
#include "Scenes/AccurateInputLag.hpp"
#include "Renderer.hpp"

AccurateInputLag::AccurateInputLag()
{
    strcpy(name, "Accurate input lag");
}

void AccurateInputLag::update(uint64_t microseconds, Inputs::State inputs)
{
    display = inputs.pressed;
}

void AccurateInputLag::draw()
{
    if(display)
    {
        renderer.rect(64, 64, 960, 92);
        renderer.rect(64, 368, 960, 400);
        renderer.rect(64, 672, 960, 704);
    }
}
