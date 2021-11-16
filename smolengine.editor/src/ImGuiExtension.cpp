#include "ImGuiExtension.h"
#include "stdafx.h"

#include <imgui/misc/cpp/imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>

namespace ImGui
{
	float label_pos = 6.0f;
	float item_pos = 130.0f;

	void Extensions::SetLabelPos(float value)
	{
		label_pos = value;
	}

	void Extensions::SetItemPos(float value)
	{
		item_pos = value;
	}

	void Extensions::RestorePos()
	{
		label_pos = 6.0f;
		item_pos = 130.0f;
	}

	void Extensions::TransformComponent(glm::vec3& wordPos, glm::vec3& scale, glm::vec3& rotation)
	{
		InputFloat3Colored("Translation", wordPos);
		InputFloat3Colored("Rotation", rotation);
		InputFloat3Colored("Scale", scale, 1.0f);
	}

	void Extensions::Text(const std::string& label, const std::string& text)
	{
		DrawLabel(label);
		ImGui::TextUnformatted(text.c_str());
	}

	void Extensions::InputFloat3Colored(const std::string& label, glm::vec3& vec3, float reset_value)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 6, 18 });
		std::string label_id = "##Label" + label;
		std::string id = "##IDXXX" + label;
		ImGui::PushID(id.c_str());

		ImGui::Columns(2, label_id.c_str());
		{
			ImGui::SetColumnWidth(0, item_pos);
			ImGui::TextUnformatted(label.c_str());
			ImGui::NextColumn();

			ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());

			float height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImVec2 buttonSize = { height + 3.0f, height };

			//X
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
				if (ImGui::Button("X", buttonSize))
				{
					vec3.x = reset_value;
				}
				ImGui::PopStyleColor(3);
				ImGui::SameLine();

				ImGui::InputFloat("##X", &vec3.x);
				ImGui::PopItemWidth();
				ImGui::SameLine();
			}

			//Y
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
				if (ImGui::Button("Y", buttonSize))
				{
					vec3.y = reset_value;
				}
				ImGui::PopStyleColor(3);
				ImGui::SameLine();

				ImGui::InputFloat("##Y", &vec3.y);
				ImGui::PopItemWidth();
				ImGui::SameLine();
			}

			//Z
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
				if (ImGui::Button("Z", buttonSize))
				{
					vec3.z = reset_value;
				}

				ImGui::PopStyleColor(3);
				ImGui::SameLine();

				ImGui::InputFloat("##Z", &vec3.z);
				ImGui::PopItemWidth();
				ImGui::SameLine();
			}

		}

		ImGui::Columns(1);
		ImGui::PopID();
		ImGui::PopStyleVar();
	}

	void Extensions::InputString(const std::string& label, std::string& output, std::string& dummyStr, const std::string& hint, ImGuiInputTextFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDString" + label;
		if (ImGui::InputTextWithHint(id.c_str(), hint.c_str(), &dummyStr, flags | ImGuiInputTextFlags_EnterReturnsTrue))
		{
			output = dummyStr;
		}
	}

	bool Extensions::InputRawString(const std::string& label, std::string& string, const std::string& hint, ImGuiInputTextFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDRawString" + label;
		return ImGui::InputTextWithHint(id.c_str(), hint.c_str(), &string, flags | ImGuiInputTextFlags_EnterReturnsTrue);
	}

	bool Extensions::ColorInput3(const std::string& label, glm::vec3& color)
	{
		DrawLabel(label);
		std::string id = "##IDColorInput3" + label;
		return ImGui::ColorEdit3(id.c_str(), glm::value_ptr(color));
	}

	bool Extensions::ColorInput4(const std::string& label, glm::vec4& color)
	{
		DrawLabel(label);
		std::string id = "##IDColorInput4" + label;
		return ImGui::ColorEdit4(id.c_str(), glm::value_ptr(color));
	}

	void Extensions::Texture(const std::string& label, void* textureID)
	{
		DrawLabel(label);
		std::string id = "##IDTexture" + label;
		ImGui::PushID(id.c_str());
		ImGui::Image(textureID, ImVec2{ 100, 100 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		ImGui::PopID();
	}

	bool Extensions::CheckBox(const std::string& label, bool& value)
	{
		DrawLabel(label);
		std::string id = "##IDCheckBox" + label;
		return ImGui::Checkbox(id.c_str(), &value);
	}

	bool Extensions::Combo(const std::string& label, const char* comboText, int& value)
	{
		DrawLabel(label);
		std::string id = "##IDCombo" + label;
		return ImGui::Combo(id.c_str(), &value, comboText);
	}

	bool Extensions::SmallButton(const std::string& label, const std::string& buttonText)
	{
		DrawLabel(label);
		return ImGui::SmallButton(buttonText.c_str());
	}

	bool Extensions::Slider(const std::string& label, float& value, float min, float max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDSlider" + label;
		return ImGui::SliderFloat(id.c_str(), &value, min, max, "%.3f", flags);
	}

	bool Extensions::Slider2(const std::string& label, glm::vec2& value, float min, float max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDSlider2" + label;
		return ImGui::SliderFloat2(id.c_str(), glm::value_ptr(value), min, max, "%.3f", flags);
	}

	bool Extensions::Slider3(const std::string& label, glm::vec3& value, float min, float max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDSlider3" + label;
		return ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max, "%.3f", flags);
	}

	bool Extensions::Slider4(const std::string& label, glm::vec4& value, float min, float max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDSlider4" + label;
		return ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max, "%.3f", flags);
	}

	bool Extensions::VerticalSlider(const std::string& label, float& value, glm::vec2& size, float min, float max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDSlider4" + label;
		return ImGui::VSliderFloat(id.c_str(), ImVec2(size.x, size.y), &value, min, max, "%.3f", flags);
	}

	void Extensions::PlotLines(const std::string& label, float* values, uint32_t size)
	{
		DrawLabel(label);
		std::string id = "##IDSlider4" + label;
		ImGui::PlotLines(id.c_str(), values, static_cast<int>(size));
	}

	bool Extensions::DragFloat(const std::string& label, float& value, float speed, float min, float max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDDragFloat" + label;
		return ImGui::DragFloat(id.c_str(), &value, speed, min, max, "%.3f", flags);
	}

	bool Extensions::DragFloat2(const std::string& label, glm::vec2& value, float speed, float min, float max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDDragFloat2" + label;
		return ImGui::DragFloat2(id.c_str(), glm::value_ptr(value), speed, min, max, "%.3f", flags);
	}

	bool Extensions::DragFloat3(const std::string& label, glm::vec3& value, float speed, float min, float max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDDragFloat3" + label;
		return ImGui::DragFloat3(id.c_str(), glm::value_ptr(value), speed, min, max, "%.3f", flags);
	}

	bool Extensions::DragFloat4(const std::string& label, glm::vec4& value, float speed, float min, float max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDDragFloat4" + label;
		return ImGui::DragFloat4(id.c_str(), glm::value_ptr(value), speed, min, max, "%.3f", flags);
	}

	bool Extensions::DragInt(const std::string& label, int& value, float speed, int min, int max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDDragInt" + label;
		return ImGui::DragInt(id.c_str(), &value, speed, min, max, "%d", flags);
	}

	bool Extensions::DragInt2(const std::string& label, glm::ivec2& value, float speed, int min, int max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDDragInt2" + label;
		return ImGui::DragInt2(id.c_str(), glm::value_ptr(value), speed, min, max, "%d", flags);
	}

	bool Extensions::DragInt3(const std::string& label, glm::ivec3& value, float speed, int min, int max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDDragInt3" + label;
		return ImGui::DragInt3(id.c_str(), glm::value_ptr(value), speed, min, max, "%d", flags);
	}

	bool Extensions::DragInt4(const std::string& label, glm::ivec4& value, float speed, int min, int max, ImGuiSliderFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDDragInt4" + label;
		return ImGui::DragInt4(id.c_str(), glm::value_ptr(value), speed, min, max, "%d", flags);
	}

	bool Extensions::InputFloat(const std::string& label, float& value, ImGuiInputTextFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDInputFloat" + label;
		return ImGui::InputFloat(id.c_str(), &value, 1.0f, 10.0f, "%.3f", flags);
	}

	bool Extensions::InputFloat2(const std::string& label, glm::vec2& value, ImGuiInputTextFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDInputFloat2" + label;
		return ImGui::InputFloat2(id.c_str(), glm::value_ptr(value), "%.3f", flags);
	}

	bool Extensions::InputFloat3(const std::string& label, glm::vec3& value, ImGuiInputTextFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDInputFloat3" + label;
		return ImGui::InputFloat3(id.c_str(), glm::value_ptr(value), "%.3f", flags);
	}

	bool Extensions::InputFloat4(const std::string& label, glm::vec4& value, ImGuiInputTextFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDInputFloat4" + label;
		return ImGui::InputFloat4(id.c_str(), glm::value_ptr(value), "%.3f", flags);
	}

	bool Extensions::InputInt(const std::string& label, int& value, ImGuiInputTextFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDInputInt" + label;
		return ImGui::InputInt(id.c_str(), &value, 1, 10, flags);
	}

	bool Extensions::InputInt2(const std::string& label, glm::ivec2& value, ImGuiInputTextFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDInputInt2" + label;
		return ImGui::InputInt2(id.c_str(), glm::value_ptr(value), flags);
	}

	bool Extensions::InputInt3(const std::string& label, glm::ivec3& value, ImGuiInputTextFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDInputInt3" + label;
		return ImGui::InputInt3(id.c_str(), glm::value_ptr(value), flags);
	}

	bool Extensions::InputInt4(const std::string& label, glm::ivec4& value, ImGuiInputTextFlags flags)
	{
		DrawLabel(label);
		std::string id = "##IDInputInt4" + label;
		return ImGui::InputInt4(id.c_str(), glm::value_ptr(value), flags);
	}

	void Extensions::DrawLabel(const std::string& label)
	{
		ImGui::SetCursorPosX(label_pos);
		ImGui::TextWrapped(label.c_str());

		ImGui::SameLine();
		ImGui::SetCursorPosX(item_pos);
	}
}