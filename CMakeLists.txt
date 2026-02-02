cmake_minimum_required(VERSION 3.22)
project(app LANGUAGES CXX OBJCXX)
set(CMAKE_CXX_STANDARD 20)

add_executable(app
    src/main.cpp
    src/Application.cpp
)

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("external/dawn" EXCLUDE_FROM_ALL)

include(cmake/imgui.cmake)
target_link_libraries(app PRIVATE imgui)

if(EMSCRIPTEN)
    set_target_properties(app PROPERTIES SUFFIX ".html")
    target_link_libraries(app PRIVATE emdawnwebgpu_cpp webgpu_glfw)
    target_link_options(app PRIVATE "-sASYNCIFY=1" "-sUSE_GLFW=3")
else()
    target_link_libraries(app PRIVATE webgpu_dawn webgpu_glfw glfw)
endif()
