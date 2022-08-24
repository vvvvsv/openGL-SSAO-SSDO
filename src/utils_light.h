#pragma once

#include "gl_env.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class Light
{
public:
    // position
    glm::vec3 lightPos;
    // color
    glm::vec3 lightColor;

    Light(glm::vec3 pos, glm::vec3 color)
    {
        lightPos = pos;
        lightColor = color;
    }
};