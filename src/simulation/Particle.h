#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <random>

struct Particle
{
    glm::vec2 position;
    glm::vec2 velocity;
    glm::f32mat2x2 C;
    float J;
    float _pad;
};

std::vector<Particle> createParticleArray(int size, int xMax, int yMax, int offset = 0);