-- require "cmake"
include "DsDeps.lua"

workspace "Ds"
    architecture "x64"
    startproject "DsSandbox"

    configurations
    {
        "Debug",
        "StrictDebug",
        "Release",
        "Shipping"
    }

    flags
    {
        "MultiProcessorCompile"
    }

-- Output directories for binaries and intermediate files
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"


-- Global Defines based on configuration
filter "configurations:Debug"
    defines { "DS_DEBUG" }
    runtime "Debug"
    symbols "On"
    optimize "Off"

filter "configurations:StrictDebug"
    defines { "DS_DEBUG", "DS_STRICT_DEBUG" }
    runtime "Debug"
    symbols "On"
    optimize "Off"
    warnings "Extra"
    fatalwarnings { "All"}

filter "configurations:Release"
    defines { "DS_RELEASE" }
    runtime "Release"
    optimize "On"
    optimize "On"

filter "configurations:Shipping"
    defines { "DS_SHIPPING"}
    runtime "Release"
    symbols "Off"
    optimize "Full"

filter {}

-- Include Engine Modules
group "Engine"
    include "Engine/Core"
    include "Engine/Physics"
    include "Engine/Rendering"
    include "Engine/Scripting"
    include "Engine/EngineModule"
group ""


-- Include Test projects
group "Tests"
    include "Test/TestFramework"
    include "Test/MemoryTests"
    include "Test/ContainerTests"
    include "Test/LoggerTests"
group ""

-- Include the editor project
group "Tools"
    include "Tools/Editor"
group ""

-- Include examples
group "Examples"
    include "Examples/Sandbox"
group ""

-- Include Third Party Libraries
group "ThirdParty"
    -- include third parties here
group ""