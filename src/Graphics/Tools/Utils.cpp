#include "stdafx.h"
#include "Graphics/Tools/Utils.h"
#include "Graphics/Window/Input.h"
#include "Graphics/GraphicsContext.h"

#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <GLFW/glfw3.h>

namespace SmolEngine
{
	bool Utils::DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
	{
		// From glm::decompose in matrix_decompose.inl

		using namespace glm;
		using T = float;

		mat4 LocalMatrix(transform);

		// Normalize the matrix.
		if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
			return false;

		// First, isolate perspective.  This is the messiest.
		if (
			epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
		{
			// Clear the perspective partition
			LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
			LocalMatrix[3][3] = static_cast<T>(1);
		}

		// Next take care of translation (easy).
		translation = vec3(LocalMatrix[3]);
		LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

		vec3 Row[3], Pdum3 = {};

		// Now get scale and shear.
		for (length_t i = 0; i < 3; ++i)
			for (length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		// Compute X scale factor and normalize first row.
		scale.x = length(Row[0]);
		Row[0] = detail::scale(Row[0], static_cast<T>(1));
		scale.y = length(Row[1]);
		Row[1] = detail::scale(Row[1], static_cast<T>(1));
		scale.z = length(Row[2]);
		Row[2] = detail::scale(Row[2], static_cast<T>(1));

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
		}
#endif

		rotation.y = asin(-Row[0][2]);
		if (cos(rotation.y) != 0) {
			rotation.x = atan2(Row[1][2], Row[2][2]);
			rotation.z = atan2(Row[0][1], Row[0][0]);
		}
		else {
			rotation.x = atan2(-Row[2][0], Row[1][1]);
			rotation.z = 0;
		}


		return true;
	}

	bool Utils::ComposeTransform(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale, glm::mat4& out_transform)
	{
		glm::mat4 rot = rotate(glm::mat4(1.0f), rotation.x, { 1, 0, 0 }) *
			rotate(glm::mat4(1.0f), rotation.y, { 0, 1, 0 }) *
			rotate(glm::mat4(1.0f), rotation.z, { 0, 0, 1 });

		out_transform = glm::translate(glm::mat4(1.0f), translation) *
			rot * glm::scale(glm::mat4(1.0f), { scale });
		return true;
	}

	bool Utils::ComposeTransform2D(const glm::vec2& translation, const glm::vec2& rotation, const glm::vec2& scale, glm::mat4& out_transform)
	{
		glm::mat4 rot = rot = rotate(glm::mat4(1.0f), rotation.x, { 0, 0, 1.0f });
		out_transform = glm::translate(glm::mat4(1.0f), { translation, 0 }) *
			rot * glm::scale(glm::mat4(1.0f), { scale, 0 });

		return true;
	}

	std::optional<std::string> Utils::OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		Window* win = GraphicsContext::GetSingleton()->GetWindow();
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)win->GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return std::nullopt;
	}

	std::optional<std::string> Utils::SaveFile(const char* filter, const std::string& initialName)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		if (!initialName.empty())
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(initialName.size()); ++i)
			{
				szFile[i] = initialName[i];
			}
		}

		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		Window* win = GraphicsContext::GetSingleton()->GetWindow();
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)win->GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		// Sets the default extension by extracting it from the filter
		ofn.lpstrDefExt = strchr(filter, '\0') + 1;

		if (GetSaveFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return std::nullopt;
	}

	bool Utils::IsPathValid(const std::string& path)
	{
		return std::filesystem::exists(path);
	}

	glm::vec3 Utils::ScreenToWorld(const glm::vec2& mousePos, float width, float height, const glm::mat4& viewProj)
	{
		// these positions must be in range [-1, 1] (!!!), not [0, width] and [0, height]
		float mouseX = mousePos.x / (width * 0.5f) - 1.0f;
		float mouseY = mousePos.y / (height * 0.5f) - 1.0f;

		glm::mat4 invVP = glm::inverse(viewProj);
		glm::vec4 screenPos = glm::vec4(mouseX, -mouseY, 1.0f, 1.0f);
		glm::vec4 worldPos = invVP * screenPos;
		return glm::normalize(glm::vec3(worldPos));
	}

	glm::vec3 Utils::CastRay(const glm::vec3& startPos, const glm::vec2& mousePos, float width, float height, float distance, const glm::mat4& viewProj)
	{
		glm::vec3 rayDir = ScreenToWorld(mousePos, width, height, viewProj);
		return startPos + rayDir * distance;
	}

	glm::vec3 Utils::CastRay(const glm::vec3& startPos, float distance, const glm::mat4& viewProj)
	{
		auto data = GraphicsContext::GetSingleton()->GetWindow()->GetWindowData();
		float w = static_cast<float>(data->Width);
		float h = static_cast<float>(data->Height);
		glm::vec2 mousePos = { Input::GetMouseX(), Input::GetMouseY() };

		return CastRay(startPos, mousePos, w, h, distance, viewProj);
	}

	std::string Utils::GetCachedPath(const std::string& filePath, CachedPathType type)
	{
		std::filesystem::path p = filePath;
		std::filesystem::path path;
		std::filesystem::path dir;

		switch (type)
		{
		case CachedPathType::Shader:

			dir = p.parent_path() / "SPIRV";
			if (!std::filesystem::exists(dir))
				std::filesystem::create_directories(dir);

			path = dir / (p.filename().string() + ".spirv");
			break;

		case CachedPathType::Pipeline:

			dir = p.parent_path() / "PipelineCache";
			if (!std::filesystem::exists(dir))
				std::filesystem::create_directories(dir);

			path = dir / (p.filename().string() + ".pipeline_cached");
			break;
		}

		return path.string();
	}
}