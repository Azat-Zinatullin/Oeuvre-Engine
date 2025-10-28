#include "ovpch.h"
#include "Intersections.h"
#include <limits>

namespace Oeuvre::Intersections
{
    bool intersectRayTriangle(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDirection,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        float& distance,
        glm::vec3& outIntersectionPoint)
    {
        const float EPSILON = 0.0000001f;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        glm::vec3 pvec = glm::cross(rayDirection, edge2);
        float det = glm::dot(edge1, pvec);

        // If determinant is close to zero, ray is parallel to triangle plane
        if (glm::abs(det) < EPSILON) {
            return false;
        }

        float invDet = 1.0f / det;

        glm::vec3 tvec = rayOrigin - v0;
        float u = glm::dot(tvec, pvec) * invDet;

        if (u < 0.0f || u > 1.0f) {
            return false;
        }

        glm::vec3 qvec = glm::cross(tvec, edge1);
        float v = glm::dot(rayDirection, qvec) * invDet;

        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }

        float t = glm::dot(edge2, qvec) * invDet;

        if (t > EPSILON) { // Intersection occurs in front of the ray origin
            distance = t;
            outIntersectionPoint = rayOrigin + rayDirection * t;
            return true;
        }

        return false; // No intersection within the valid range
    }

}

