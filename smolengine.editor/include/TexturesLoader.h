#pragma once
#include <Core/Core.h>

#include "Primitives/Texture.h"

namespace SmolEngine
{
	struct TexturesLoader
	{
		TexturesLoader()
		{
			TextureCreateInfo texCI = {};
			texCI.bImGUIHandle = true;

			texCI.FilePath = "assets/buttons/play_button.png";
			m_PlayButton = Texture::Create();
			m_PlayButton->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/pause_button.png";
			m_StopButton = Texture::Create();
			m_StopButton->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/move_button.png";
			m_MoveButton = Texture::Create();
			m_MoveButton->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/rotate_button.png";
			m_RotateButton = Texture::Create();
			m_RotateButton->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/scale_button.png";
			m_ScaleButton = Texture::Create();
			m_ScaleButton->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/search_button.png";
			m_SearchButton = Texture::Create();
			m_SearchButton->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/remove_button.png";
			m_RemoveButton = Texture::Create();
			m_RemoveButton->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/folder_button.png";
			m_FolderButton = Texture::Create();
			m_FolderButton->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/documents_button.png";
			m_DocumentsIcon = Texture::Create();
			m_DocumentsIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/scene_button.png";
			m_SceneIcon = Texture::Create();
			m_SceneIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/default_background.jpg";
			m_BackgroundIcon = Texture::Create();
			m_BackgroundIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/cube_icon.png";
			m_CubeIcon = Texture::Create();
			m_CubeIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/sphere_icon.png";
			m_SphereIcon = Texture::Create();
			m_SphereIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/capsule_icon.png";
			m_CapsuleIcon = Texture::Create();
			m_CapsuleIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/torus_icon.png";
			m_TorusIcon = Texture::Create();
			m_TorusIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/return_button.png";
			m_ReturnIcon = Texture::Create();
			m_ReturnIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/forward_button.png";
			m_ForwardIcon = Texture::Create();
			m_ForwardIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/update_button.png";
			m_UpdateIcon = Texture::Create();
			m_UpdateIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/blueprint_icon.png";
			m_BlueprintIcon = Texture::Create();
			m_BlueprintIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/audiofile_icon.png";
			m_AudioFileIcon = Texture::Create();
			m_AudioFileIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/glTF_button.png";
			m_glTFIcon = Texture::Create();
			m_glTFIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/animation_icon.png";
			m_AnimationIcon = Texture::Create();
			m_AnimationIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/log_warning_icon.png";
			m_WarningIcon = Texture::Create();
			m_WarningIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/log_error_icon.png";
			m_ErrorIcon = Texture::Create();
			m_ErrorIcon->LoadFromFile(&texCI);

			texCI.FilePath = "assets/buttons/log_info_icon.png";
			m_InfoIcon = Texture::Create();
			m_InfoIcon->LoadFromFile(&texCI);

			s_Instance = this;
		}

		static TexturesLoader* Get() { return s_Instance; }

		Ref<Texture>  m_PlayButton{};
		Ref<Texture>  m_StopButton{};
		Ref<Texture>  m_MoveButton{};
		Ref<Texture>  m_ScaleButton{};
		Ref<Texture>  m_RotateButton{};
		Ref<Texture>  m_SearchButton{};
		Ref<Texture>  m_RemoveButton{};
		Ref<Texture>  m_FolderButton{};
		Ref<Texture>  m_DocumentsIcon{};
		Ref<Texture>  m_SceneIcon{};
		Ref<Texture>  m_glTFIcon{};
		Ref<Texture>  m_BackgroundIcon{};
		Ref<Texture>  m_CubeIcon{};
		Ref<Texture>  m_SphereIcon{};
		Ref<Texture>  m_CapsuleIcon{};
		Ref<Texture>  m_TorusIcon{};
		Ref<Texture>  m_ReturnIcon{};
		Ref<Texture>  m_ForwardIcon{};
		Ref<Texture>  m_UpdateIcon{};
		Ref<Texture>  m_BlueprintIcon{};
		Ref<Texture>  m_AudioFileIcon{};
		Ref<Texture>  m_AnimationIcon{};
		Ref<Texture>  m_WarningIcon{};
		Ref<Texture>  m_InfoIcon{};
		Ref<Texture>  m_ErrorIcon{};
	private:

		inline static TexturesLoader* s_Instance = nullptr;
	};
}