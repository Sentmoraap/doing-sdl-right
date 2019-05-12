#pragma once
#include <cstdint>

class Renderer
{
    public:
        static void init();
        static void rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
};
