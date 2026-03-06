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
    glm::u32 gridSize;
    glm::f32 dX;
    glm::f32 idX;
    glm::f32 dt;
    glm::u32 particleCount;
    glm::f32 gravity;
    glm::f32 rest_density;
    glm::f32 dynamic_viscosity;
    glm::f32 eos_stiffness;
    glm::f32 eos_power;
    glm::f32 _pad2;
    glm::f32 _pad3;
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