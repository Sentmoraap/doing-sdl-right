#pragma once

#include "Scenes/Scene.hpp"

class AccurateInputLag : public Scene
{
private:
    struct State
    {
        bool display;
        uint64_t time = 0;
    };
    State cur, saved;

public:
    AccurateInputLag();
    void displayImGuiSettings() override;
    void update(uint64_t microseconds, Inputs::State inputs) override;
    void saveState() override;
    void loadState() override;
    void draw() override;
};
