#include "Particle.h"

#include <cstdlib>
#include <iostream>

std::vector<Particle> createParticleArray(int count, float worldSize, float spacing)
{
    std::vector<Particle> instances;

    int n = static_cast<int>(std::ceil(std::cbrt(float(count))));
    instances.reserve(n * n * n);

    float cubeSize = n * spacing;

    float startX = (worldSize - cubeSize) / 2.0f;
    float startY = (worldSize - cubeSize) / 2.0f;
    float startZ = (worldSize - cubeSize) / 2.0f;

    for (int k = 0; k < n; k++)
    {
        for (int j = 0; j < n; j++)
        {
            for (int i = 0; i < n; i++)
            {
                float x = startX + i * spacing;
                float y = startY + j * spacing;
                float z = startZ + k * spacing;

                if (instances.size() == count)
                    break;

                Particle p{};
                p.position = {x, y, z};
                p.velocity = {0.0f, 0.0f, 0.0f};
                instances.push_back(p);
            }
        }
    }
    return instances;
}