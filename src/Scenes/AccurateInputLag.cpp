#include <cstring>
#include "Scenes/AccurateInputLag.hpp"
#include "Renderer.hpp"
#include "Joysticks.hpp"

AccurateInputLag::AccurateInputLag()
{
    strcpy_s(name, sizeof(name), "Accurate input lag");
}

void AccurateInputLag::update(uint16_t frameRate)
{
    display = false;
    int nbKeys;
    const Uint8* keys = SDL_GetKeyboardState(&nbKeys);
    for(int i = 0; i < nbKeys; i++) if(keys[i]) display = true;
    display |= joysticks.isAnyInputPressed();
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
