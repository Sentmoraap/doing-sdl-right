#pragma once

#include "Scenes/Scene.hpp"

class AccurateInputLag : public Scene
{
private:
    struct State
    {
        bool display = false;
        uint64_t time = 0;
        uint32_t totalFrames0 = 0, totalFrames1 = 0;
        uint16_t presses = 0;
        float lastInputLag0 = 0, lastInputLag1 = 0;
    };
    State cur, saved;

public:
    AccurateInputLag();
    void displayImGuiSettings() override;
    void update(uint64_t microseconds, Inputs::State inputs) override;
    void saveState() override;
    void loadState() override;
    void draw() override;
    std::pair<float, float> getInputLags() const;
};
