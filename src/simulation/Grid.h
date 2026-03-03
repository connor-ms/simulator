#pragma once

#include <glm/glm.hpp>

const int GRID_SIZE = 128;

struct GridNode
{
    uint32_t vX;
    uint32_t vY;
    uint32_t mass;
};