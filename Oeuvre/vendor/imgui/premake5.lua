project "imgui"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"imconfig.h",
		"imgui.h",
		"imgui_internal.h",
		"imstb_rectpack.h",
		"imstb_textedit.h",
		"imstb_truetype.h",
		"backends/imgui_impl_dx11.h",
		"backends/imgui_impl_win32.h",
		"backends/imgui_impl_glfw.h",
		"misc/cpp/imgui_stdlib.h",
		"imgui.cpp",
		"imgui_demo.cpp",
		"imgui_draw.cpp",
		"imgui_tables.cpp",
		"imgui_widgets.cpp",
		"backends/imgui_impl_dx11.cpp",
		"backends/imgui_impl_win32.cpp",
		"backends/imgui_impl_glfw.cpp",
		"misc/cpp/imgui_stdlib.cpp",
	}

	includedirs
	{
		"./",
		"%{wks.location}/Oeuvre/vendor/GLFW/include"
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