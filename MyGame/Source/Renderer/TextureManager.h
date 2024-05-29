#pragma once

#include "Texture.h"
#include <DescriptorHeap.h>

#include "../DirectX/DescriptorHeap.h"

namespace MyGame
{
	class ManagedTexture : public Texture
	{
		friend class TextureRef;

	public:
		ManagedTexture(const std::wstring& filename, bool sRGB);

		void WaitForLoad();

	private:
		bool IsValid() { return m_IsValid; }
		void Unload();

		std::wstring m_Filename;
		bool m_IsValid;
		bool m_IsLoading;
		size_t m_ReferenceCount;
	};

	class TextureManager
	{
	public:
		enum DefaultTexture
		{
			Magenta2D,
			BlackOpaque2D,
			BlackTransparent2D,
			WhiteOpaque2D,
			WhiteTransparent2D,
			DefaultNormalMap,
			BlackCubeMap,
			NumDefaultTextures
		};

		static void Initialize(const std::wstring& rootPath);
		static void Shutdown();

		static void DestroyTexture(const std::wstring& key);
		static ManagedTexture* FindOrLoadTexture(const std::wstring& fileName, bool forceSRGB);
	};
}