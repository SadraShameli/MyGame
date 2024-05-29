#pragma once

#include "../DirectX/CommonIncludes.h"

#include <string>

namespace MyGame
{
	class Shader
	{
	public:
		static HRESULT CompileVertexShader(IDxcBlob** Blob, const std::wstring& FilePath);
		static HRESULT CompilePixelShader(IDxcBlob** Blob, const std::wstring& FilePath);
		static HRESULT D3CompileVertexShader(ID3DBlob** Blob, const std::wstring& FilePath);
		static HRESULT D3CompilePixelShader(ID3DBlob** Blob, const std::wstring& FilePath);

		static HRESULT CompileFromFile(IDxcBlob** Blob, const std::wstring& FilePath, const std::wstring& ShaderProfile, const std::wstring& MainEntry);
		static HRESULT D3CompileFromFile(ID3DBlob** Blob, const std::wstring& FilePath, const std::string& ShaderProfile, const std::string& MainEntry);
	};
}
