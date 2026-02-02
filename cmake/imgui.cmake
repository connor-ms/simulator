add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp

    imgui/misc/cpp/imgui_stdlib.cpp

    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_wgpu.cpp
)

target_include_directories(imgui PUBLIC
    imgui
    imgui/backends
    imgui/misc/cpp
)

target_compile_definitions(imgui PUBLIC
    IMGUI_IMPL_WEBGPU
    IMGUI_IMPL_WEBGPU_BACKEND_DAWN
)

if(APPLE)
    set_source_files_properties(
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_wgpu.cpp
        PROPERTIES LANGUAGE OBJCXX
    )
endif()

if(EMSCRIPTEN)
    target_link_libraries(imgui PUBLIC emdawnwebgpu_cpp glfw)
else()
    target_link_libraries(imgui PUBLIC webgpu_dawn glfw)
endif()
