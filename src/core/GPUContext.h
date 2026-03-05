#pragma once

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include <glm/glm.hpp>

struct Globals
{
    glm::vec2 windowSize;  // 0 + 2*4 = 8
    glm::vec2 _pad;        // 8 + 2*4 = 16
    glm::vec4 worldSize;   // 16 + 4*4 = 32
    glm::mat4x4 proj;      // 32 + 4*4*4 = 96
    glm::u32 gridSize;     // 96 + 4 = 100
    glm::f32 dX;           // 104
    glm::f32 idX;          // 108
    glm::f32 dt;           // 112
    glm::f32 particleMass; // 116
    glm::u32 particleCount;
    glm::f32 gravity;
    glm::f32 _pad2;
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