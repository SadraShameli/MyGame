#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <future>

namespace MyGame
{
	namespace FileManager
	{
		using ByteArray = std::vector<uint8_t>;
		static constexpr size_t MAX_FILEPATH = 260;

		template <typename T>
		inline ByteArray ReadBinary(const T& fileName)
		{
			std::ifstream file(fileName, std::ios::binary | std::ios::ate);
			size_t fileSize = file.tellg();

			ByteArray data;
			data.resize(fileSize);

			file.clear();
			file.seekg(0);
			file.read((char*)data.data(), fileSize);

			return data;
		}

		template <typename T>
		inline std::future<ByteArray> ReadBinaryAsync(const T& fileName)
		{
			return std::async(std::launch::async, ReadBinary, fileName);
		}

		template <typename T>
		inline std::string ReadText(const T& fileName)
		{
			std::ifstream file(fileName, std::ios::ate);
			size_t fileSize = file.tellg();

			std::string data;
			data.resize(fileSize);
			data.clear();

			file.clear();
			file.seekg(0);
			file.read((char*)data.data(), fileSize);

			return data;
		}

		template <typename T>
		inline std::future<std::string> ReadTextAsync(const T& fileName)
		{
			return std::async(std::launch::async, ReadText, fileName);
		}

		inline std::string GetBasePath(const std::string& filePath)
		{
			size_t lastSlash;
			if ((lastSlash = filePath.rfind('/')) != std::string::npos)
				return filePath.substr(0, lastSlash + 1);
			else if ((lastSlash = filePath.rfind('\\')) != std::string::npos)
				return filePath.substr(0, lastSlash + 1);
			else
				return "";
		}

		inline std::wstring GetBasePath(const std::wstring& filePath)
		{
			size_t lastSlash;
			if ((lastSlash = filePath.rfind(L'/')) != std::wstring::npos)
				return filePath.substr(0, lastSlash + 1);
			else if ((lastSlash = filePath.rfind(L'\\')) != std::wstring::npos)
				return filePath.substr(0, lastSlash + 1);
			else
				return L"";
		}

		inline std::string RemoveBasePath(const std::string& filePath)
		{
			size_t lastSlash;
			if ((lastSlash = filePath.rfind('/')) != std::string::npos)
				return filePath.substr(lastSlash + 1, std::string::npos);
			else if ((lastSlash = filePath.rfind('\\')) != std::string::npos)
				return filePath.substr(lastSlash + 1, std::string::npos);
			else
				return filePath;
		}

		inline std::wstring RemoveBasePath(const std::wstring& filePath)
		{
			size_t lastSlash;
			if ((lastSlash = filePath.rfind(L'/')) != std::string::npos)
				return filePath.substr(lastSlash + 1, std::string::npos);
			else if ((lastSlash = filePath.rfind(L'\\')) != std::string::npos)
				return filePath.substr(lastSlash + 1, std::string::npos);
			else
				return filePath;
		}

		inline std::string GetFileExtension(const std::string& filePath)
		{
			std::string fileName = RemoveBasePath(filePath);
			size_t extOffset = fileName.rfind('.');
			if (extOffset == std::wstring::npos)
				return "";

			return fileName.substr(extOffset + 1);
		}

		inline std::wstring GetFileExtension(const std::wstring& filePath)
		{
			std::wstring fileName = RemoveBasePath(filePath);
			size_t extOffset = fileName.rfind(L'.');
			if (extOffset == std::wstring::npos)
				return L"";

			return fileName.substr(extOffset + 1);
		}

		inline std::string RemoveExtension(const std::string& filePath)
		{
			return filePath.substr(0, filePath.rfind("."));
		}

		inline std::wstring RemoveExtension(const std::wstring& filePath)
		{
			return filePath.substr(0, filePath.rfind(L"."));
		}
	}
}