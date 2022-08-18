workspace "MyGame"
	architecture "x64"
	startproject "MyGame"

	systemversion "latest"
	cppdialect "C++latest"

	configurations
	{
		"Debug",
		"Release"
	}

	flags
	{
		"MultiProcessorCompile"
	}	

outputdir = "%{cfg.buildcfg}"

project "MyGame"
	kind "StaticLib"
	location "MyGame"
	language "C++"
	kind "ConsoleApp"
	staticruntime "off"

	targetdir ("%{wks.location}/Binary/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinaryInt/" .. outputdir .. "/%{prj.name}")

	pchheader "CommonHeaders.h"
	pchsource ("%{prj.location}/Source/CommonHeaders.cpp")	

	files
	{
		"%{prj.location}/Source/**.h",
		"%{prj.location}/Source/**.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
	}

	includedirs
	{
		"%{prj.location}/Source",
		"%{prj.location}/Vendor/Box2D",
		"%{prj.location}/Vendor/ImGui",
		"%{prj.location}/Vendor/ImGui/**",
		"%{prj.location}/Vendor/SpdLog/include",
		"%{prj.location}/Vendor/DirectXTK12/Inc",
		"%{prj.location}/Vendor/DirectXTK12/Src",
		"%{prj.location}/Vendor/D3D12MemoryAlloc/include",
	}

	links
	{
		"Box2D",
		"ImGui",
		"DirectXTK_Desktop_2022_Win10",
		"D3D12MemoryAlloc",
		"d3d12.lib",
		"d3dcompiler.lib",
		"dxcompiler.lib",
		"dxgi.lib"
	}

	filter "configurations:Debug"
	runtime "Debug"
		symbols "on"
		defines 
		{
			"MYGAME_DEBUG",
			"MYGAME_USE_DXCOMPILER"
		}		

	filter "configurations:Release"
		runtime "Release"
		optimize "Speed"	
		buildoptions 
		{ 
			"/Ox",
			"/Gm-",
			"/Ob2",
			"/Ot",
			"/GT",
			"/Oy-"
		}	
		flags
		{
			"LinkTimeOptimization"
		}

project "Box2D"
	location "MyGame"
	kind "StaticLib"
	staticruntime "off"	

	targetdir ("%{wks.location}/Binary/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinaryInt/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.location}/Vendor/%{prj.name}/src/**.h",
		"%{prj.location}/Vendor/%{prj.name}/src/**.cpp",
		"%{prj.location}/Vendor/%{prj.name}/include/**.h"
	}

	includedirs
	{
		"%{prj.location}/Vendor/%{prj.name}/**",
	}

project "ImGui"
	location "MyGame"
	kind "StaticLib"
	language "C++"

	targetdir ("%{wks.location}/Binary/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinaryInt/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.location}/Vendor/%{prj.name}/imconfig.h",
		"%{prj.location}/Vendor/%{prj.name}/imgui.h",
		"%{prj.location}/Vendor/%{prj.name}/imgui.cpp",
		"%{prj.location}/Vendor/%{prj.name}/imgui_draw.cpp",
		"%{prj.location}/Vendor/%{prj.name}/imgui_tables.h",
		"%{prj.location}/Vendor/%{prj.name}/imgui_internal.h",
		"%{prj.location}/Vendor/%{prj.name}/imgui_widgets.cpp",
		"%{prj.location}/Vendor/%{prj.name}/imstb_rectpack.h",
		"%{prj.location}/Vendor/%{prj.name}/imstb_textedit.h",
		"%{prj.location}/Vendor/%{prj.name}/imstb_truetype.h",
		"%{prj.location}/Vendor/%{prj.name}/imgui_demo.cpp"
	}

	includedirs
	{
		"%{prj.location}/Vendor/%{prj.name}/**",
	}

externalproject ("DirectXTK_Desktop_2022_Win10")
 	location ("%{wks.name}/Vendor/DirectXTK12")
	uuid "3E0E8608-CD9B-4C76-AF33-29CA38F2C9F0"
	kind "StaticLib"

	buildoptions 
	{ 
		"/D_WIN32_WINNT=0x0A00",
	}

project "D3D12MemoryAlloc"
	location "MyGame"
	kind "StaticLib"
	language "C++"

	targetdir ("%{wks.location}/Binary/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinaryInt/" .. outputdir .. "/%{prj.name}")	

	files
	{		
		"%{prj.location}/Vendor/%{prj.name}/src/D3D12MemAlloc.cpp"
	}

	includedirs
	{
		"%{prj.location}/Vendor/%{prj.name}/**",
	}