#include "stdafx.h"
#include "Common/Text.h"

namespace Frostium
{
	Ref<Text> Text::CreateSDF(const std::string& sdfPath, const std::string& texturePath)
	{
		Ref<Text> text = std::make_shared<Text>();
		text->CreateSDF(sdfPath, texturePath, text.get());
		return text;
	}

	void Text::CreateSDF(const std::string& sdfPath, const std::string& texturePath, Text* out_text)
	{
		if (out_text)
		{
			std::filebuf fileBuffer;
			fileBuffer.open(sdfPath, std::ios::in);
			std::istream istream(&fileBuffer);
			if (!istream.good() || out_text->m_Initialized)
				return;

			while (!istream.eof())
			{
				std::string line;
				std::stringstream lineStream;
				std::getline(istream, line);
				lineStream << line;

				std::string info;
				lineStream >> info;

				if (info == "char")
				{
					// char id
					uint32_t charid = out_text->NextValuePair(&lineStream);
					// Char properties
					out_text->m_FontChars[charid].x = out_text->NextValuePair(&lineStream);
					out_text->m_FontChars[charid].y = out_text->NextValuePair(&lineStream);
					out_text->m_FontChars[charid].width = out_text->NextValuePair(&lineStream);
					out_text->m_FontChars[charid].height = out_text->NextValuePair(&lineStream);
					out_text->m_FontChars[charid].xoffset = out_text->NextValuePair(&lineStream);
					out_text->m_FontChars[charid].yoffset = out_text->NextValuePair(&lineStream);
					out_text->m_FontChars[charid].xadvance = out_text->NextValuePair(&lineStream);
					out_text->m_FontChars[charid].page = out_text->NextValuePair(&lineStream);
				}
			}

			Texture::Create(texturePath, &out_text->m_SDFTexture, TextureFormat::R8G8B8A8_UNORM);
			out_text->m_Initialized = true;
		}
	}

	void Text::SetText(const std::string& text)
	{
		if (m_Initialized)
		{
			m_Text = text;
			GenerateText(text);
		}
	}

	void Text::SetColor(const glm::vec4& color)
	{
		m_Color = color;
	}

	void Text::SetPosition(const glm::vec3& pos)
	{
		m_Pos = pos;
	}

	void Text::SetScale(const glm::vec2& scale)
	{
		m_Scale = { scale, 1 };
	}

	void Text::SetRotation(float rot)
	{
		m_Rotation = { rot, rot, 0 };
	}

	void Text::SetSize(float size)
	{

	}

	const std::string& Text::GetText() const
	{
		return m_Text;
	}

	void Text::GenerateText(const std::string& text)
	{
		std::vector<TextVertex> vertices;
		std::vector<uint32_t> indices;
		uint32_t indexOffset = 0;

		float w = static_cast<float>(m_SDFTexture.GetWidth());

		float posx = 0.0f;
		float posy = 0.0f;

		for (uint32_t i = 0; i < text.size(); i++)
		{
			bmchar* charInfo = &m_FontChars[(int)text[i]];

			if (charInfo->width == 0)
				charInfo->width = 36;

			float charw = ((float)(charInfo->width) / 36.0f);
			float dimx = 1.0f * charw;
			float charh = ((float)(charInfo->height) / 36.0f);
			float dimy = 1.0f * charh;

			float us = charInfo->x / w;
			float ue = (charInfo->x + charInfo->width) / w;
			float ts = charInfo->y / w;
			float te = (charInfo->y + charInfo->height) / w;

			float xo = charInfo->xoffset / 36.0f;
			float yo = charInfo->yoffset / 36.0f;

			posy = yo;

			vertices.push_back({ { posx + dimx + xo,  posy + dimy, 0.0f }, { ue, te } });
			vertices.push_back({ { posx + xo,         posy + dimy, 0.0f }, { us, te } });
			vertices.push_back({ { posx + xo,         posy,        0.0f }, { us, ts } });
			vertices.push_back({ { posx + dimx + xo,  posy,        0.0f }, { ue, ts } });

			std::array<uint32_t, 6> letterIndices = { 0,1,2, 2,3,0 };
			for (auto& index : letterIndices)
			{
				indices.push_back(indexOffset + index);
			}
			indexOffset += 4;

			float advance = ((float)(charInfo->xadvance) / 36.0f);
			posx += advance;
		}
		m_IndexCount = static_cast<uint32_t>(indices.size());

		// Center
		for (auto& v : vertices)
		{
			v.Pos[0] -= posx / 2.0f;
			v.Pos[1] -= 0.5f;
		}

		// Generate host accessible buffers for the text vertices and indices and upload the data
#ifdef FROSTIUM_OPENGL_IMPL
#else
		m_VulkanText.Update(vertices, indices);
#endif
	}

	int32_t Text::NextValuePair(std::stringstream* stream)
    {
		std::string pair;
		*stream >> pair;
		size_t spos = pair.find("=");
		std::string value = pair.substr(spos + 1);
		int32_t val = std::stoi(value);
		return val;
    }
}