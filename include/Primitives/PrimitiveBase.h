#pragma once

class PrimitiveBase
{
public:
	virtual ~PrimitiveBase() = default;

	virtual void Free() {};
	virtual bool IsGood() const { return false; };

	template<typename T>
	T* Cast() { return dynamic_cast<T*>(this); }
};