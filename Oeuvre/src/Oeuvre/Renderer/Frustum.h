#pragma once

#include "glm/mat4x4.hpp"

namespace Oeuvre
{
    class Frustum
    {
    public:
        Frustum();
        Frustum(const Frustum&);
        ~Frustum();

        void ConstructFrustum(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, float screenDepth);
        bool CheckCube(glm::vec3 min, glm::vec3 max) const;   

    private:
        glm::vec4 m_planes[6];
    };
}

