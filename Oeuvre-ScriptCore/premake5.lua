project "Oeuvre-ScriptCore"
    location "Oeuvre-ScriptCore"
    kind "SharedLib"
    language "C#"
    dotnetframework "4.7.2"
    namespace "Oeuvre"

    targetdir ("%{wks.location}/resources/scripts")
    objdir ("%{wks.location}/resources/scripts/intermediate")

    files
    {
        "Oeuvre-ScriptCore/Source/**.cs",
        "Oeuvre-ScriptCore/Properties/**.cs"
    }

    filter "configurations:Debug"
        optimize "Off"
        symbols "Default"

    filter "configurations:Release"
        optimize "On"
        symbols "Off"