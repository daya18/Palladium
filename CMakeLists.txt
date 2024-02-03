# CMakeList.txt : CMake project for Palladium, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Palladium")

# Add source to this project's executable.
add_executable ( Palladium )

# Add sdl
add_subdirectory ( external/SDL )
set ( SDL_TEST OFF )
set ( SDL_STATIC ON )
target_link_libraries ( Palladium PRIVATE SDL2::SDL2-static )

if ( TARGET SDL2::SDL2main )
	target_link_libraries ( Palladium PRIVATE SDL2::SDL2main )
endif ()

# Add GLM
add_subdirectory ( external/GLM )
target_link_libraries ( Palladium PRIVATE glm )

# Add Vulkan
include ( "FindVulkan" )
message ( "Vulkan Found: ${Vulkan_FOUND}" )
target_link_libraries ( Palladium PRIVATE Vulkan::Vulkan )

target_include_directories ( Palladium PRIVATE 
	"external/OBJ-Loader/Source"
	external
)

target_sources ( Palladium PRIVATE 
	"source/Main.cpp"  
	source/Core.cpp
 "source/Application.cpp"  "source/Axel.cpp" "source/Camera.cpp")

# Setup precompiled headers
target_precompile_headers ( Palladium PRIVATE source/PCH.hpp )

target_compile_definitions ( Palladium PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Palladium PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
