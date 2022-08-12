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
	IDxcBlob* Shader::CompileVertexShader(const std::wstring& filePath)
	{
		return CompileFromFile(filePath, L"vs_6_6");
	}

	IDxcBlob* Shader::CompilePixelShader(const std::wstring& filePath)
	{
		return CompileFromFile(filePath, L"ps_6_6");
	}

	ID3DBlob* Shader::D3CompileVertexShader(const std::wstring& filePath)
	{
		return D3CompileFromFile(filePath, "vs_5_1");
	}

	ID3DBlob* Shader::D3CompilePixelShader(const std::wstring& filePath)
	{
		return D3CompileFromFile(filePath, "ps_5_1");
	}

	IDxcBlob* Shader::CompileFromFile(const std::wstring& filePath, const std::wstring& shaderProfile)
	{
		MYGAME_INFO(L"Loading shader: {0} - {1}", filePath, shaderProfile);
		if (!std::filesystem::exists(filePath)) { MYGAME_ERROR(L"Can't locate shader: {0}", filePath); return nullptr; }

		IDxcLibrary* library = nullptr;
		MYGAME_VERIFY_HRESULT(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));

		IDxcCompiler* compiler = nullptr;
		MYGAME_VERIFY_HRESULT(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

		uint32_t codePage = CP_UTF8;
		IDxcBlobEncoding* sourceBlob = nullptr;
		MYGAME_VERIFY_HRESULT(library->CreateBlobFromFile(filePath.c_str(), &codePage, &sourceBlob));

		HRESULT hr = S_FALSE;
		IDxcOperationResult* result = nullptr;
		if (SUCCEEDED(compiler->Compile(sourceBlob, nullptr, L"main", shaderProfile.c_str(), NULL, 0, NULL, 0, NULL, &result)))
			result->GetStatus(&hr);
		if (FAILED(hr))
		{
			IDxcBlobEncoding* errorsBlob = nullptr;
			hr = result->GetErrorBuffer(&errorsBlob);
			if (SUCCEEDED(hr) && errorsBlob)
				MYGAME_ERROR("Shader compilation failed: {0}", (const char*)errorsBlob->GetBufferPointer());
			return nullptr;
		}
		IDxcBlob* blob = nullptr;
		result->GetResult(&blob);
		return blob;
	}

	ID3DBlob* Shader::D3CompileFromFile(const std::wstring& filePath, const std::string& shaderProfile)
	{
		MYGAME_INFO("Loading shader: {0} - {1}", Utility::WideStringToUTF8(filePath), shaderProfile);
		if (!std::filesystem::exists(filePath)) { MYGAME_ERROR(L"Can't locate shader: {0}", filePath); return nullptr; }
#ifdef MYGAME_DEBUG
		UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ID3DBlob* errorMsg = nullptr;
		ID3DBlob* blob = nullptr;
		if (FAILED(D3DCompileFromFile(filePath.c_str(), nullptr, nullptr, "main", shaderProfile.c_str(), compileFlags, 0, &blob, &errorMsg)))
		{
			if (errorMsg)
				MYGAME_ERROR("Shader compilation failed: {0}", (const char*)errorMsg->GetBufferPointer());
		}
		return blob;
	}
}