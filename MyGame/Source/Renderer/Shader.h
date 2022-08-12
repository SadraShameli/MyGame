#pragma once

#include <string>

namespace MyGame
{
	class Shader
	{
	public:
		static IDxcBlob* CompileVertexShader(const std::wstring&);
		static IDxcBlob* CompilePixelShader(const std::wstring&);

		static IDxcBlob* CompileFromFile(const std::wstring&, const std::wstring&);

		static ID3DBlob* D3CompileVertexShader(const std::wstring&);
		static ID3DBlob* D3CompilePixelShader(const std::wstring&);

		static ID3DBlob* D3CompileFromFile(const std::wstring&, const std::string&);
	};
}
