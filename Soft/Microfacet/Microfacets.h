#pragma once

#include "Microfacet.h"

class GGXDistrib:
	public MicrofacetDistrib
{
public:
	GGXDistrib(float roughness, bool sampleVisible, float aniso = 0.0f)
	{
		float r2 = roughness * roughness;
		visible = sampleVisible;
		//float al = glm::sqrt(1.0f - aniso * 0.9f);
		//alpha = glm::vec2(r2 / al, r2 * al);
		alpha = r2;
	}

	float d(const glm::vec3 &N, const glm::vec3 &M)
	{
		return Microfacet::ggx(glm::dot(N, M), alpha);
	}

	float pdf(const glm::vec3 &N, const glm::vec3 &M, const glm::vec3 &Wo)
	{
		if (!visible) return d(N, M);
		return d(N, M) * Microfacet::schlickG(glm::dot(N, Wo), alpha) * Math::absDot(M, Wo) / Math::absDot(N, Wo);
	}

	glm::vec3 sampleWm(const glm::vec3 &N, const glm::vec3 &Wo, const glm::vec2 &u)
	{
		if (visible)
		{
			glm::mat3 TBN = Math::TBNMatrix(N);
			glm::mat3 TBNinv = glm::inverse(TBN);

			glm::vec3 Vh = glm::normalize((TBNinv * Wo) * glm::vec3(alpha, alpha, 1.0f));

			float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
			glm::vec3 T1 = lensq > 0.0f ? glm::vec3(-Vh.y, Vh.x, 0.0f) / glm::sqrt(lensq) : glm::vec3(1.0f, 0.0f, 0.0f);
			glm::vec3 T2 = glm::cross(Vh, T1);

			glm::vec2 xi = Transform::toConcentricDisk(u);
			float s = 0.5f * (1.0f + Vh.z);
			xi.y = (1.0f - s) * glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x)) + s * xi.y;

			glm::vec3 H = T1 * xi.x + T2 * xi.y + Vh * glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x - xi.y * xi.y));
			H = glm::normalize(glm::vec3(H.x * alpha, H.y * alpha, glm::max(0.0f, H.z)));
			return glm::normalize(TBN * H);
		}
		else
		{
			glm::vec2 xi = Transform::toConcentricDisk(u);

			glm::vec3 H = glm::vec3(xi.x, xi.y, glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x - xi.y * xi.y)));
			H = glm::normalize(H * glm::vec3(alpha, alpha, 1.0f));
			return glm::normalize(Transform::normalToWorld(N, H));
		}
	}

	float g(const glm::vec3 &N, const glm::vec3 &Wo, const glm::vec3 &Wi)
	{
		return Microfacet::smithG(N, Wo, Wi, alpha);
	}

private:
	bool visible;
	float alpha;
};

class GTR1Distrib:
	public MicrofacetDistrib
{
public:
	GTR1Distrib(float roughness): alpha(roughness * roughness) {}

	float d(const glm::vec3 &N, const glm::vec3 &M)
	{
		return Microfacet::gtr1(Math::absDot(N, M), alpha);
	}

	float pdf(const glm::vec3 &N, const glm::vec3 &M, const glm::vec3 &Wo)
	{
		return d(N, M) * Math::absDot(N, M);
	}

	glm::vec3 sampleWm(const glm::vec3 &N, const glm::vec3 &Wo, const glm::vec2 &u)
	{
		float cosTheta = glm::sqrt(glm::max(0.0f, (1.0f - glm::pow(alpha, 1.0f - u.x)) / (1.0f - alpha)));
		float sinTheta = glm::sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));
		float phi = 2.0f * u.y * Math::Pi;

		glm::vec3 M = glm::normalize(glm::vec3(glm::cos(phi) * sinTheta, glm::sin(phi) * sinTheta, cosTheta));
		if (!Math::sameHemisphere(N, Wo, M)) M = -M;

		return glm::normalize(Transform::normalToWorld(N, M));
	}

private:
	float alpha;
};
