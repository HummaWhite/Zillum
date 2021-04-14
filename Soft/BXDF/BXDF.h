#pragma once

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Math/Math.h"

class BXDF
{
public:
	enum
	{
		Diffuse			= 1 << 0,
		GlosRefl		= 1 << 1,
		SpecRefl		= 1 << 2,
		SpecTrans		= 1 << 3,
		GlosTrans		= 1 << 4,
		All				= 0b11111
	};

public:
	BXDF(int type): mType(type) {}

	bool hasType(int type) const
	{
		return type & mType;
	}

	bool isDelta() const
	{
		return (mType & SpecRefl) || (mType & SpecTrans);
	}

	static bool isDelta(int type)
	{
		return (type & SpecRefl) || (type & SpecTrans);
	}

	int type() const { return mType; }

protected:
	int mType;
};
