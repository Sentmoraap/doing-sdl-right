#pragma once

#include "Scenes/Scene.hpp"

class AccurateInputLag : public Scene
{
private:
    bool display = false;

public:
    AccurateInputLag();
    void update(uint64_t microseconds, Inputs::State inputs) override;
    void draw() override;
};
