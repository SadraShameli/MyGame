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

project "MyGame"
	location "MyGame"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"
	
	targetdir ("%{wks.location}/Binary/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinaryIntermediate/" .. outputdir .. "/%{prj.name}")

	pchheader "CommonHeaders.h"
	pchsource "MyGame/Source/CommonHeaders.cpp"

	files
	{
		"%{prj.name}/Source/**.h",
		"%{prj.name}/Source/**.cpp",		
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	includedirs
	{
		"%{prj.name}/",
		"%{prj.name}/Vendor/Box2D",
		"%{prj.name}/Vendor/GLAD/include",
		"%{prj.name}/Vendor/GLFW/include",
		"%{prj.name}/Vendor/GLM",
		"%{prj.name}/Vendor/ImGui",
		"%{prj.name}/Vendor/SpdLog/include"
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
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/include/**.h"
	}

	includedirs
	{
		"%{prj.name}/include",
		"%{prj.name}/src"
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
        "%{prj.name}/**.h",
        "%{prj.name}/**.c"
    }

    includedirs
    {
        "%{prj.name}/include"
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
		"%{prj.name}/include/*.h",
		"%{prj.name}/src/*.c" 
	}

	includedirs
	{
		"%{prj.name}/include"
	}

	filter "system:windows"
		systemversion "latest"
		staticruntime "On"

		defines 
		{ 
			"_GLFW_WIN64",
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
		"%{prj.name}/*.h",
		"%{prj.name}/*.cpp" 
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