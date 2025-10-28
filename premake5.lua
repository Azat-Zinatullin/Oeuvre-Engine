workspace "Oeuvre"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release"
	}

	startproject "Editor"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

VULKAN_SDK = os.getenv('VULKAN_SDK')

group "Dependencies"
	include "Oeuvre/vendor/GLFW"
	include "Oeuvre/vendor/imgui"
	include "Oeuvre/vendor/ImGuizmo"
	include "Oeuvre/vendor/yaml-cpp"
	-- include "Oeuvre/vendor/JoltPhysics"
	include "Oeuvre/vendor/vk-bootstrap"
	include "Oeuvre/vendor/Glad"
group ""

group "Core"

project "Oeuvre"
	location "Oeuvre"
	kind "StaticLib"
	language "C++"
	conformancemode "No"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "ovpch.h"
	pchsource "%{prj.name}/src/ovpch.cpp"

	nuget 
	{ 
		"directxtk_desktop_win10:2024.10.29.1",
		"directxtex_desktop_win10:2024.10.29.1",
		"NVAPI:1.0.440",
		"nlohmann.json:3.11.3",
		"Microsoft.Direct3D.D3D12:1.615.1",
		"directxtk12_desktop_win10:2025.3.21.3"
	} 

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/src/**.c"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{prj.name}/vendor/GLFW/include",
		"%{prj.name}/vendor/imgui",
		"%{prj.name}/vendor/ImGuizmo",
		"%{prj.name}/vendor/glm",
		"%{prj.name}/vendor/assimp/include",
		"%{prj.name}/vendor/stb/include",
		"%{prj.name}/vendor/fmod/include",
		"%{prj.name}/vendor/entt/include",
		"%{prj.name}/vendor/mono/include",
		"%{prj.name}/vendor/yaml-cpp/include",
		"%{prj.name}/vendor/HBAOPlus/include",
		"%{prj.name}/vendor/ffx_sdk/include",
		"%{VULKAN_SDK}/Include",
		"%{prj.name}/vendor/VulkanSDK/Include",
		"%{prj.name}/vendor/VulkanMemoryAllocator/include",
		"%{prj.name}/vendor/D3D12MemoryAllocator/include",
		"%{prj.name}/vendor/JoltPhysics/JoltPhysics",
		"%{prj.name}/vendor/shaderc/include",
		"%{prj.name}/vendor/vk-bootstrap/src",
		"%{prj.name}/vendor/Glad/include",
		"%{prj.name}/vendor/imgui-node-editor"
	}

	libdirs
	{
		"%{prj.name}/lib",
		"%{VULKAN_SDK}/Lib"
	}

	links
	{
		"opengl32.lib",
		"vulkan-1.lib",
		"assimp-vc143-mt.lib",
		"GFSDK_SSAO_D3D11.win64.lib",
		"ffx_brixelizer_x64d.lib",
		"GLFW",
		"imgui",
		"imguizmo",
		"yaml-cpp",
		--"JoltPhysics",
		"vk-bootstrap",
		"Glad"
	}

	defines
	{
		"YAML_CPP_STATIC_DEFINE"
	}

	filter "system:windows"
		cppdialect "C++20"
		systemversion "latest"

		defines
		{
			"OV_PLATFORM_WINDOWS"
		}

		buildoptions
		{	 
			"/utf-8" 
		}

		links
		{
			"Ws2_32.lib",
			"Winmm.lib",
			"Version.lib",
			"Bcrypt.lib"
		}

	filter "configurations:Debug"
		defines 
		{
			"_DEBUG",
			"OV_DEBUG",
			"JPH_ENABLE_ASSERTS"
		}
		symbols "On"

		libdirs 
		{
			"%{prj.name}/lib/debug",
			"%{prj.name}/vendor/mono/lib/Debug"
		}

		links
		{
			"fmodL_vc.lib",
			"libmono-static-sgen.lib",
			"shaderc_combined.lib",
			"JoltPhysics.lib"
		}

	filter "configurations:Release"
		defines 
		{
			"NDEBUG",
			"OV_RELEASE"
		}
		optimize "On"

		libdirs 
		{
			"%{prj.name}/lib/release",
			"%{prj.name}/vendor/mono/lib/Release"
		}

		links
		{
			"fmod_vc.lib",
			"libmono-static-sgen.lib",
			"shaderc_combined.lib",
			"JoltPhysics.lib"
		}

	filter { "configurations:Debug or configurations:Release" }
		defines 
		{
			"JPH_DEBUG_RENDERER",
			"JPH_FLOATING_POINT_EXCEPTIONS_ENABLED"
		}

		
include "Oeuvre-ScriptCore"
group ""

project "Editor"
	location "Editor"
	kind "ConsoleApp"
	language "C++"
	conformancemode "No"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	nuget 
	{ 
		"directxtk_desktop_win10:2024.10.29.1",
		"directxtex_desktop_win10:2024.10.29.1",
		"NVAPI:1.0.440",
		"nlohmann.json:3.11.3",
		"Microsoft.Direct3D.D3D12:1.615.1",
		"directxtk12_desktop_win10:2025.3.21.3"
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
		"Oeuvre/vendor/ImGuizmo",
		"Oeuvre/vendor/glm",
		"Oeuvre/vendor/assimp/include",
		"Oeuvre/vendor/stb/include",
		"Oeuvre/vendor/fmod/include",
		"Oeuvre/vendor/entt/include",
		"Oeuvre/vendor/HBAOPlus/include",
		"Oeuvre/vendor/ffx_sdk/include",
		"%{VULKAN_SDK}/Include",
		"Oeuvre/vendor/VulkanSDK/Include",
		"Oeuvre/vendor/VulkanMemoryAllocator/include",
		"Oeuvre/vendor/D3D12MemoryAllocator/include",
		"Oeuvre/vendor/JoltPhysics/JoltPhysics",
		"Oeuvre/vendor/shaderc/include",
		"Oeuvre/vendor/vk-bootstrap/src",
		"Oeuvre/vendor/Glad/include",
		"Oeuvre/vendor/imgui-node-editor"
	}

	links
	{
		"Oeuvre"
	}

	filter "system:windows"
		cppdialect "C++20"
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
		defines 
		{
			"_DEBUG",
			"OV_DEBUG"
		}
		symbols "On"

	filter "configurations:Release"
		defines 
		{
			"NDEBUG",
			"OV_RELEASE"
		}
		optimize "On"