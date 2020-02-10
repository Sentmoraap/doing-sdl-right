#pragma once

#include "Scenes/Scene.hpp"

class Scrolling : public Scene
{
private:
    int scrollSpeed = 960; // per second

    struct State
    {
        int64_t scrollX = 0, scrollY = 0; //  >> 16
    };

    State cur, saved;

    static constexpr uint8_t SQUARES_SIZE = 128;

public:
    Scrolling();
    void update(uint64_t microseconds, Inputs::State inputs) override;
    void saveState() override;
    void loadState() override;
    void displayImGuiSettings() override;
    void draw() override;
};
