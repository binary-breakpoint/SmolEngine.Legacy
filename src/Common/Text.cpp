#include "stdafx.h"
#include "Common/Text.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
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
			if (!istream.good() || out_text->m_SDFTexture.IsReady())
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

			TextureCreateInfo createCI = {};
			createCI.bVerticalFlip = false;
			createCI.FilePath = texturePath;
			Texture::Create(&createCI, &out_text->m_SDFTexture);
		}
	}

	void Text::SetText(const std::string& text)
	{
		m_Text = text;
		GenerateText(text);
	}

	void Text::SetColor(const glm::vec4& color)
	{
		m_Color = color;
	}

	void Text::SetPosition(const glm::vec2& pos)
	{
		m_Pos = { pos, 1.0f };
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
		m_Size = size;
		if(!m_Text.empty())
			SetText(m_Text);
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

		float w = static_cast<float>(m_SDFTexture.GetInfo().Width);

		float posx = 0.0f;
		float posy = 0.0f;

		for (uint32_t i = 0; i < text.size(); i++)
		{
			bmchar* charInfo = &m_FontChars[(int)text[i]];

			if (charInfo->width == 0)
				charInfo->width = static_cast<uint32_t>(m_Size);

			float charw = ((float)(charInfo->width) / m_Size);
			float dimx = 1.0f * charw;
			float charh = ((float)(charInfo->height) / m_Size);
			float dimy = 1.0f * charh;

			float us = charInfo->x / w;
			float ue = (charInfo->x + charInfo->width) / w;
			float ts = charInfo->y / w;
			float te = (charInfo->y + charInfo->height) / w;

			float xo = charInfo->xoffset / m_Size;
			float yo = charInfo->yoffset / m_Size;

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

			float advance = ((float)(charInfo->xadvance) / m_Size);
			posx += advance;
		}
		m_IndexCount = static_cast<uint32_t>(indices.size());

		// Center
		for (auto& v : vertices)
		{
			v.Pos[0] -= posx / 2.0f;
			v.Pos[1] -= 0.5f;
			// flip
			v.Pos = v.Pos * glm::vec3(1.0f, -1.0f, 1.0f);
		}

		// Generate host accessible buffers for the text vertices and indices and upload the data
		if (m_VertexBuffer.IsReady())
		{
			m_VertexBuffer = {};
			m_IndexBuffer = {};
		}

		VertexBuffer::Create(&m_VertexBuffer, vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(TextVertex)), false);
		IndexBuffer::Create(&m_IndexBuffer, indices.data(), static_cast<uint32_t>(indices.size()), false);
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