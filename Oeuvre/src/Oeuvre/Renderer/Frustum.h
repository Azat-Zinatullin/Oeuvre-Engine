#pragma once

#include "glm/mat4x4.hpp"
#include "AABB.h"

namespace Oeuvre
{
    class Frustum
    {
    public:
        Frustum();
        Frustum(const Frustum&) = delete;
        ~Frustum();

        void ConstructFrustum(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, float screenDepth);
        bool CheckAABB(glm::vec3 min, glm::vec3 max) const;  
        bool CheckSphere(glm::vec3 pos, float radius) const;

    private:
        glm::vec4 m_planes[6];
    };
}

