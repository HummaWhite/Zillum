#include "Color.h"

RGB24::RGB24(glm::vec3 color)
{
    glm::vec3 c = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
    c *= 255;
    *this = RGB24(c.x, c.y, c.z);
}

RGB24 RGB24::swapRB(RGB24 c)
{
    std::swap(c.r, c.b);
    return c;
}