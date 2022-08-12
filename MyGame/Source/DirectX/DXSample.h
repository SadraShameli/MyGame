#pragma once

#include <d3d12.h>
#include <dxgi.h>
#include <string>

namespace MyGame
{
	class DXSample
	{
	public:
		DXSample(UINT width, UINT height, std::wstring name);
		virtual ~DXSample();

		virtual void OnInit() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;
		virtual void OnSizeChanged(UINT width, UINT height, bool minimized) = 0;
		virtual void OnDestroy() = 0;

		virtual void OnKeyDown(UINT8) {}
		virtual void OnKeyUp(UINT8) {}
		virtual void OnWindowMoved(int, int) {}
		virtual void OnMouseMove(UINT, UINT) {}
		virtual void OnLeftButtonDown(UINT, UINT) {}
		virtual void OnLeftButtonUp(UINT, UINT) {}
		virtual void OnDisplayChanged() {}

		UINT GetWidth() const { return m_width; }
		UINT GetHeight() const { return m_height; }
		const WCHAR* GetTitle() const { return m_title.c_str(); }
		bool GetTearingSupport() const { return m_tearingSupport; }
		RECT GetWindowsBounds() const { return m_windowBounds; }
		virtual IDXGISwapChain* GetSwapchain() { return nullptr; }

		void UpdateForSizeChange(UINT clientWidth, UINT clientHeight);
		void SetWindowBounds(int left, int top, int right, int bottom);
		std::wstring GetAssetFullPath(LPCWSTR assetName);

	protected:
		void GetHardwareAdapter(_In_ IDXGIFactory1* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter = false);
		void CheckTearingSupport();
		void SetCustomWindowText(LPCWSTR text);

		RECT m_windowBounds;
		UINT m_width;
		UINT m_height;
		float m_aspectRatio;

		bool m_tearingSupport;
		bool m_useWarpDevice;
		bool m_enableUI;

	private:
		std::wstring m_assetsPath;
		std::wstring m_title;
	};
}