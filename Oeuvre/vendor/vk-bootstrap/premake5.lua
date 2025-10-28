project "vk-bootstrap"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp",
		"src/**.inl"
	}

	includedirs
	{
		"%{VULKAN_SDK}/Include"
	}

	filter "system:linux"
		pic "On"

		systemversion "latest"
		--staticruntime "On"

	filter "system:windows"
		systemversion "latest"
		--staticruntime "On"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"