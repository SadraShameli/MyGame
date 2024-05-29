#include "CommonHeaders.h"

#include "TextureManager.h"
#include "../Utilities/FileManager.h"

#include "ResourceUploadBatch.h"

namespace MyGame
{
	static std::wstring s_RootPath;
	static std::map<std::wstring, std::unique_ptr<ManagedTexture>> s_TextureCache;
	static std::mutex s_Mutex;

	ManagedTexture* TextureManager::FindOrLoadTexture(const std::wstring& fileName, bool forceSRGB)
	{
		std::lock_guard<std::mutex> lock(s_Mutex);
		std::wstring key = fileName;
		ManagedTexture* tex = nullptr;

		if (forceSRGB)
			key += L"_sRGB";

		auto iter = s_TextureCache.find(key);
		if (iter != s_TextureCache.end())
		{
			tex = iter->second.get();
			tex->WaitForLoad();
			return tex;
		}
		else
		{
			tex = new ManagedTexture(key, forceSRGB);
			s_TextureCache[key].reset(tex);
		}

		return tex;
	}

	void TextureManager::DestroyTexture(const std::wstring& key)
	{
		std::lock_guard<std::mutex> lock(s_Mutex);

		auto iter = s_TextureCache.find(key);
		if (iter != s_TextureCache.end())
			s_TextureCache.erase(iter);
	}

	ManagedTexture::ManagedTexture(const std::wstring& fileName, bool sRGB)
		: m_Filename(fileName), m_IsValid(false), m_IsLoading(true), m_ReferenceCount(0)
	{
		FileManager::ByteArray data = FileManager::ReadBinary(fileName);
		CreateDDSFromMemory(data.data(), data.size(), sRGB);
	}

	void ManagedTexture::WaitForLoad()
	{
		while ((volatile bool&)m_IsLoading)
			std::this_thread::yield();
	}

	void ManagedTexture::Unload()
	{
		TextureManager::DestroyTexture(m_Filename);
	}

	void TextureManager::Initialize(const std::wstring& rootPath)
	{
		s_RootPath = rootPath;
	}

	void TextureManager::Shutdown()
	{
		s_TextureCache.clear();
	}
}