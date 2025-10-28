project "imguizmo"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
        --"GraphEditor.h",
        --"GraphEditor.cpp",
        --"ImCurveEdit.h",
        --"ImCurveEdit.cpp",
        --"ImGradient.h",
        --"ImGradient.cpp",
        "ImGuizmo.h",
        "ImGuizmo.cpp"
	}

	includedirs
	{
		"./",
		"../imgui"
	}

	filter "system:linux"
		pic "On"
		systemversion "latest"
		staticruntime "Off"

	filter "system:windows"
		systemversion "latest"
		staticruntime "Off"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"