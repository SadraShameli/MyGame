#pragma once

#include "../Core/Application.h"
#include "FileManager.h"

#include <commdlg.h>

#include <string>
#include <optional>

namespace MyGame
{
	namespace Utility
	{
		inline static std::optional<std::string> OpenFile(const char* filter)
		{
			OPENFILENAMEA ofn;
			CHAR szFile[260] = {};
			CHAR currentDir[256] = {};
			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = Application::Get().GetNativeWindow();
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			if (GetCurrentDirectoryA(256, currentDir))
				ofn.lpstrInitialDir = currentDir;
			ofn.lpstrFilter = filter;
			ofn.nFilterIndex = 1;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

			if (GetOpenFileNameA(&ofn) == TRUE)
				return ofn.lpstrFile;

			return {};
		}

		inline static std::optional<std::string> SaveFile(const char* filter)
		{
			OPENFILENAMEA ofn;
			CHAR szFile[260] = {};
			CHAR currentDir[256] = {};
			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = Application::Get().GetNativeWindow();
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			if (GetCurrentDirectoryA(256, currentDir))
				ofn.lpstrInitialDir = currentDir;
			ofn.lpstrFilter = filter;
			ofn.nFilterIndex = 1;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
			ofn.lpstrDefExt = strchr(filter, '\0') + 1;

			if (GetSaveFileNameA(&ofn) == TRUE)
				return ofn.lpstrFile;

			return {};
		}

		inline static std::shared_ptr<std::vector<byte>> ReadFileHelper(const std::string& fileName)
		{
			struct _stat64 fileStat;
			int fileExists = _wstat64(Utility::UTF8ToWideString(fileName).c_str(), &fileStat);
			if (fileExists == -1)
				return std::make_shared<std::vector<byte>>(std::vector<byte>());

			std::ifstream file(fileName, std::ios::in | std::ios::binary);
			if (!file)
				return std::make_shared<std::vector<byte>>(std::vector<byte>());

			std::shared_ptr<std::vector<byte>> byteArray = std::make_shared<std::vector<byte>>(fileStat.st_size);
			file.read((char*)byteArray->data(), byteArray->size());
			file.close();

			return byteArray;
		}
	}
}