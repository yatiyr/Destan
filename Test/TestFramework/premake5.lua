project "TestFramework"
    location "%{wks.location}/Test/TestFramework"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "include/**.h",
        "include/**.hpp",
        "src/**.cpp",
        "src/**.c",
        "src/**.hpp"
    }

    includedirs
    {
        "include",
        "%{wks.location}/Engine/Core/include"
    }

    links
    {
        "Core"
    }

    filter "system:windows"
        systemversion "latest"
        defines
        {
            "DESTAN_PLATFORM_WINDOWS"
        }

    filter "system:linux"
        defines
        {
            "DESTAN_PLATFORM_LINUX"
        }
        
        links
        {
            "pthread"
        }

    filter "system:macosx"
        defines
        {
            "DESTAN_PLATFORM_MACOS"
        }