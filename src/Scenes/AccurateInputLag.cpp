#include <cstring>
#include "Scenes/AccurateInputLag.hpp"
#include "Renderer.hpp"

AccurateInputLag::AccurateInputLag()
{
    strcpy(name, "Accurate input lag");
    int nbJoysticks = SDL_NumJoysticks();
    for(int iJs = 0; iJs < nbJoysticks; iJs++)
    {
        SDL_Joystick *js = SDL_JoystickOpen(iJs);
        if(js) joysticks.push_back(js);
    }
}

void AccurateInputLag::update(uint16_t frameRate)
{
    display = false;
    int nbKeys;
    const Uint8* keys = SDL_GetKeyboardState(&nbKeys);
    for(int i = 0; i < nbKeys; i++) if(keys[i]) display = true;
    for(SDL_Joystick *js : joysticks)
    {
        int n = SDL_JoystickNumHats(js);
        for(int iHat = 0; iHat < n; iHat++) if(SDL_JoystickGetHat(js, iHat) != SDL_HAT_CENTERED) display = true;
        n = SDL_JoystickNumButtons(js);
        for(int iBtn = 0; iBtn < n; iBtn++) if(SDL_JoystickGetButton(js, iBtn)) display = true;
    }
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
