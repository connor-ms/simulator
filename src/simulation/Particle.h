#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <random>

struct Particle
{
    glm::vec3 position;
    float _pad0;
    glm::vec3 velocity;
    float _pad1;
    glm::vec4 C[3];
    float debug1;
    float debug2;
    float _pad3;
    float _pad4;
};

std::vector<Particle> createParticleArray(int count, float worldSize, float wallMargin);