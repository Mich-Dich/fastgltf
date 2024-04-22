project "fastgltf"
	kind "StaticLib"
	language "C++"
    staticruntime "off"

	targetdir ("bin/" .. outputs .. "/%{prj.name}")
	objdir ("bin-int/" .. outputs .. "/%{prj.name}")

	files
	{
		"include/fastgltf/base64.hpp",
		"include/fastgltf/core.hpp",
		"include/fastgltf/glm_element_traits.hpp",
		"include/fastgltf/tools.hpp",
		"include/fastgltf/types.hpp",
		"include/fastgltf/util.hpp",
		"src/fastgltf.cpp",
		"src/base64.cpp",

		"deps/simdjson/simdjson.h",
		"deps/simdjson/simdjson.cpp",
	}

	includedirs
	{
		"deps/simdjson",
		"include",
	}

	filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++17"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:RelWithDebInfo"
		-- buildoptions "/MD"
		-- defines "PFF_RELEASE_WITH_DEBUG_INFO"
		runtime "Release"
		symbols "on"
		optimize "speed"

	filter "configurations:Release"
		buildoptions "/MD"
		runtime "Release"
		optimize "on"
