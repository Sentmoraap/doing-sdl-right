#pragma once

#include "Scenes/Scene.hpp"

class AccurateInputLag : public Scene
{
private:
    bool display = false;

public:
    AccurateInputLag();
    void update(uint16_t frameRate) override;
    void draw() override;
};
