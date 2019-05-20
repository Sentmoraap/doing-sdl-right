#pragma once

#include <vector>
#include <SDL2/SDL.h>

class Joysticks
{
    private:
        std::vector<SDL_Joystick*> joysticks;

    public:
        void init();
        bool isAnyInputPressed() const;
        std::pair<int16_t, int16_t> getXY() const;
};

extern Joysticks joysticks;
