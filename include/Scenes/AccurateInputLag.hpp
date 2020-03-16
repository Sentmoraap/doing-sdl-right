#pragma once

#include "Scenes/Scene.hpp"

class AccurateInputLag : public Scene
{
private:
    struct State
    {
        bool display = false;
        uint64_t time = 0;
        uint32_t totalFrames = 0;
        uint16_t presses = 0;
        float lastInputLag = 0;
    };
    State cur, saved;

public:
    AccurateInputLag();
    void displayImGuiSettings() override;
    void update(uint64_t microseconds, Inputs::State inputs) override;
    void saveState() override;
    void loadState() override;
    void draw() override;
    float getInputLag() const;
};
