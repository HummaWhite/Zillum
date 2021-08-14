#pragma once

#include <iostream>
#include <memory>
#include <array>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Math/Math.h"

enum class SamplerType
{
	Independent, SimpleSobol
};

class Sampler;
typedef std::shared_ptr<Sampler> SamplerPtr;

class Sampler
{
public:
	Sampler(SamplerType type) : type(type) {}

	virtual float get1D() = 0;
	virtual glm::vec2 get2D() = 0;

	template<int N>
	std::array<float, N> get()
	{
		auto ret = std::array<float, N>();
		for (int i = 0; i < N; i++) ret[i] = get1D();
		return ret;
	}

	virtual void setPixel(int x, int y) = 0;
	virtual void nextSample() = 0;
	virtual bool isProgressive() const = 0;
	virtual std::shared_ptr<Sampler> copy() = 0;

	SamplerType getType() const { return type; }

private:
	SamplerType type;
};