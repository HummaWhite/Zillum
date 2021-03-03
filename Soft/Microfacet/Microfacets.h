#ifndef MICROFACETS_H
#define MICROFACETS_H

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
		return ggx(glm::dot(N, M));
	}

	float pdf(const glm::vec3 &N, const glm::vec3 &M, const glm::vec3 &Wo)
	{
		float cosTheta = glm::dot(N, M);
		if (!visible) return ggx(cosTheta);
		return ggx(cosTheta) * schlickG(glm::dot(N, Wo)) * Math::absDot(M, Wo) / Math::absDot(N, Wo);
	}

	float pdf(float cosTheta, float NoWo, float MoWo)
	{
		return schlickG(NoWo) * ggx(cosTheta) * MoWo / NoWo;
	}

	glm::vec3 sampleWm(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		if (visible)
		{
			glm::mat3 TBN = Math::TBNMatrix(N);
			glm::mat3 TBNinv = glm::inverse(TBN);

			glm::vec3 Vh = glm::normalize((TBNinv * Wo) * glm::vec3(alpha, alpha, 1.0f));

			float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
			glm::vec3 T1 = lensq > 0.0f ? glm::vec3(-Vh.y, Vh.x, 0.0f) / glm::sqrt(lensq) : glm::vec3(1.0f, 0.0f, 0.0f);
			glm::vec3 T2 = glm::cross(Vh, T1);

			glm::vec2 xi = Transform::toConcentricDisk(Math::randBox());
			float s = 0.5f * (1.0f + Vh.z);
			xi.y = (1.0f - s) * glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x)) + s * xi.y;

			glm::vec3 H = T1 * xi.x + T2 * xi.y + Vh * glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x - xi.y * xi.y));
			H = glm::normalize(glm::vec3(H.x * alpha, H.y * alpha, glm::max(0.0f, H.z)));
			return glm::normalize(TBN * H);
		}
		else
		{
			RandomGenerator rg;
			glm::vec2 xi = Transform::toConcentricDisk(glm::vec2(rg.get(), rg.get()));

			glm::vec3 H = glm::vec3(xi.x, xi.y, glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x - xi.y * xi.y)));
			H = glm::normalize(H * glm::vec3(alpha, alpha, 1.0f));
			return glm::normalize(Transform::normalToWorld(N, H));
		}
	}

	float schlickG(float cosTheta)
	{
		float k = alpha * 0.5f;
		return cosTheta / (cosTheta * (1.0f - k) + k);
	}

	float g(const glm::vec3 &N, const glm::vec3 &Wo, const glm::vec3 &Wi)
	{
		float NoWi = Math::absDot(N, Wo);
		float NoWo = Math::absDot(N, Wi);

		float g1 = schlickG(NoWo);
		float g2 = schlickG(NoWi);

		return g1 * g2;
	}

	float ggx(float cosTheta)
	{
		if (cosTheta < 1e-6f) return 0.0f;

		float a2 = alpha * alpha;
		float nom = a2;
		float denom = cosTheta * cosTheta * (a2 - 1.0f) + 1.0f;
		denom = denom * denom * Math::Pi;

		return nom / denom;
	}

	float ggx(float cosTheta, float sinPhi, const glm::vec2 &alph)
	{
		if (cosTheta < 1e-6f) return 0.0f;

		float sinPhi2 = sinPhi * sinPhi;

		float p = (1.0f - sinPhi2) / (alph.x * alph.x) + sinPhi2 / (alph.y * alph.y);
		float k = 1.0f + (p - 1.0f) * (1.0f - cosTheta * cosTheta);
		k = k * k * Math::Pi * alph.x * alph.y;

		return 1.0f / k;
	}

private:
	bool visible;
	float alpha;
};

#endif
