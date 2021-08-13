#include "ToneMapping.h"

namespace ToneMapping
{
    glm::vec3 reinhard(const glm::vec3 &color)
    {
        return color / (color + glm::vec3(1.0f));
    }

    glm::vec3 CE(const glm::vec3 &color)
    {
        return glm::vec3(1.0f) - glm::exp(-color);
    }

    glm::vec3 filmic(const glm::vec3 &color)
    {
        auto calc = [](const glm::vec3 &x)
        {
            const float A = 0.22f, B = 0.3f, C = 0.1f, D = 0.2f, E = 0.01f, F = 0.3f;
            return ((x * (x * A + glm::vec3(B * C)) + glm::vec3(D * E)) / (x * (x * A + glm::vec3(B)) + glm::vec3(D * F)) - glm::vec3(E / F));
        };
        const float WHITE = 11.2f;

        return calc(color * 1.6f) / calc(glm::vec3(WHITE));
    }

    glm::vec3 ACES(const glm::vec3 &color)
    {
        return (color * (color * 2.51f + glm::vec3(0.03f))) / (color * (color * 2.43f + glm::vec3(0.59f)) + glm::vec3(0.14f));
    }
}