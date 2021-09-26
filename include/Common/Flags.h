#pragma once

namespace SmolEngine
{
	enum class ShaderType : int
	{
		Vertex = 1,
		Fragment = 2,
		Compute = 4,
		Geometry = 8,

		RayGen = 16,
		RayMiss = 32,
		RayHit = 64,
	};

	inline ShaderType operator~ (ShaderType a) { return (ShaderType)~(int)a; }
	inline ShaderType operator| (ShaderType a, ShaderType b) { return (ShaderType)((int)a | (int)b); }
	inline ShaderType operator& (ShaderType a, ShaderType b) { return (ShaderType)((int)a & (int)b); }
	inline ShaderType operator^ (ShaderType a, ShaderType b) { return (ShaderType)((int)a ^ (int)b); }
	inline ShaderType& operator|= (ShaderType& a, ShaderType b) { return (ShaderType&)((int&)a |= (int)b); }
	inline ShaderType& operator&= (ShaderType& a, ShaderType b) { return (ShaderType&)((int&)a &= (int)b); }
	inline ShaderType& operator^= (ShaderType& a, ShaderType b) { return (ShaderType&)((int&)a ^= (int)b); }

	enum class FeaturesFlags: int
	{
		Imgui             = 1,
		RendererDebug     = 2,
	};

	inline FeaturesFlags operator~ (FeaturesFlags a) { return (FeaturesFlags)~(int)a; }
	inline FeaturesFlags operator| (FeaturesFlags a, FeaturesFlags b) { return (FeaturesFlags)((int)a | (int)b); }
	inline FeaturesFlags operator& (FeaturesFlags a, FeaturesFlags b) { return (FeaturesFlags)((int)a & (int)b); }
	inline FeaturesFlags operator^ (FeaturesFlags a, FeaturesFlags b) { return (FeaturesFlags)((int)a ^ (int)b); }
	inline FeaturesFlags& operator|= (FeaturesFlags& a, FeaturesFlags b) { return (FeaturesFlags&)((int&)a |= (int)b); }
	inline FeaturesFlags& operator&= (FeaturesFlags& a, FeaturesFlags b) { return (FeaturesFlags&)((int&)a &= (int)b); }
	inline FeaturesFlags& operator^= (FeaturesFlags& a, FeaturesFlags b) { return (FeaturesFlags&)((int&)a ^= (int)b); }
}