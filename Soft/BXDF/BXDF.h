#ifndef BXDF_H
#define BXDF_H

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Math/Math.h"

class BXDF
{
public:
	enum Type
	{
		REFLECTION 		= 1 << 0,
		TRANSMISSION 	= 1 << 1,
		DIFFUSE			= 1 << 2,
		GLOSSY			= 1 << 3,
		SPECULAR		= 1 << 4,
		ALL				= 0b11111
	};

public:
	BXDF(Type _type): type(_type) {}

protected:
	Type type;
};

#endif
