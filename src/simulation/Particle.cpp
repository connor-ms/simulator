#include "Particle.h"

std::vector<Particle> createParticleArray(
    int size,
    float xMax,   // should be 1.0
    float yMax,   // should be 1.0
    float offset) // something like 0.05
{
    std::vector<Particle> instances;
    instances.reserve(size);

    // Make grid roughly square
    int countX = static_cast<int>(std::ceil(std::sqrt(size)));
    int countY = static_cast<int>(std::ceil(float(size) / countX));

    float usableWidth = xMax - 2.0f * offset;
    float usableHeight = yMax - 2.0f * offset;

    float dx = usableWidth / float(countX);
    float dy = usableHeight / float(countY);

    int created = 0;

    for (int j = 0; j < countY && created < size; j++)
    {
        for (int i = 0; i < countX && created < size; i++)
        {
            float x = offset + (i + 0.5f) * dx;
            float y = offset + (j + 0.5f) * dy;

            Particle p{};
            p.position = {x, y};
            p.velocity = {0.0f, 0.0f};
            p.C = glm::f32mat2x2(0.0f);
            // p.mass = 1.0f;

            instances.push_back(p);
            created++;
        }
    }

    return instances;
}