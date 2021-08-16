#pragma once

#include "Tools/GLM.h"

#include <array>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	class Frustum
	{
	public:
		void SetRadius(float value);
		void Update(const glm::mat4& matrix);
		bool CheckSphere(const glm::vec3& pos) const;

	private:

		enum side
		{
			LEFT = 0,
			RIGHT = 1,
			TOP = 2,
			BOTTOM = 3,
			BACK = 4,
			FRONT = 5
		};

		std::array<glm::vec4, 6> planes;
		float                    radius = 25.0f;
	};
}