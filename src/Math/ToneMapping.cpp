#include "../../include/Core/ToneMapping.h"

NAMESPACE_BEGIN(ToneMapping)

Vec3f reinhard(const Vec3f &color)
{
    return color / (color + Vec3f(1.0f));
}

Vec3f CE(const Vec3f &color)
{
    return Vec3f(1.0f) - glm::exp(-color);
}

Vec3f filmic(const Vec3f &color)
{
    auto calc = [](const Vec3f &x)
    {
        const float A = 0.22f, B = 0.3f, C = 0.1f, D = 0.2f, E = 0.01f, F = 0.3f;
        return ((x * (x * A + Vec3f(B * C)) + Vec3f(D * E)) / (x * (x * A + Vec3f(B)) + Vec3f(D * F)) - Vec3f(E / F));
    };
    const float WHITE = 11.2f;

    return calc(color * 1.6f) / calc(Vec3f(WHITE));
}

Vec3f ACES(const Vec3f &color)
{
    return (color * (color * 2.51f + Vec3f(0.03f))) / (color * (color * 2.43f + Vec3f(0.59f)) + Vec3f(0.14f));
}

NAMESPACE_END(ToneMapping)