#pragma once

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include <GLFW/glfw3.h>

class GUI
{
public:
    bool init(wgpu::Device device, wgpu::TextureFormat format, GLFWwindow *window);
    void update(wgpu::RenderPassEncoder encoder);
};