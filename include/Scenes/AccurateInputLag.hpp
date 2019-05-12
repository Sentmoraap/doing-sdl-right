#pragma once

#include "Scenes/Scene.hpp"
#include <vector>
#include <SDL2/SDL.h>

class AccurateInputLag : public Scene
{
private:
    bool display = false;
    std::vector<SDL_Joystick*> joysticks;

public:
    AccurateInputLag();
    void update(uint16_t frameRate) override;
    void draw() override;
};
