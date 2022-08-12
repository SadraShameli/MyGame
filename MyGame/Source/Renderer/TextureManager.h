#pragma once

#include "Texture.h"

namespace MyGame
{
	class ManagedTexture;

	class TextureRef
	{
	public:
		TextureRef(const TextureRef& ref);
		TextureRef(ManagedTexture* tex = nullptr);
		~TextureRef();

		void operator= (nullptr_t);
		void operator= (TextureRef& rhs);

		bool IsValid() const;

		D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const;

		const Texture* Get() const;
		const Texture* operator->() const;

	private:
		ManagedTexture* m_ref;
	};

	namespace TextureManager
	{
		void Initialize(const std::string& RootPath);
		void Shutdown();

		TextureRef LoadDDSFromFile(const std::string& filePath, Texture::DefaultTexture fallback = Texture::kMagenta2D, bool sRGB = false);
	}
}