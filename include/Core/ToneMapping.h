#pragma once

#include <iostream>
#include <cmath>

#include "glmIncluder.h"
#include "Utils/NamespaceDecl.h"
#include "Spectrum.h"

NAMESPACE_BEGIN(ToneMapping)

Vec3f reinhard(const Vec3f &color);
Vec3f CE(const Vec3f &color);
Vec3f filmic(const Vec3f &color);
Vec3f ACES(const Vec3f &color);

NAMESPACE_END(ToneMapping)