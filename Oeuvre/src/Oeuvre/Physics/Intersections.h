#pragma once

#include "glm/vec3.hpp"

namespace Oeuvre::Intersections
{
    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;
    };


    struct Triangle {
        glm::vec3 v0, v1, v2;
    };

    bool intersectRayTriangle(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDirection,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        float& distance,
        glm::vec3& outIntersectionPoint);
}

