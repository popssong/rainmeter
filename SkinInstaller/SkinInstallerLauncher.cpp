/*
  Copyright (C) 2011 Birunthan Mohanathas

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

typedef int (*SkinInstallerMainFunc)(LPWSTR cmdLine);

WCHAR* GetCommandLineArguments()
{
	WCHAR* args = GetCommandLine();

	// Skip past (quoted) application path in cmdLine.
	if (*args == L'"')
	{
		++args;  // Skip leading quote.
		while (*args && *args != L'"')
		{
			++args;
		}
		++args;  // Skip trailing quote.
	}
	else
	{
		while (*args && *args != L' ')
		{
			++args;
		}
	}

	// Skip leading whitespace (similar to CRT implementation).
	while (*args && *args <= L' ')
	{
		++args;
	}

	return args;
}

/*
** Attempts to load SkinInstaller.dll. If it fails, retries after loading our own copies of the
** CRT DLLs in the Runtime directory.
*/
HINSTANCE LoadSkinInstallerLibrary()
{
	HINSTANCE rmDll = LoadLibrary(L"SkinInstaller.dll");
	if (!rmDll)
	{
		WCHAR path[MAX_PATH];
		if (GetModuleFileName(nullptr, path, MAX_PATH) > 0)
		{
			PathRemoveFileSpec(path);
			PathAppend(path, L"Runtime");
			SetDllDirectory(path);
			PathAppend(path, L"msvcp120.dll");

			// Loading msvcpNNN.dll will load msvcrNNN.dll as well.
			HINSTANCE msvcrDll = LoadLibrary(path);
			SetDllDirectory(L"");

			if (msvcrDll)
			{
				rmDll = LoadLibrary(L"SkinInstaller.dll");
				FreeLibrary(msvcrDll);
			}
		}
	}

	return rmDll;
}

/*
** Entry point. In Release builds, the entry point is Main() since the CRT is not used.
**
*/
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	// Prevent system error message boxes.
	UINT oldMode = SetErrorMode(0);
	SetErrorMode(oldMode | SEM_FAILCRITICALERRORS);

	WCHAR* args = GetCommandLineArguments();

	HINSTANCE skinInstallerDll = LoadSkinInstallerLibrary();
	if (skinInstallerDll)
	{
		auto skinInstallerMain =
			(SkinInstallerMainFunc)GetProcAddress(skinInstallerDll, MAKEINTRESOURCEA(1));
		if (skinInstallerMain)
		{
			return skinInstallerMain(args);
		}
	}

	WCHAR message[128];
	wsprintf(
		message,
		L"SkinInstaller.dll load error %ld.",
		GetLastError());
	MessageBox(nullptr, message, L"Skin Installer", MB_OK | MB_ICONERROR);

	return 1;
}

#ifndef _DEBUG
EXTERN_C int WINAPI Main()
{
	int result = wWinMain(nullptr, nullptr, nullptr, 0);
	ExitProcess(result);
	return 0;  // Never reached.
}
#endif
