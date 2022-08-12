#include "CommonHeaders.h"

#include "Texture.h"
#include "TextureManager.h"
#include "DirectXHelpers.h"

#include "../DirectX/CommandContext.h"
#include "../DirectX/DirectXImpl.h"
#include "../Utilities/Utility.h"
#include "../Utilities/FileManager.h"

using namespace DirectX;

namespace MyGame
{
	class ManagedTexture : public Texture
	{
		friend class TextureRef;

	public:
		ManagedTexture(const std::string& FileName);

		void WaitForLoad(void) const;
		void CreateFromMemory(std::shared_ptr<std::vector<byte>> memory, Texture::DefaultTexture fallback, bool sRGB);

	private:
		bool IsValid(void) const { return m_IsValid; }
		void Unload();

		std::string m_MapKey;
		bool m_IsValid;
		bool m_IsLoading;
		size_t m_ReferenceCount;
	};

	namespace TextureManager
	{
		std::mutex s_Mutex;
		std::string s_RootPath;
		std::map<std::string, std::unique_ptr<ManagedTexture>> s_TextureCache;

		void Initialize(const std::string& TextureLibRoot) { s_RootPath = TextureLibRoot; }
		void Shutdown() { s_TextureCache.clear(); }

		ManagedTexture* FindOrLoadTexture(const std::string& fileName, Texture::DefaultTexture fallback, bool forceSRGB)
		{
			ManagedTexture* tex = nullptr;
			{
				std::lock_guard<std::mutex> Guard(s_Mutex);

				std::string key = fileName;
				if (forceSRGB)
					key += "_sRGB";

				auto iter = s_TextureCache.find(key);
				if (iter != s_TextureCache.end())
				{
					tex = iter->second.get();
					tex->WaitForLoad();
					return tex;
				}
				else
				{
					tex = new ManagedTexture(key);
					s_TextureCache[key].reset(tex);
				}
			}

			std::shared_ptr<std::vector<byte>> memory = Utility::ReadFileHelper(s_RootPath + fileName);
			tex->CreateFromMemory(memory, fallback, forceSRGB);

			return tex;
		}

		void DestroyTexture(const std::string& key)
		{
			std::lock_guard<std::mutex> Guard(s_Mutex);

			auto iter = s_TextureCache.find(key);
			if (iter != s_TextureCache.end())
				s_TextureCache.erase(iter);
		}
	}

	ManagedTexture::ManagedTexture(const std::string& FileName) : m_MapKey(FileName), m_IsValid(false), m_IsLoading(true), m_ReferenceCount(0)
	{
		m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	void ManagedTexture::CreateFromMemory(std::shared_ptr<std::vector<byte>> memory, Texture::DefaultTexture fallback, bool forceSRGB)
	{
		if (!memory->size())
			m_hCpuDescriptorHandle = GetDefaultTexture(fallback);
		else
		{
			m_hCpuDescriptorHandle = DirectXImpl::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			/*if (SUCCEEDED(CreateDDSTextureFromMemory(DirectXImpl::D12Device.Get(), (const uint8_t*)memory->data(), memory->size(),
				0, forceSRGB, m_pResource.GetAddressOf(), m_hCpuDescriptorHandle)))
			{
				m_IsValid = true;
				D3D12_RESOURCE_DESC desc = GetResource()->GetDesc();
				m_Width = (uint32_t)desc.Width;
				m_Height = desc.Height;
				m_Depth = desc.DepthOrArraySize;
			}
			else
			{
				DirectXImpl::D12Device->CopyDescriptorsSimple(1, m_hCpuDescriptorHandle, GetDefaultTexture(fallback),
					D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}*/
		}

		m_IsLoading = false;
	}

	void ManagedTexture::WaitForLoad() const
	{
		while ((volatile bool&)m_IsLoading)
			std::this_thread::yield();
	}

	void ManagedTexture::Unload() { TextureManager::DestroyTexture(m_MapKey); }

	TextureRef::TextureRef(const TextureRef& ref) : m_ref(ref.m_ref)
	{
		if (m_ref != nullptr)
			++m_ref->m_ReferenceCount;
	}

	TextureRef::TextureRef(ManagedTexture* tex) : m_ref(tex)
	{
		if (m_ref != nullptr)
			++m_ref->m_ReferenceCount;
	}

	TextureRef::~TextureRef()
	{
		if (m_ref != nullptr && --m_ref->m_ReferenceCount == 0)
			m_ref->Unload();
	}

	void TextureRef::operator= (std::nullptr_t)
	{
		if (m_ref != nullptr)
			--m_ref->m_ReferenceCount;

		m_ref = nullptr;
	}

	void TextureRef::operator= (TextureRef& rhs)
	{
		if (m_ref != nullptr)
			--m_ref->m_ReferenceCount;

		m_ref = rhs.m_ref;

		if (m_ref != nullptr)
			++m_ref->m_ReferenceCount;
	}

	bool TextureRef::IsValid() const { return m_ref && m_ref->IsValid(); }

	const Texture* TextureRef::Get() const { return m_ref; }

	const Texture* TextureRef::operator->() const
	{
		MYGAME_ASSERT(m_ref != nullptr);
		return m_ref;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE TextureRef::GetSRV() const
	{
		if (m_ref != nullptr)
			return m_ref->GetSRV();
		else
			return GetDefaultTexture(Texture::kMagenta2D);
	}

	TextureRef TextureManager::LoadDDSFromFile(const std::string& filePath, Texture::DefaultTexture fallback, bool forceSRGB)
	{
		return TextureManager::FindOrLoadTexture(filePath, fallback, forceSRGB);
	}
}