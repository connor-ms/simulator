#pragma once

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "../core/GPUContext.h"
#include "../simulation/Simulator.h"
#include "Camera.h"

struct Globals;

struct Uniforms
{
    glm::mat4x4 projection;
    //    glm::mat4x4 view;
    //    glm::mat4x4 model;
};

class Renderer
{
public:
    void init(GPUContext *ctx, SimulationState *simState);
    void initBindGroupLayouts();
    void initBuffers();
    void initBindGroups();
    void initPipeline();
    void onFrame(wgpu::CommandEncoder encoder);

private:
    GPUContext *m_ctx;
    SimulationState *m_simState;

    Camera m_cam;
    Uniforms m_uniforms;

    wgpu::BindGroupLayout m_bindGroupLayout;
    wgpu::BindGroup m_bindGroup;

    wgpu::Buffer m_vb;
    wgpu::Buffer m_lineBuffer;

    wgpu::BindGroupLayout m_renderBindGroupLayout;
    wgpu::BindGroup m_renderBindGroup;
    wgpu::BindGroup m_lineRenderBindGroup;
    wgpu::RenderPipeline m_pipeline;
    wgpu::RenderPipeline m_linePipeline;

    wgpu::Buffer m_uniformBuffer;
    wgpu::BindGroupLayout m_uniformsBindGroupLayout;
    wgpu::BindGroup m_uniformsBindGroup;
};