project "hasl"
	kind "StaticLib"
	language "C++"
	cppdialect "C++latest"
	staticruntime "on"

	targetdir("bin/" .. outputdir)
	objdir("bin-int/" .. outputdir)
	
	pchheader "pch.h"
	pchsource "include/pch.cpp"

	files
	{
		"include/**.h",
		"include/**.cpp"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		
	filter "configurations:Release"
		runtime "Release"
		optimize "on"