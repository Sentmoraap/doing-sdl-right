#include "Inputs.hpp"
#include <tuple>

void Inputs::init()
{
    int nbJoysticks = SDL_NumJoysticks();
    for(int iJs = 0; iJs < nbJoysticks; iJs++)
    {
        SDL_Joystick *js = SDL_JoystickOpen(iJs);
        if(js) joysticks.push_back(js);
    }
}

Inputs::State Inputs::getState()
{
    SDL_PumpEvents();
    State ret;
    ret.pressed = isAnyInputPressed();
    int nbKeys;
    const Uint8* keys = SDL_GetKeyboardState(&nbKeys);
    ret.test = nbKeys > SDL_SCANCODE_T && keys[SDL_SCANCODE_T];
    ret.reset = nbKeys > SDL_SCANCODE_R && keys[SDL_SCANCODE_R];
    std::tie(ret.x, ret.y) = getXY();
    return ret;
}

bool Inputs::isAnyInputPressed() const
{
    // Joystick
    for(SDL_Joystick *js : joysticks)
    {
        int n = SDL_JoystickNumHats(js);
        for(int iHat = 0; iHat < n; iHat++) if(SDL_JoystickGetHat(js, iHat) != SDL_HAT_CENTERED) return true;
        n = SDL_JoystickNumButtons(js);
        for(int iBtn = 0; iBtn < n; iBtn++) if(SDL_JoystickGetButton(js, iBtn)) return true;
    }

    // Keyboard
    int nbKeys;
    const Uint8* keys = SDL_GetKeyboardState(&nbKeys);
    for(int i = 0; i < nbKeys; i++) if(keys[i] && i != SDL_SCANCODE_T && i != SDL_SCANCODE_R) return true;

    return false;
}

std::pair<int16_t, int16_t> Inputs::getXY() const
{
    int x = 0, y = 0;

    // Joystick
    for(SDL_Joystick *js : joysticks)
    {
        if(SDL_JoystickNumHats(js))
        {
            switch(SDL_JoystickGetHat(js, 0))
            {
                case SDL_HAT_LEFTUP:
                	x -= 32767;
                    y += 32767;
                    break;

                case SDL_HAT_UP:
                    y += 32767;
                    break;


                case SDL_HAT_RIGHTUP:
                    x += 32767;
                    y += 32767;

                case SDL_HAT_LEFT:
                    x -= 32767;
                    break;

                case SDL_HAT_CENTERED:
                    break;

                case SDL_HAT_RIGHT:
                    x += 32767;
                    break;

                case SDL_HAT_LEFTDOWN:
                    x -= 32767;
                    y -= 32767;
                    break;

                case SDL_HAT_DOWN:
                    y -= 32767;
                    break;

                case SDL_HAT_RIGHTDOWN:
                    x += 32767;
                    y -= 32767;
            }
        }
        if(SDL_JoystickNumAxes(js) >= 2)
        {
            x += SDL_JoystickGetAxis(js, 0);
            y += SDL_JoystickGetAxis(js, 1);
        }
    }
    if(x < -32767) x = -32767; else if(x > 32767) x = 32767;
    if(y < -32767) y = -32767; else if(y > 32767) y = 32767;

    // Keyboard
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    if(keys[SDL_SCANCODE_LEFT])  x -= 32767;
    if(keys[SDL_SCANCODE_RIGHT]) x += 32767;
    if(keys[SDL_SCANCODE_UP])    y -= 32767;
    if(keys[SDL_SCANCODE_DOWN])  y += 32767;
    if(x < -32767) x = -32767; else if(x > 32767) x = 32767;
    if(y < -32767) y = -32767; else if(y > 32767) y = 32767;

    return std::make_pair(x, y);
}
