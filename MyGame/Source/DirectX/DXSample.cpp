#include "CommonHeaders.h"

#include "DXSample.h"
#include "DirectXHelpers.h"

#include "../Core/Application.h"

using namespace Microsoft::WRL;

namespace MyGame
{
	DXSample::DXSample(UINT width, UINT height, std::wstring name) :
		m_width(width),
		m_height(height),
		m_windowBounds{ 0, 0, 0, 0 },
		m_title(name),
		m_aspectRatio(0.0f),
		m_useWarpDevice(false),
		m_enableUI(true)
	{
		WCHAR assetsPath[512];
		GetAssetsPath(assetsPath, _countof(assetsPath));
		m_assetsPath = assetsPath;

		UpdateForSizeChange(width, height);
		CheckTearingSupport();
	}

	DXSample::~DXSample() {}

	void DXSample::UpdateForSizeChange(UINT clientWidth, UINT clientHeight)
	{
		m_width = clientWidth;
		m_height = clientHeight;
		m_aspectRatio = static_cast<float>(clientWidth) / static_cast<float>(clientHeight);
	}

	std::wstring DXSample::GetAssetFullPath(LPCWSTR assetName) { return m_assetsPath + assetName; }

	_Use_decl_annotations_
		void DXSample::GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
	{
		*ppAdapter = nullptr;
		ComPtr<IDXGIAdapter1> adapter;
		ComPtr<IDXGIFactory6> factory6;

		if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
		{
			for (UINT adapterIndex = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(adapterIndex, requestHighPerformanceAdapter == true ?
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&adapter))); ++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr))) break;
			}
		}

		if (adapter.Get() == nullptr)
		{
			for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) break;
			}
		}
		*ppAdapter = adapter.Detach();
	}

	void DXSample::SetCustomWindowText(LPCWSTR text) { SetWindowText(Application::Get().GetNativeWindow(), text); }

	void DXSample::CheckTearingSupport()
	{
		ComPtr<IDXGIFactory6> factory;
		HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
		BOOL allowTearing = FALSE;
		if (SUCCEEDED(hr))
			hr = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
		m_tearingSupport = SUCCEEDED(hr) && allowTearing;
	}

	void DXSample::SetWindowBounds(int left, int top, int right, int bottom)
	{
		m_windowBounds.left = static_cast<LONG>(left);
		m_windowBounds.top = static_cast<LONG>(top);
		m_windowBounds.right = static_cast<LONG>(right);
		m_windowBounds.bottom = static_cast<LONG>(bottom);
	}
}