#pragma once
#include "Graphics/GUI/UIStyle.h"

#include <glm/glm.hpp>
#include <functional>

namespace SmolEngine
{
	struct nk_context;

	enum class UICanvasFlags: int
	{
		Invalid            = 1,
		Borders            = 2,
		Minimizable        = 16,
		Closable           = 32,
		NoInput            = 64,
		NoSrollbar         = 124,
	};

	inline UICanvasFlags operator~ (UICanvasFlags a) { return (UICanvasFlags)~(int)a; }
	inline UICanvasFlags operator| (UICanvasFlags a,UICanvasFlags b) { return (UICanvasFlags)((int)a | (int)b); }
	inline UICanvasFlags operator& (UICanvasFlags a,UICanvasFlags b) { return (UICanvasFlags)((int)a & (int)b); }
	inline UICanvasFlags operator^ (UICanvasFlags a,UICanvasFlags b) { return (UICanvasFlags)((int)a ^ (int)b); }
	inline UICanvasFlags& operator|= (UICanvasFlags& a, UICanvasFlags b) { return (UICanvasFlags&)((int&)a |= (int)b); }
	inline UICanvasFlags& operator&= (UICanvasFlags& a, UICanvasFlags b) { return (UICanvasFlags&)((int&)a &= (int)b); }
	inline UICanvasFlags& operator^= (UICanvasFlags& a, UICanvasFlags b) { return (UICanvasFlags&)((int&)a ^= (int)b); }

	class UICanvas
	{
	public:
		UICanvas();

		void Draw(const std::function<void()>& func);

		UIStyle        Style{};
		UICanvasFlags  eFlags = UICanvasFlags::Invalid;
		std::string    Title = "";
		glm::vec4      Rect = glm::vec4(50, 50, 250, 280);
	private:
		nk_context*    pContext = nullptr;
	};
}