#pragma once

#include <cstdint>

class Scene
{
protected:
    char name[32] = "Dummy";

public:
    const char* getName() const { return name;}
    virtual void displayImGuiSettings() {};
    virtual void update(uint16_t frameRate) {};
    virtual void draw() {};
};
