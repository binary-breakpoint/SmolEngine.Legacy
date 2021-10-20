#pragma once
#include "Common/Memory.h"

#include <glm/glm.hpp>
#include <array>

namespace SmolEngine
{
    struct BoundingBox
    {
        BoundingBox();
        ~BoundingBox();

        void             Reset();
        void             MinPoint(const glm::vec3& val);
        void             MaxPoint(const glm::vec3& val);
        void             Transform(const glm::mat4x4& model);

        const glm::vec3& MinPoint(bool transformed = false) const;
        const glm::vec3& MaxPoint(bool transformed = false) const;
        const glm::vec3& Center(bool transformed = false) const;
        const glm::vec3& Extent(bool transformed = false) const;

    private:
        std::array<glm::vec3, 2>  boundaries;
        Ref<BoundingBox>          original;
        glm::vec3                 center;
        glm::vec3                 extent;
	};
}