#pragma once

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include <glm/glm.hpp>

struct Globals
{
    glm::vec2 windowSize;
    glm::vec2 _pad;
    glm::vec4 worldSize;
    glm::mat4x4 proj;
};

struct GPUContext
{
    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
    wgpu::Surface surface;
    wgpu::TextureFormat format;

    wgpu::Buffer globalsBuffer;
    // wgpu::Buffer m_gridBuffer;

    wgpu::BindGroupLayout globalsBindGroupLayout;
    wgpu::BindGroup globalsBindGroup;

    Globals globals;
};