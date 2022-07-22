workspace "MyGame"
	architecture "x64"
	startproject "MyGame"

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
defaultDirectory = "MyGame"

project "MyGame"
	location "MyGame"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"
	
	targetdir ("%{wks.location}/Binary/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinaryIntermediate/" .. outputdir .. "/%{prj.name}")

	pchheader "CommonHeaders.h"
	pchsource "MyGame/Source/CommonHeaders.cpp"

	files
	{
		"" .. defaultDirectory .. "/Source/**.h",
		"" .. defaultDirectory .. "/Source/**.cpp",		
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	includedirs
	{
		"" .. defaultDirectory .. "",
		"" .. defaultDirectory .. "/Vendor/Box2D",
		"" .. defaultDirectory .. "/Vendor/GLAD/include",
		"" .. defaultDirectory .. "/Vendor/GLFW/include",
		"" .. defaultDirectory .. "/Vendor/GLM",
		"" .. defaultDirectory .. "/Vendor/ImGui",
		"" .. defaultDirectory .. "/Vendor/SpdLog/include"
	}

	links
	{
		"Box2D",
		"GLFW",
		"GLAD",
		"ImGui"
	}

	filter "system:windows"
		cppdialect "C++20"
		staticruntime "On"
		systemversion "latest"

	filter "configurations:Debug"
		defines "MYGAME_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "MYGAME_RELEASE"
		runtime "Release"
		optimize "on"

project "Box2D"
	location "MyGame"
	kind "StaticLib"
	language "C++"
	cppdialect "C++11"
	staticruntime "off"

	targetdir ("%{wks.location}/Binary/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinaryIntermediate/" .. outputdir .. "/%{prj.name}")

	files
	{
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/**.h",
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/**.cpp",
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/include/**.h"
	}

	includedirs
	{
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/include",
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/src"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"

project "GLAD"
	location "MyGame"
    kind "StaticLib"
    language "C"
    staticruntime "off"
    
	targetdir ("%{wks.location}/Binary/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinaryIntermediate/" .. outputdir .. "/%{prj.name}")

    files
    {
        "" .. defaultDirectory .. "/Vendor/%{prj.name}/**.h",
        "" .. defaultDirectory .. "/Vendor/%{prj.name}/**.c"
    }

    includedirs
    {
        "" .. defaultDirectory .. "/Vendor/%{prj.name}/include"
    }
    
    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

project "GLFW"
	location "MyGame"
	kind "StaticLib"
	language "C"

	targetdir ("%{wks.location}/Binary/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinaryIntermediate/" .. outputdir .. "/%{prj.name}")


	files
	{	
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/include/GLFW/glfw3.h",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/include/GLFW/glfw3native.h",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/internal.h",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/platform.h",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/mappings.h",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/context.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/init.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/input.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/monitor.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/platform.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/vulkan.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/window.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/egl_context.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/osmesa_context.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/null_platform.h",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/null_joystick.h",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/null_init.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/null_monitor.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/null_window.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/null_joystick.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/win32_init.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/win32_module.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/win32_joystick.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/win32_monitor.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/win32_time.h",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/win32_time.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/win32_thread.h",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/win32_thread.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/win32_window.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/wgl_context.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/egl_context.c",
    	"" .. defaultDirectory .. "/Vendor/%{prj.name}/src/osmesa_context.c"
	}

	includedirs
	{
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/include"
	}

	filter "system:windows"
		systemversion "latest"
		staticruntime "On"

		defines 
		{ 
			"_GLFW_WIN32",
			"_GLFW_USE_HYBRID_HPG",
			"_CRT_SECURE_NO_WARNINGS"
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

project "ImGui"
	location "MyGame"
	kind "StaticLib"
	language "C++"

	targetdir ("%{wks.location}/Binary/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinaryIntermediate/" .. outputdir .. "/%{prj.name}")

	files
	{
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/imconfig.h",
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/imgui.h",
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/imgui.cpp",
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/imgui_draw.cpp",
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/imgui_internal.h",
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/imgui_widgets.cpp",
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/imstb_rectpack.h",
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/imstb_textedit.h",
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/imstb_truetype.h",
		"" .. defaultDirectory .. "/Vendor/%{prj.name}/imgui_demo.cpp"
	}

	filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"
		staticruntime "On"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

group "Dependencies"
	includedirs "MyGame/Vendor/Box2D"
	includedirs "MyGame/Vendor/GLFW"
	includedirs "MyGame/Vendor/GLAD"
	includedirs "MyGame/Vendor/ImGui"
group ""