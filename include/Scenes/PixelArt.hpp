#pragma once

#include <GL/glew.h>
#include "Scenes/Scene.hpp"

class PixelArt : public Scene
{
private:
    GLuint texture;

public:
    PixelArt();
    void init();
    void draw() override;
};
