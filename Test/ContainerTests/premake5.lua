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
        "%{wks.location}/Test/TestFramework/include",
        "%{wks.location}/Engine/Core/include"
    }

    links
    {
        "Core",
        "TestFramework"
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