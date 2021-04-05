#include "stdafx.h"
#include "Utils/Frustum.h"

namespace Frostium
{
	void Frustum::Update(const glm::mat4& matrix)
	{
		planes[side::LEFT].x = matrix[0].w + matrix[0].x;
		planes[side::LEFT].y = matrix[1].w + matrix[1].x;
		planes[side::LEFT].z = matrix[2].w + matrix[2].x;
		planes[side::LEFT].w = matrix[3].w + matrix[3].x;

		planes[RIGHT].x = matrix[0].w - matrix[0].x;
		planes[RIGHT].y = matrix[1].w - matrix[1].x;
		planes[RIGHT].z = matrix[2].w - matrix[2].x;
		planes[RIGHT].w = matrix[3].w - matrix[3].x;

		planes[TOP].x = matrix[0].w - matrix[0].y;
		planes[TOP].y = matrix[1].w - matrix[1].y;
		planes[TOP].z = matrix[2].w - matrix[2].y;
		planes[TOP].w = matrix[3].w - matrix[3].y;

		planes[BOTTOM].x = matrix[0].w + matrix[0].y;
		planes[BOTTOM].y = matrix[1].w + matrix[1].y;
		planes[BOTTOM].z = matrix[2].w + matrix[2].y;
		planes[BOTTOM].w = matrix[3].w + matrix[3].y;

		planes[BACK].x = matrix[0].w + matrix[0].z;
		planes[BACK].y = matrix[1].w + matrix[1].z;
		planes[BACK].z = matrix[2].w + matrix[2].z;
		planes[BACK].w = matrix[3].w + matrix[3].z;

		planes[FRONT].x = matrix[0].w - matrix[0].z;
		planes[FRONT].y = matrix[1].w - matrix[1].z;
		planes[FRONT].z = matrix[2].w - matrix[2].z;
		planes[FRONT].w = matrix[3].w - matrix[3].z;

		for (auto i = 0; i < planes.size(); i++)
		{
			float length = sqrtf(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
			planes[i] /= length;
		}
	}

	bool Frustum::CheckSphere(const glm::vec3& pos, float radius) const
	{
		for (auto i = 0; i < planes.size(); i++)
		{
			if ((planes[i].x * pos.x) + (planes[i].y * pos.y) + (planes[i].z * pos.z) + planes[i].w <= -radius)
				return false;
		}

		return true;
	}
}