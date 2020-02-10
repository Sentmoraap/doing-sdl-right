#pragma once

#include <cstdint>
#include "Inputs.hpp"

class Scene
{
protected:
    char name[32] = "Dummy";

public:
    const char* getName() const { return name;}
    virtual void displayImGuiSettings() {};
    virtual void update(uint64_t microseconds, Inputs::State inputs) {};
    virtual void saveState() {};
    virtual void loadState() {};
    virtual void draw() {};
};
