add_library(imgui STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp

    external/imgui/misc/cpp/imgui_stdlib.cpp

    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_wgpu.cpp
)

target_include_directories(imgui PUBLIC
    external/imgui
    external/imgui/backends
    external/imgui/misc/cpp
)

target_compile_definitions(imgui PUBLIC
    IMGUI_IMPL_WEBGPU
    IMGUI_IMPL_WEBGPU_BACKEND_DAWN
)

if(APPLE)
    set_source_files_properties(
        external/imgui/backends/imgui_impl_glfw.cpp
        external/imgui/backends/imgui_impl_wgpu.cpp
        PROPERTIES LANGUAGE OBJCXX
    )
endif()

if(EMSCRIPTEN)
    target_link_libraries(imgui PUBLIC emdawnwebgpu_cpp glfw)
else()
    target_link_libraries(imgui PUBLIC webgpu_dawn glfw)
endif()
