#pragma once

#include <vector>
#include <SDL2/SDL.h>

class Inputs
{
    private:
        std::vector<SDL_Joystick*> joysticks;

        bool isAnyInputPressed() const;
        std::pair<int16_t, int16_t> getXY() const;

    public:
        struct State
        {
            int16_t x, y;
            bool pressed : 1, test : 1, reset : 1;
        };
        void init();
        State getState();
};
