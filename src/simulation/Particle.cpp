#include "Particle.h"

std::vector<Particle> createParticleArray(int size, int xMax, int yMax, int offset)
{
    std::vector<Particle> instances{};

    std::random_device rand;
    std::mt19937 generator(rand());
    std::uniform_real_distribution<float> xDist(0 + offset, xMax - offset);
    std::uniform_real_distribution<float> yDist(0 + offset, yMax - offset);

    for (int i = 0; i < size; i++)
        instances.push_back({{xDist(generator), yDist(generator)}, glm::vec2(0), glm::f32mat2x2(0), float(0)});

    return instances;
}