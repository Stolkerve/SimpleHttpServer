workspace "SimpleHttpServerWorkspace"
    configurations { "Debug", "Release" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}"

project "SimpleHttpServer"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
    files { "./src/**.h", "./src/**.hpp", "./src/**.cpp" }

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	includedirs
	{
        "./asio/"
    }

	filter "system:linux"
		pic "On"
		systemversion "latest"
		links
		{
			"pthread",
		}
	filter "system:windows"
		systemversion "latest"
		links
		{
		}

    filter { "configurations:Debug" }
        defines { "DEBUG" }
        symbols "On"

    filter { "configurations:Release" }
        defines { "NDEBUG" }
        optimize "On"