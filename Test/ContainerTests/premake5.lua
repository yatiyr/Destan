project "ContainerTests"
    location "%{wks.location}/Test/ContainerTests"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/**.h",
        "src/**.cpp",
        "src/**.hpp",
        "main.cpp"
    }

    includedirs
    {
        "include",
        "%{wks.location}/Test/TestFramework/include",
        "%{wks.location}/Engine/Core/include"
    }

    links
    {
        "Core",
        "TestFramework"
    }

    -- Define precompiled header for C++ files only
    filter "files:src/**.cpp"
        pchheader "core/destan_pch.h"
        pchsource "%{wks.location}/Engine/Core/src/destan_pch.cpp"

    -- Explicitly disable PCH for header files
    filter "files:**.h or **.hpp or **.inl"
        flags { "NoPCH" }
        
    -- Reset filter for subsequent rules
    filter {}  

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