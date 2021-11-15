#pragma once

#include <glm/glm.hpp>
#include <string>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace ImGui
{
	class Extensions
	{
	public:

		static void SetLabelPos(float value);
		static void SetItemPos(float value);
		static void RestorePos();

		static void TransformComponent(glm::vec3& wordPos, glm::vec3& scale, glm::vec3& rotation);
		static void InputFloat3Colored(const std::string& label, glm::vec3& vec3, float reset_value = 0.0f);

		static void Text(const std::string& label, const std::string& text);
		static void InputString(const std::string& label, std::string& output, std::string& dummyStr,  const std::string& hint = "Name", ImGuiInputTextFlags flags = 0);
		static bool InputRawString(const std::string& label, std::string& string, const std::string& hint = "", ImGuiInputTextFlags flags = 0);

		static bool ColorInput3(const std::string& label, glm::vec3& color);
		static bool ColorInput4(const std::string& label, glm::vec4& color);
		static void Texture(const std::string& label, void* textureID);
		static bool CheckBox(const std::string& label, bool& value);
		static bool Combo(const std::string& label, const char* comboText, int& value);
		static bool SmallButton(const std::string& label, const std::string& buttonText);

		static bool Slider(const std::string& label, float& value, float min, float max, ImGuiSliderFlags flags = 0);
		static bool Slider2(const std::string& label, glm::vec2& value, float min, float max, ImGuiSliderFlags flags = 0);
		static bool Slider3(const std::string& label, glm::vec3& value, float min, float max, ImGuiSliderFlags flags = 0);
		static bool Slider4(const std::string& label, glm::vec4& value, float min, float max, ImGuiSliderFlags flags = 0);

		static bool VerticalSlider(const std::string& label, float& value, glm::vec2& size, float min, float max, ImGuiSliderFlags flags = 0);
		static void PlotLines(const std::string& label, float* values, uint32_t size);

		static bool DragFloat(const std::string& label, float& value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, ImGuiSliderFlags flags = 0);
		static bool DragFloat2(const std::string& label, glm::vec2& value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, ImGuiSliderFlags flags = 0);
		static bool DragFloat3(const std::string& label, glm::vec3& value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, ImGuiSliderFlags flags = 0);
		static bool DragFloat4(const std::string& label, glm::vec4& value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, ImGuiSliderFlags flags = 0);

		static bool DragInt(const std::string& label, int& value, float speed = 1.0f, int min = 0, int max = 0, ImGuiSliderFlags flags = 0);
		static bool DragInt2(const std::string& label, glm::ivec2& value, float speed = 1.0f, int min = 0, int max = 0, ImGuiSliderFlags flags = 0);
		static bool DragInt3(const std::string& label, glm::ivec3& value, float speed = 1.0f, int min = 0, int max = 0, ImGuiSliderFlags flags = 0);
		static bool DragInt4(const std::string& label, glm::ivec4& value, float speed = 1.0f, int min = 0, int max = 0, ImGuiSliderFlags flags = 0);

		static bool InputFloat(const std::string& label, float& value, ImGuiInputTextFlags flags = 0);
		static bool InputFloat2(const std::string& label, glm::vec2& value, ImGuiInputTextFlags flags = 0);
		static bool InputFloat3(const std::string& label, glm::vec3& value, ImGuiInputTextFlags flags = 0);
		static bool InputFloat4(const std::string& label, glm::vec4& value, ImGuiInputTextFlags flags = 0);

		static bool InputInt(const std::string& label, int& value, ImGuiInputTextFlags flags = 0);
		static bool InputInt2(const std::string& label, glm::ivec2& value, ImGuiInputTextFlags flags = 0);
		static bool InputInt3(const std::string& label, glm::ivec3& value, ImGuiInputTextFlags flags = 0);
		static bool InputInt4(const std::string& label, glm::ivec4& value, ImGuiInputTextFlags flags = 0);

		static void DrawLabel(const std::string& label);
	};
}