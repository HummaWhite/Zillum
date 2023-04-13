#include "Core/PhaseFunction.h"

float HGPhaseFunction(float cosTheta, float g) {
	float denom = 1.f + g * (g + 2 * cosTheta);
	return .25f * Math::PiInv * (1 - g * g) / (denom * Math::sqrtc(denom));
}

float HGPhasePDF(Vec3f wo, Vec3f wi, float g) {
	return HGPhaseFunction(glm::dot(wo, wi), g);
}

PhaseSample HGPhaseSample(Vec3f wo, float g, Vec2f u) {
	float g2 = g * g;
	float cosTheta = (glm::abs(g) < 1e-3f) ?
		1.f - 2.f * u.x :
		-(1 + g2 - Math::square((1 - g2) / (1 + g - 2 * g * u.x)) / (2.f * g));

	float sinTheta = Math::sqrtc(1.f - cosTheta * cosTheta);
	float phi = Math::Pi * 2.f * u.y;
	Vec3f wiLocal(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);

	return PhaseSample(Transform::localToWorld(wo, wiLocal), HGPhaseFunction(cosTheta, g));
}