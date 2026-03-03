#include "Particle.h"

std::vector<Particle> createParticleArray(int size)
{
    std::vector<Particle> instances{};

    const int WIDTH = 250;

    std::random_device rand;
    std::mt19937 generator(rand());
    std::uniform_real_distribution<float> dist(-WIDTH, WIDTH);

    for (int i = 0; i < size; i++)
        instances.push_back({{dist(generator), dist(generator)}, glm::vec2(0), glm::f32mat2x2(0), float(0)});

    return instances;
}