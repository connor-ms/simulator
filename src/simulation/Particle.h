#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <random>

struct Particle
{
    glm::vec2 position;
    glm::vec2 velocity;
    glm::f32mat2x2 C;
    float debug1;
    float debug2;
};

std::vector<Particle> createParticleArray(int size, float width, float height, glm::vec2 origin);