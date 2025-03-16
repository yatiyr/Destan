project "Core"
    location "%{wks.location}/Engine/Core"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    staticruntime "off"

    targetdir("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir("%{wks.location}/obj/" .. outputdir .. "/%{prj.name}")

    files
    {
        "include/**.h",
        "include/**.hpp",
        "include/**.inl",
        "src/**.cpp",
        "src/**.c",
        "src/**.hpp"
    }

    includedirs
    {
        "include"
    }

    links
    {
        -- Add links here
    }

    -- Define precompiled header for C++ files only
    filter "files:src/**.cpp"
        pchheader "core/destan_pch.h"
        pchsource "src/destan_pch.cpp"

    -- Explicitly disable PCH for header files
    filter "files:**.h or **.hpp or **.inl"
        flags { "NoPCH" }
        
    -- Reset filter for subsequent rules
    filter {}    

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