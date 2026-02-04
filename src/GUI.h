#pragma once

#include <dawn/webgpu_cpp.h>
#include <GLFW/glfw3.h>

class GUI
{
public:
    bool init(wgpu::Device device, wgpu::TextureFormat format, GLFWwindow *window);
    void update(wgpu::RenderPassEncoder encoder);
};