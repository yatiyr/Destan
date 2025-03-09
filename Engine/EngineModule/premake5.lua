project "EngineModule"
    location "%{wks.location}/Engine/EngineModule"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/obj/" .. outputdir .. "/%{prj.name}")

    -- Use Core's PCH
    externalincludedirs {
        "%{wks.location}/Engine/Core/include"
    }
    
    pchheader "core/destan_pch.h"
    pchsource "%{wks.location}/Engine/Core/src/destan_pch.cpp"

    files {
        "include/**.h",
        "include/**.hpp",
        "src/**.cpp",
        "src/**.c",
        "src/**.hpp"
    }

    includedirs {
        "include",
        "%{wks.location}/Engine/Core/include",
        "%{wks.location}/Engine/Physics/include",
        "%{wks.location}/Engine/Rendering/include",
        "%{wks.location}/Engine/Scripting/include"
    }

    links {
        "Core",
        "Physics",
        "Rendering",
        "Scripting"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "DESTAN_PLATFORM_WINDOWS",
            "_CRT_SECURE_NO_WARNINGS"
        }

    filter "system:linux"
        defines
        {
            "DESTAN_PLATFORM_LINUX"
        }

        links
        {
            "dl",
            "pthread"
        }

    filter "system:macosx"
        defines
        {
            "DESTAN_PLATFORM_MACOS"
        }