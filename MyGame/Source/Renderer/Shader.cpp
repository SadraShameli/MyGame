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
	HRESULT Shader::CompileVertexShader(IDxcBlob** Blob, const std::wstring& FilePath)
	{
		return CompileFromFile(Blob, FilePath, L"vs_6_2");
	}

	HRESULT Shader::CompilePixelShader(IDxcBlob** Blob, const std::wstring& FilePath)
	{
		return CompileFromFile(Blob, FilePath, L"ps_6_2");
	}

	HRESULT Shader::D3CompileVertexShader(ID3DBlob** Blob, const std::wstring& FilePath)
	{
		return D3CompileFromFile(Blob, FilePath, "vs_5_1");
	}

	HRESULT Shader::D3CompilePixelShader(ID3DBlob** Blob, const std::wstring& FilePath)
	{
		return D3CompileFromFile(Blob, FilePath, "ps_5_1");
	}

	HRESULT Shader::CompileFromFile(IDxcBlob** Blob, const std::wstring& FilePath, const std::wstring& ShaderProfile, const std::wstring& MainEntry)
	{
		MYGAME_INFO(L"Shader: Loading Source at: {0} - Entry: {1} - Profile: {2}", FilePath, MainEntry, ShaderProfile);

		if (!std::filesystem::exists(FilePath)) { MYGAME_ERROR(L"Shader: Failed to Locate Sourcefile at: {0}", FilePath); return S_FALSE; }

		IDxcLibrary* library = nullptr;
		MYGAME_HRESULT_VERIFY(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));

		IDxcCompiler* compiler = nullptr;
		MYGAME_HRESULT_VERIFY(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

		uint32_t codePage = CP_UTF8;
		IDxcBlobEncoding* sourceBlob;
		MYGAME_HRESULT_VERIFY(library->CreateBlobFromFile(FilePath.c_str(), &codePage, &sourceBlob));

		HRESULT hr = S_FALSE;
		IDxcOperationResult* result;

		if (SUCCEEDED(compiler->Compile(sourceBlob, nullptr, MainEntry.c_str(), ShaderProfile.c_str(), nullptr, 0, nullptr, 0, nullptr, &result)))
			result->GetStatus(&hr);
		if (FAILED(hr))
		{
			IDxcBlobEncoding* errorsBlob;
			if (SUCCEEDED(result->GetErrorBuffer(&errorsBlob)) && errorsBlob)
				MYGAME_ERROR("Shader: Failed to Compile: {0}", (const char*)errorsBlob->GetBufferPointer());
			return hr;
		}

		result->GetResult(Blob);
		return hr;
	}

	HRESULT Shader::D3CompileFromFile(ID3DBlob** Blob, const std::wstring& FilePath, const std::string& ShaderProfile, const std::string& MainEntry)
	{
		MYGAME_INFO("Shader: Loading Source at: {0} - Entry: {1} - Profile: {2}", Utility::WideStringToUTF8(FilePath), MainEntry, ShaderProfile);

		if (!std::filesystem::exists(FilePath)) { MYGAME_ERROR(L"Shader: Failed to Locate Sourcefile at: {0}", FilePath); return S_FALSE; }

#ifdef MYGAME_DEBUG
		UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		HRESULT hr = S_FALSE;
		ComPtr<ID3DBlob> errorMsg;

		if (FAILED(hr = D3DCompileFromFile(FilePath.c_str(), nullptr, nullptr, MainEntry.c_str(), ShaderProfile.c_str(), compileFlags, 0, Blob, &errorMsg)))
		{
			if (errorMsg)
				MYGAME_ERROR("Shader: Failed to Compile {0}", (const char*)errorMsg->GetBufferPointer());
		}
		return hr;
	}
}