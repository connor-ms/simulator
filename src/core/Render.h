#pragma once

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

struct Globals;

class Renderer
{
public:
    void init(wgpu::Device device, wgpu::TextureFormat format, wgpu::Surface surface, wgpu::Buffer pb, Globals g, wgpu::BindGroupLayout globalsLayout, wgpu::BindGroup globals);
    void initBindGroupLayouts();
    void initBuffers();
    void initBindGroups();
    void initPipeline();
    void onFrame(wgpu::CommandEncoder encoder);

private:
    wgpu::Device m_device;
    wgpu::TextureFormat m_format;

    wgpu::Surface m_surface;

    wgpu::BindGroupLayout m_bindGroupLayout;
    wgpu::BindGroup m_bindGroup;

    wgpu::BindGroupLayout m_globalBindGroupLayout;
    wgpu::BindGroup m_globalBindGroup;

    wgpu::Buffer m_vb;
    wgpu::Buffer m_lineBuffer;
    wgpu::Buffer m_particleBuffer;

    wgpu::BindGroupLayout m_renderBindGroupLayout;
    wgpu::BindGroup m_renderBindGroup;
    wgpu::BindGroup m_lineRenderBindGroup;
    wgpu::RenderPipeline m_pipeline;
    wgpu::RenderPipeline m_linePipeline;
};