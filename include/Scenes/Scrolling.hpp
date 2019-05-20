#pragma once

#include "Scenes/Scene.hpp"

class Scrolling : public Scene
{
private:
    int scrollSpeed = 8;
    int scrollX = 0, scrollY = 0;

    static constexpr uint8_t SQUARES_SIZE = 128;

public:
    Scrolling();
    void update(uint16_t frameRate) override;
    void displayImGuiSettings() override;
    void draw() override;
};
