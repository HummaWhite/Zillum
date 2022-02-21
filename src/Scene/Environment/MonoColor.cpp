#include "../../../include/Core/Environment.h"

EnvLiSample EnvSingleColor::sampleLi(const Vec2f &u1, const Vec2f &u2)
{
    auto Wi = Transform::planeToSphere(u1);
    return { Wi, mRadiance, Math::PiInv * 0.25f };
}

float EnvSingleColor::pdfLi(const Vec3f &Wi)
{
    return Math::PiInv * 0.25f;
}