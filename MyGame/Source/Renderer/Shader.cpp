#include "CommonHeaders.h"

#include "Shader.h"

#include "../Core/Timer.h"
#include "../Debugs/Instrumentor.h"
#include "../Debugs/DebugHelpers.h"
#include "../Utilities/Utility.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	bool Shader::CompileVertexShader(ComPtr<IDxcBlob>& blob, const std::string& filePath)
	{
		return CompileFromFile(blob, filePath, "vs_6_6");
	}

	bool Shader::CompilePixelShader(ComPtr<IDxcBlob>& blob, const std::string& filePath)
	{
		return CompileFromFile(blob, filePath, "ps_6_6");
	}

	bool Shader::CompileFromFile(ComPtr<IDxcBlob>& blob, const std::string& filePath, const std::string& shaderProfile)
	{
		MYGAME_INFO("Loading shader: {0} - {1}", filePath, shaderProfile);

		if (!std::filesystem::exists(filePath)) { MYGAME_ERROR("Can't locate shader: " + filePath); return false; }

		ComPtr<IDxcLibrary> library;
		MYGAME_VERIFY_HRESULT(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));

		ComPtr<IDxcCompiler> compiler;
		MYGAME_VERIFY_HRESULT(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

		uint32_t codePage = CP_UTF8;
		std::wstring wfilePath = Utility::UTF8ToWideString(filePath);

		ComPtr<IDxcBlobEncoding> sourceBlob;
		MYGAME_VERIFY_HRESULT(library->CreateBlobFromFile(wfilePath.c_str(), &codePage, &sourceBlob));

		HRESULT hr = S_FALSE;
		std::wstring wshaderProFile = Utility::UTF8ToWideString(shaderProfile);

		ComPtr<IDxcOperationResult> result;
		if (SUCCEEDED(compiler->Compile(sourceBlob.Get(), nullptr, L"main", wshaderProFile.c_str(), NULL, 0, NULL, 0, NULL, &result)))
			result->GetStatus(&hr);
		if (FAILED(hr))
		{
			ComPtr<IDxcBlobEncoding> errorsBlob;
			hr = result->GetErrorBuffer(&errorsBlob);
			if (SUCCEEDED(hr) && errorsBlob)
				MYGAME_ERROR("Shader compilation failed: {0}", (const char*)errorsBlob->GetBufferPointer());
			return false;
		}
		result->GetResult(blob.GetAddressOf());
		return true;
	}

	bool Shader::D3CompileVertexShader(ComPtr<ID3DBlob>& blob, const std::string& filePath)
	{
		return D3CompileFromFile(blob, filePath, "vs_5_1");
	}

	bool Shader::D3CompilePixelShader(ComPtr<ID3DBlob>& blob, const std::string& filePath)
	{
		return D3CompileFromFile(blob, filePath, "ps_5_1");
	}

	bool Shader::D3CompileFromFile(ComPtr<ID3DBlob>& blob, const std::string& filePath, const std::string& shaderProfile)
	{
		MYGAME_INFO("Loading shader: {0} - {1}", filePath, shaderProfile);

#ifdef MYGAME_DEBUG
		UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		std::wstring wfilePath = Utility::UTF8ToWideString(filePath);

		ComPtr<ID3DBlob> errorMsg;
		if (FAILED(D3DCompileFromFile(reinterpret_cast<LPCWSTR>(wfilePath.c_str()), nullptr, nullptr, "main", shaderProfile.c_str(), compileFlags, 0, blob.GetAddressOf(), errorMsg.GetAddressOf())))
		{
			if (errorMsg)
				MYGAME_ERROR("Shader compilation failed: {0}", (const char*)errorMsg->GetBufferPointer());
			return false;
		}
		return true;
	}
}