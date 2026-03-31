cmake_minimum_required(VERSION 3.16)
project(glshowcase LANGUAGES C CXX)

file(GLOB CPP_SRCS src/*.cpp)

set(IMGUI_SRCS
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/backends/imgui_impl_glfw.cpp
    include/imgui/backends/imgui_impl_opengl3.cpp
)

set(STBI_SRC include/stb_image/stb_image.cpp)
set(STBI_INCLUDE include/stb_image)

set(GL3W_SRC include/gl3w/gl3w.c)
set(GL3W_INCLUDE include/gl3w)

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/models ${CMAKE_BINARY_DIR}/models
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/textures ${CMAKE_BINARY_DIR}/textures
)

add_executable(glshowcase
   	${CPP_SRCS}
	${IMGUI_SRCS}
	${GL3W_SRC}
	${STBI_SRC}
)

target_include_directories(glshowcase PRIVATE
    include
    include/imgui
    include/imgui/backends
    ${GL3W_INCLUDE}
	${STBI_INCLUDE}
)

find_package(OpenGL REQUIRED)
target_link_libraries(glshowcase PRIVATE 
	${CMAKE_SOURCE_DIR}/libs/libassimp.so
    glfw
    OpenGL::GL
    dl
)

target_compile_options(glshowcase PRIVATE -Wno-pragmas)

if(UNIX AND NOT APPLE)
    set_target_properties(glshowcase PROPERTIES
        BUILD_RPATH "${CMAKE_SOURCE_DIR}/libs"
    )
endif()

add_dependencies(glshowcase copy_assets)
