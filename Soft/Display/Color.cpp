#include "Color.h"

RGB24::RGB24(Vec3f color)
{
    Vec3f c = glm::clamp(color, Vec3f(0.0f), Vec3f(1.0f));
    c *= 255;
    *this = RGB24(c.x, c.y, c.z);
}

RGB24 RGB24::swapRB(RGB24 c)
{
    std::swap(c.r, c.b);
    return c;
}