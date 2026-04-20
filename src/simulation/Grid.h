#pragma once

#include <glm/glm.hpp>

struct GridNode
{
    std::atomic<int32_t> vX;
    std::atomic<int32_t> vY;
    std::atomic<int32_t> vZ;
    std::atomic<int32_t> mass;
};