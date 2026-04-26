#pragma once

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include <glm/glm.hpp>

struct Globals
{
    glm::vec2 mousePos;
    glm::i32 isMouseDown;
    glm::i32 _pad;
    // glm::u32 gridSize;
    glm::uvec4 gridSize;
    glm::f32 dt;
    glm::u32 particleCount;
    glm::f32 gravity;
    glm::f32 rest_density;
    glm::f32 dynamic_viscosity;
    glm::f32 eos_stiffness;
    glm::f32 eos_power;
    glm::f32 _pad1;
    // glm::f32 _pad4;
};

struct GPUContext
{
    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
    wgpu::Surface surface;
    wgpu::TextureFormat format;

    wgpu::Buffer globalsBuffer;

    wgpu::BindGroupLayout globalsBindGroupLayout;
    wgpu::BindGroup globalsBindGroup;

    Globals globals;
};