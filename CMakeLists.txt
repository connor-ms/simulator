cmake_minimum_required(VERSION 3.22)
project(app LANGUAGES CXX OBJCXX)
set(CMAKE_CXX_STANDARD 20)

include(cmake/imgui.cmake)
include(FetchContent)

add_executable(app
    src/main.cpp
    src/core/Application.cpp
    src/core/Util.cpp
    src/simulation/Simulator.cpp
    src/simulation/Particle.cpp
    src/render/Camera.cpp
    src/render/Render.cpp
    src/gui/GUI.cpp
)

target_compile_definitions(app PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/shaders"
)

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("external/dawn" EXCLUDE_FROM_ALL)

FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	0af55ccecd98d4e5a8d1fad7de25ba429d60e863 #refs/tags/1.0.1
)

FetchContent_MakeAvailable(glm)

target_link_libraries(app PRIVATE imgui glm::glm)

if(EMSCRIPTEN)
    set_target_properties(app PROPERTIES SUFFIX ".html")
    target_link_libraries(app PRIVATE emdawnwebgpu_cpp webgpu_glfw)
    target_link_options(app PRIVATE
        "-sASYNCIFY=1"
        "-sUSE_GLFW=3"
        "-sALLOW_MEMORY_GROWTH"
        --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders"
        --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/test.html"
    )
else()
    target_link_libraries(app PRIVATE webgpu_dawn webgpu_glfw glfw)
endif()
