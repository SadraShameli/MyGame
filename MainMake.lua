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

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"

	filter { }

	flags
	{
		"MultiProcessorCompile"		
	}	

	staticruntime "On"
	optimize "Speed"		

outputdir = "%{cfg.buildcfg}"

project "MyGame"
	location "MyGame"
	kind "ConsoleApp"
	language "C++"
	
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
		"GLFW_EXPOSE_NATIVE_WIN32",
	}

	includedirs
	{
		"%{prj.location}/Source",
		"%{prj.location}/Vendor/Box2D",
		"%{prj.location}/Vendor/GLFW/include",
		"%{prj.location}/Vendor/GLM",
		"%{prj.location}/Vendor/ImGui",
		"%{prj.location}/Vendor/SpdLog/include",
		"%{prj.location}/Vendor/DirectXTK12/Inc",
		"%{prj.location}/Vendor/DirectXTK12/Src",
		"%{prj.location}/Vendor/D3D12MemoryAlloc/include",
	}

	links
	{
		"Box2D",
		"GLFW",
		"ImGui",
		"DirectXTK12",
		"D3D12MemoryAlloc",
		"d3d12.lib",
		"d3dcompiler.lib",
		"dxcompiler.lib",
		"dxgi.lib"
	}

	filter "configurations:Debug"
		defines 
		{
			"MYGAME_DEBUG"			
		}		

--	filter { "files:**.hlsl" }
--   		flags "ExcludeFromBuild"
--   		shadermodel "6.5"
--
--	filter { "files:**Pixel.hlsl" }
--   		removeflags "ExcludeFromBuild"
--   		shadertype "Pixel"
--   		shaderentry "ForPixel"
--
--	filter { "files:**Vertex.hlsl" }
--   		removeflags "ExcludeFromBuild"
--   		shadertype "Vertex"
--   		shaderentry "ForVertex"			

	buildoptions 
	{ 
		"/Gm-",
		"/Ob2",
		"/Ot",
		"/GL",
		"/GT",
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

project "GLFW"
	location "MyGame"
	kind "StaticLib"
	language "C"

	targetdir ("%{wks.location}/Binary/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinaryInt/" .. outputdir .. "/%{prj.name}")
	
	defines 
	{ 
		"_GLFW_WIN32",
		"_GLFW_USE_HYBRID_HPG",
		"_CRT_SECURE_NO_WARNINGS"
	}

	files
	{	
		"%{prj.location}/Vendor/%{prj.name}/include/GLFW/glfw3.h",
    	"%{prj.location}/Vendor/%{prj.name}/include/GLFW/glfw3native.h",
    	"%{prj.location}/Vendor/%{prj.name}/src/internal.h",
    	"%{prj.location}/Vendor/%{prj.name}/src/platform.h",
    	"%{prj.location}/Vendor/%{prj.name}/src/mappings.h",
    	"%{prj.location}/Vendor/%{prj.name}/src/context.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/init.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/input.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/monitor.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/platform.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/vulkan.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/window.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/egl_context.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/osmesa_context.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/null_platform.h",
    	"%{prj.location}/Vendor/%{prj.name}/src/null_joystick.h",
    	"%{prj.location}/Vendor/%{prj.name}/src/null_init.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/null_monitor.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/null_window.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/null_joystick.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/win32_init.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/win32_module.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/win32_joystick.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/win32_monitor.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/win32_time.h",
    	"%{prj.location}/Vendor/%{prj.name}/src/win32_time.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/win32_thread.h",
    	"%{prj.location}/Vendor/%{prj.name}/src/win32_thread.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/win32_window.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/wgl_context.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/egl_context.c",
    	"%{prj.location}/Vendor/%{prj.name}/src/osmesa_context.c"
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

externalproject ("DirectXTK_Windows10_2022")
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