workspace "Oeuvre"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release"
	}

	startproject "Editor"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "Oeuvre/vendor/GLFW"
	include "Oeuvre/vendor/imgui"
group ""

group "Core"

project "Oeuvre"
	location "Oeuvre"
	kind "StaticLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	nuget 
	{ 
		"directxtk_desktop_win10:2024.10.29.1",
		"NVAPI:1.0.440"
	} 

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{prj.name}/vendor/GLFW/include",
		"%{prj.name}/vendor/imgui",
		"%{prj.name}/vendor/glm",
		"%{prj.name}/vendor/assimp/include",
		"%{prj.name}/vendor/PhysX/include",
		"%{prj.name}/vendor/fmod/include",
	}

	libdirs
	{
		"%{prj.name}/lib"
	}

	links
	{
		"GLFW",
		"imgui",
		"assimp-vc143-mt.lib",
		"GFSDK_VXGI_x64.lib",
		"PhysXExtensions_static_64.lib",
		"PhysXFoundation_64.lib",
		"PhysXPvdSDK_static_64.lib",
		--"PhysXTask_static_64.lib",
		--"PhysXVehicle_static_64.lib",
		--"PhysXVehicle2_static_64.lib",
		"PhysXCommon_64.lib",
		"PhysX_64.lib",
		"PVDRuntime_64.lib",
		"fmod_vc.lib",
		"fmodL_vc.lib"
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"OV_PLATFORM_WINDOWS"
		}

		buildoptions
		{	 
			"/utf-8" 
		}

	filter "configurations:Debug"
		defines "OV_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "OV_RELEASE"
		optimize "On"

group ""

project "Editor"
	location "Editor"
	kind "ConsoleApp"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	nuget 
	{ 
		"directxtk_desktop_win10:2024.10.29.1",
		"directxtex_desktop_win10:2024.10.29.1",
		"NVAPI:1.0.440"
	}

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{	
		"Oeuvre/src",
		"Oeuvre/vendor/GLFW/include",
		"Oeuvre/vendor/spdlog/include",
		"Oeuvre/vendor/imgui",
		"Oeuvre/vendor/glm",
		"Oeuvre/vendor/assimp/include",
		"Oeuvre/vendor/PhysX/include",
		"Oeuvre/vendor/fmod/include"
	}

	links
	{
		"Oeuvre"
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			"OV_PLATFORM_WINDOWS"
		}

		buildoptions
		{	 
			"/utf-8" 
		}


	filter "configurations:Debug"
		defines "OV_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "OV_RELEASE"
		optimize "On"