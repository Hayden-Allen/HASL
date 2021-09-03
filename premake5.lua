project "hasl"
	kind "StaticLib"
	language "C++"
	cppdialect "C++latest"
	staticruntime "on"
	flags "MultiProcessorCompile"

	targetdir("bin/" .. outputdir)
	objdir("bin-int/" .. outputdir)
	
	pchheader "pch.h"
	pchsource "src/pch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		
	filter "configurations:Release"
		runtime "Release"
		optimize "on"