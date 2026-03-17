#include "Particle.h"

#include <cstdlib>

std::vector<Particle> createParticleArray(int size, float width, float height, glm::vec2 origin)
{
    std::vector<Particle> instances;
    instances.reserve(size);

    int countX = static_cast<int>(std::ceil(std::sqrt(size)));
    int countY = static_cast<int>(std::ceil(float(size) / countX));

    float dx = width / float(countX);
    float dy = height / float(countY);

    int created = 0;

    for (int j = 0; j < countY && created < size; j++)
    {
        for (int i = 0; i < countX && created < size; i++)
        {
            int jitter = (std::rand() % 5) + 1;

            float x = origin.x + (i + 0.5f) * dx + jitter;
            float y = origin.y + (j + 0.5f) * dy + jitter;

            Particle p{};
            p.position = {x, y};
            p.velocity = {0.0f, 0.0f};
            p.C = glm::f32mat2x2(0.0f);

            instances.push_back(p);
            created++;
        }
    }

    return instances;
}