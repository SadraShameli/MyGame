#pragma once

#include "GpuBuffer.h"

namespace MyGame
{
	class ReadbackBuffer : public GpuBuffer
	{
	public:
		virtual ~ReadbackBuffer() { Destroy(); }

		void Create(const std::wstring_view& name, uint32_t NumElements, uint32_t ElementSize);

		void* Map();
		void Unmap();

	protected:
		void CreateDerivedViews() {}
	};
}