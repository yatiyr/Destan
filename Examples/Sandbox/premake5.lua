project "DestanSandbox"
    location "%{wks.location}/Examples/Sandbox"
    kind "ConsoleApp"
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
        "src/**.h",
        "src/**.cpp",
        "src/**.hpp"
    }

    includedirs {
        "%{wks.location}/Engine/EngineModule/include"
    }

    links {
        "EngineModule"
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