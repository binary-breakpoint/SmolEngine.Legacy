#include "stdafx.h"
#include "Camera/Camera.h"

namespace SmolEngine
{
	SceneViewProjection::SceneViewProjection(Camera* cam)
	{
		Update(cam);
	}

	void SceneViewProjection::Update(Camera* cam)
	{
		View = cam->GetViewMatrix();
		Projection = cam->GetProjection();
		CamPos = glm::vec4(cam->GetPosition(), 1.0f);
		NearClip = cam->GetNearClip();
		FarClip = cam->GetFarClip();
		SkyBoxMatrix = glm::mat4(glm::mat3(View));
	}
}