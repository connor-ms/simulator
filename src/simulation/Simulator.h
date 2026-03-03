#pragma once

#include <webgpu/webgpu_cpp.h>
#include "Particle.h"
#include "../core/GPUContext.h"

struct SimulationState
{
    wgpu::Buffer particleBuffer;
    wgpu::Buffer gridBuffer;
    std::vector<Particle> particles;
};

class Simulator
{
public:
    void init(GPUContext *ctx);
    void initBindGroupLayouts();
    void initBuffers();
    void initBindGroups();
    void initPipeline();
    void onFrame(wgpu::CommandEncoder encoder);

    SimulationState *getState() { return &m_state; }

private:
    GPUContext *m_ctx;
    SimulationState m_state;

    wgpu::BindGroupLayout m_bindGroupLayout;
    wgpu::BindGroup m_bindGroup;

    wgpu::PipelineLayout m_computePipelineLayout;
    wgpu::ComputePipeline m_computePipeline;

    wgpu::PipelineLayout m_computePipelineLayout2;
    wgpu::ComputePipeline m_computePipeline2;
};