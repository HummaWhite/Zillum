#include "../../../include/Core/Environment.h"

EnvLiSample EnvSingleColor::sampleLi(const Vec2f &u1, const Vec2f &u2)
{
    auto Wi = Transform::planeToSphere(u1);
    return {Wi, radiance, Math::PiInv * 0.25f};
}

float EnvSingleColor::pdfLi(const Vec3f &Wi)
{
    return Math::PiInv * 0.25f;
}

LightLeSample EnvSingleColor::sampleLe(float radius, const std::array<float, 6> &u)
{
    auto Wi = Transform::planeToSphere({u[0], u[1]});
    auto ori = Vec3f(Transform::toConcentricDisk({u[2], u[3]}), 1.0f) * radius;
    ori = Transform::normalToWorld(Wi, ori);

    float pdf = Math::PiInv * 0.25f * Math::PiInv * Math::square(1.0f / radius);
    return {{ori, -Wi}, radiance, pdf};
}