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
    glm::mat4x4 view;
};

class Renderer
{
public:
    void init(GPUContext *ctx, SimulationState *simState);

    void onFrame(wgpu::CommandEncoder encoder);
    void onResize(uint32_t width, uint32_t height);
    void onMouseMove(double xpos, double ypos) {}
    void onMouseButton(int button, int action, int mods) {}

private:
    void initBuffers();
    void initBindGroupLayouts();
    void initBindGroups();
    void initPipeline();

    GPUContext *m_ctx;
    SimulationState *m_simState;

    Camera m_cam;
    Uniforms m_uniforms;

    wgpu::BindGroupLayout m_renderBindGroupLayout;
    wgpu::BindGroupLayout m_uniformsBindGroupLayout;

    wgpu::Buffer m_particleBuffer;
    wgpu::Buffer m_lineBuffer;
    wgpu::Buffer m_uniformBuffer;

    wgpu::BindGroup m_particleBindGroup;
    wgpu::BindGroup m_lineRenderBindGroup;
    wgpu::BindGroup m_uniformsBindGroup;

    wgpu::RenderPipeline m_particlePipeline;
    wgpu::RenderPipeline m_linePipeline;
};