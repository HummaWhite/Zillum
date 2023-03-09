#include "Core/Environment.h"

EnvLiSample EnvSingleColor::sampleLi(const Vec2f &u1, const Vec2f &u2) {
    auto wi = Transform::planeToSphere(u1);
    return { wi, mRadiance, Math::PiInv * 0.25f };
}

float EnvSingleColor::pdfLi(const Vec3f &wi) {
    return Math::PiInv * 0.25f;
}