cmake_minimum_required(VERSION 3.16)

project(ShitRenderEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    # warning level 4 and all warnings as errors
    #how to deal with these warns??
    #add_compile_options(/W4 /WX /wd4201 /wd4996 /wd4100 /wd4189 /wd4819) 
    #add_compile_options(/Wall /wd4514 /wd4625 /wd4626 /wd4668 /wd5045)
	#add_compile_options(/W4)
else()
    # lots of warnings and all warnings as errors -Weffc++
	add_compile_options(
		-Wall 
		-Wextra 
		-pedantic 
		-Wshadow 
		-ansi 
		-Wno-missing-field-initializers
		-Wnon-virtual-dtor 	
		-Wno-cast-function-type
	)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS on)

set(SHIT_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SHIT_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SHIT_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SHIT_OUTPUT_DIR})

set(SHIT_VK_VERSION_MAJOR 1 CACHE STRING 1)
set(SHIT_VK_VERSION_MINOR 1 CACHE STRING 1)
set(SHIT_VK_VERSION_PATCH 0 CACHE STRING 0)

set(CMAKE_DEBUG_POSTFIX d)

#configure_file(assets.cfg.in ${CMAKE_BINARY_DIR}/assets.cfg)
configure_file(config.hpp.in ${CMAKE_BINARY_DIR}/config.hpp)

set(X86 off CACHE BOOL "x86")

set(VULKAN_INCLUDE_DIR $ENV{VULKAN_SDK}/include)
if(WIN32)
if(X86)
   set(VULKAN_LIB_DIR $ENV{VULKAN_SDK}/lib32)
else()
   set(VULKAN_LIB_DIR $ENV{VULKAN_SDK}/lib)
endif()
endif()

#============================
## create engine library
file(GLOB engineFiles
${CMAKE_SOURCE_DIR}/engine/imguiimpl/*.hpp
${CMAKE_SOURCE_DIR}/engine/imguiimpl/*.cpp
engine/core/*.hpp
engine/core/*.cpp
)
add_library(engine ${engineFiles})
set_target_properties(engine PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

target_include_directories(engine PRIVATE 
${CMAKE_SOURCE_DIR}
${CMAKE_SOURCE_DIR}/engine/core
${CMAKE_SOURCE_DIR}/engine/imguiimpl
${CMAKE_SOURCE_DIR}/dependencies
${CMAKE_SOURCE_DIR}/dependencies/imgui
${CMAKE_SOURCE_DIR}/dependencies/ShitRenderer
${CMAKE_SOURCE_DIR}/dependencies/rapidjson
${CMAKE_SOURCE_DIR}/dependencies/stb
${CMAKE_BINARY_DIR} 
${VULKAN_INCLUDE_DIR}
)

target_link_libraries(engine PRIVATE
${CMAKE_SOURCE_DIR}/dependencies/lib/freetype.lib
${CMAKE_SOURCE_DIR}/dependencies/lib/imgui.lib
)
target_link_libraries(engine PRIVATE
debug
${CMAKE_SOURCE_DIR}/dependencies/lib/ShitRendererd.lib
${VULKAN_LIB_DIR}/shaderc_combinedd.lib
)
target_link_libraries(engine PRIVATE
optimized
${CMAKE_SOURCE_DIR}/dependencies/lib/ShitRenderer.lib
${VULKAN_LIB_DIR}/shaderc_combined.lib
)

add_custom_command(TARGET engine POST_BUILD COMMAND ${CMAKE_COMMAND} -E
copy \"${CMAKE_SOURCE_DIR}/dependencies/lib/brotlicommon.dll\"
\"${CMAKE_SOURCE_DIR}/dependencies/lib/brotlidec.dll\"
\"${CMAKE_SOURCE_DIR}/dependencies/lib/bz2.dll\"
\"${CMAKE_SOURCE_DIR}/dependencies/lib/glew32.dll\"
\"${CMAKE_SOURCE_DIR}/dependencies/lib/freetype.dll\"
\"${CMAKE_SOURCE_DIR}/dependencies/lib/libpng16.dll\"
\"${CMAKE_SOURCE_DIR}/dependencies/lib/zlib1.dll\"
\"${CMAKE_SOURCE_DIR}/dependencies/lib/GLRenderer$<$<CONFIG:debug>:d>.dll\"
\"${CMAKE_SOURCE_DIR}/dependencies/lib/VKRenderer$<$<CONFIG:debug>:d>.dll\"
\"${SHIT_OUTPUT_DIR}\\$<IF:$<CONFIG:Debug>,Debug,Release>\")

#===================
#add examples
function(engineexample examplename)
message(${examplename})

add_executable(${examplename} examples/${examplename}.cpp)

if(MSVC)
set_target_properties(${examplename} PROPERTIES FOLDER examples
DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
)
#COMPILE_FLAGS "/wd5054 /wd4100"

target_link_options(${examplename} PRIVATE	
	"/subsystem:windows"
)
endif()

target_include_directories(${examplename} PRIVATE 
${CMAKE_SOURCE_DIR}
${CMAKE_BINARY_DIR}
${CMAKE_SOURCE_DIR}/engine
${CMAKE_SOURCE_DIR}/engine/imguiimpl
${CMAKE_SOURCE_DIR}/dependencies
${CMAKE_SOURCE_DIR}/dependencies/imgui
${CMAKE_SOURCE_DIR}/dependencies/ShitRenderer
)

target_link_libraries(${examplename} PRIVATE
#glm::glm
#ShitRenderer
#imgui::imgui
engine
)

add_dependencies(${examplename} engine)

endfunction()
#======================
engineexample(01-image)
engineexample(02-mymodel)
engineexample(03-gltf-viewer)
engineexample(04-testscene)