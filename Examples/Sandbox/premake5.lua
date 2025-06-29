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

    files {
        "include/**.h",
        "include/**.hpp",
        "include/**.inl",
        "src/**.cpp",
        "src/**.c",
        "src/**.hpp"
    }

    includedirs {
        "%{wks.location}/Engine/EngineModule/include"
    }

    links {
        "EngineModule"
    }

    -- Define precompiled header for C++ files only
    filter "files:src/**.cpp"
        pchheader "core/ds_pch.h"
        pchsource "%{wks.location}/Engine/Core/src/ds_pch.cpp"

    -- Explicitly disable PCH for header files
    filter "files:**.h or **.hpp or **.inl"
        flags { "NoPCH" }
        
    -- Reset filter for subsequent rules
    filter {}    

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "DS_PLATFORM_WINDOWS",
            "_CRT_SECURE_NO_WARNINGS"
        }

    filter "system:linux"
        defines
        {
            "DS_PLATFORM_LINUX"
        }

        links
        {
            "dl",
            "pthread"
        }

    filter "system:macosx"
        defines
        {
            "DS_PLATFORM_MACOS"
        }