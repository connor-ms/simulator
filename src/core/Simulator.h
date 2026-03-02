#include <webgpu/webgpu_cpp.h>
#include "Particle.h"

class Simulator
{
public:
    void init(wgpu::Device device, wgpu::BindGroupLayout globalsLayout, wgpu::BindGroup globals);
    void initBindGroupLayouts();
    void initBuffers();
    void initBindGroups();
    void initPipeline();
    void onFrame(wgpu::CommandEncoder encoder);
    wgpu::Buffer m_particleBuffer;

private:
    wgpu::Device m_device;
    wgpu::BindGroupLayout m_bindGroupLayout;
    wgpu::BindGroup m_bindGroup;

    wgpu::BindGroupLayout m_globalBindGroupLayout;
    wgpu::BindGroup m_globalBindGroup;

    wgpu::PipelineLayout m_computePipelineLayout;
    wgpu::ComputePipeline m_computePipeline;

    wgpu::PipelineLayout m_computePipelineLayout2;
    wgpu::ComputePipeline m_computePipeline2;

    std::vector<Particle> m_particles;
};