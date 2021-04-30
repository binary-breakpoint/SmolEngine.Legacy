#include "stdafx.h"
#include "Animator.h"

#include "Common/Mesh.h"

namespace Frostium
{
	AnimatorStorage* Animator::s_Storage = new AnimatorStorage();

	void Animator::SubmitAnimaton(Mesh* mesh)
	{
		s_Storage->m_Skins[s_Storage->m_SkinIndex] = mesh;
		s_Storage->m_SkinIndex++;
	}

	void Animator::SubmitAnimaton2D()
	{

	}

	void Animator::BeginSubmit(float deltaTime)
	{
		s_Storage->m_SkinIndex = 0;
		s_Storage->m_DeltaTime = deltaTime;
	}

	void Animator::EndSubmit()
	{
		for (uint32_t i = 0; i < s_Storage->m_SkinIndex; ++i)
		{
			Mesh* mesh = s_Storage->m_Skins[i];
			if(mesh->m_PlayAnimations)
				mesh->UpdateAnimations(s_Storage->m_DeltaTime);
		}
	}
}