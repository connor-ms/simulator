#include "Simulator.h"
#include <iostream>
#include "../core/Util.h"

void Simulator::init(GPUContext *ctx)
{
    m_ctx = ctx;
    m_state.particles = createParticleArray(1000);

    initBindGroupLayouts();
    initBuffers();
    initBindGroups();
    initPipeline();
}

void Simulator::initBindGroupLayouts()
{
    wgpu::BindGroupLayoutEntry computeEntry{};
    computeEntry.binding = 0;
    computeEntry.visibility = wgpu::ShaderStage::Compute;
    computeEntry.buffer.type = wgpu::BufferBindingType::Storage;

    wgpu::BindGroupLayoutDescriptor computeDesc{};
    computeDesc.entryCount = 1;
    computeDesc.entries = &computeEntry;
    m_bindGroupLayout = m_ctx->device.CreateBindGroupLayout(&computeDesc);

    if (!m_bindGroupLayout)
        std::cout << "ERROR: Failed to create compute bind group layout!" << std::endl;
}

void Simulator::initBuffers()
{
    // Particle buffer
    wgpu::BufferDescriptor desc{};
    desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    desc.mappedAtCreation = false;
    desc.size = m_state.particles.size() * sizeof(Particle);
    desc.label = "Particle";
    m_state.particleBuffer = m_ctx->device.CreateBuffer(&desc);
    m_ctx->device.GetQueue().WriteBuffer(m_state.particleBuffer, 0, m_state.particles.data(), m_state.particles.size() * sizeof(Particle));

    if (m_state.particleBuffer == nullptr)
    {
        std::cout << "ERROR: Failed to initialize particle buffer." << std::endl;
    }
}

void Simulator::initBindGroups()
{
    uint32_t particleBufferSize = static_cast<uint32_t>(m_state.particles.size() * sizeof(Particle));

    std::cout << particleBufferSize << std::endl;

    // Compute bind group
    wgpu::BindGroupEntry computeEntry{};
    computeEntry.binding = 0;
    computeEntry.buffer = m_state.particleBuffer;
    computeEntry.offset = 0;
    computeEntry.size = particleBufferSize;

    wgpu::BindGroupDescriptor computeDesc{};
    computeDesc.layout = m_bindGroupLayout;
    computeDesc.entryCount = 1;
    computeDesc.entries = &computeEntry;
    m_bindGroup = m_ctx->device.CreateBindGroup(&computeDesc);

    if (!m_bindGroup)
        std::cout << "ERROR: Failed to create compute bind group!" << std::endl;
}

void Simulator::initPipeline()
{
    std::cout << "initCompute" << std::endl;

    // compute 1
    {
        wgpu::ShaderModule computeShaderModule = Util::loadShaderModule(SHADER_DIR "/compute.wgsl", m_ctx->device);

        if (!computeShaderModule)
        {
            std::cout << "ERROR: Failed to load compute.wgsl!" << std::endl;
            return;
        }

        if (!m_bindGroupLayout)
        {
            std::cout << "ERROR: m_computeBindGroupLayout is null!" << std::endl;
            return;
        }

        wgpu::BindGroupLayout layouts[] = {
            m_ctx->globalsBindGroupLayout,
            m_bindGroupLayout,
        };

        wgpu::PipelineLayoutDescriptor plDesc{};
        plDesc.bindGroupLayoutCount = 2;
        plDesc.bindGroupLayouts = layouts;
        m_computePipelineLayout = m_ctx->device.CreatePipelineLayout(&plDesc);

        if (!m_computePipelineLayout)
        {
            std::cout << "ERROR: Failed to create compute pipeline layout!" << std::endl;
            return;
        }

        wgpu::ComputePipelineDescriptor cpDesc{};
        cpDesc.layout = m_computePipelineLayout;
        cpDesc.compute.module = computeShaderModule;
        cpDesc.compute.entryPoint = "computeSomething";
        cpDesc.label = "Compute";

        m_computePipeline = m_ctx->device.CreateComputePipeline(&cpDesc);
        if (!m_computePipeline)
        {
            std::cout << "ERROR: Failed to create compute pipeline!" << std::endl;
        }
    }

    // compute 2
    {
        wgpu::ShaderModule computeShaderModule = Util::loadShaderModule(SHADER_DIR "/compute2.wgsl", m_ctx->device);

        if (!computeShaderModule)
        {
            std::cout << "ERROR: Failed to load compute.wgsl!" << std::endl;
            return;
        }

        if (!m_bindGroupLayout)
        {
            std::cout << "ERROR: m_computeBindGroupLayout is null!" << std::endl;
            return;
        }

        wgpu::BindGroupLayout layouts[] = {
            m_ctx->globalsBindGroupLayout,
            m_bindGroupLayout,
        };

        wgpu::PipelineLayoutDescriptor plDesc{};
        plDesc.bindGroupLayoutCount = 2;
        plDesc.bindGroupLayouts = layouts;
        m_computePipelineLayout2 = m_ctx->device.CreatePipelineLayout(&plDesc);

        if (!m_computePipelineLayout2)
        {
            std::cout << "ERROR: Failed to create compute pipeline layout!" << std::endl;
            return;
        }

        wgpu::ComputePipelineDescriptor cpDesc{};
        cpDesc.layout = m_computePipelineLayout2;
        cpDesc.compute.module = computeShaderModule;
        cpDesc.compute.entryPoint = "computeSomething";
        cpDesc.label = "Compute";

        m_computePipeline2 = m_ctx->device.CreateComputePipeline(&cpDesc);
        if (!m_computePipeline2)
        {
            std::cout << "ERROR: Failed to create compute pipeline!" << std::endl;
            return;
        }
    }

    std::cout << "initCompute Done" << std::endl;
}

void Simulator::onFrame(wgpu::CommandEncoder encoder)
{
    {
        wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.SetPipeline(m_computePipeline);
        computePass.SetBindGroup(0, m_ctx->globalsBindGroup);
        computePass.SetBindGroup(1, m_bindGroup);

        uint32_t workgroupSize = 48;
        uint32_t workgroupCount = (static_cast<uint32_t>(m_state.particles.size()) + workgroupSize - 1) / workgroupSize;
        computePass.DispatchWorkgroups(workgroupCount, 1, 1);

        computePass.End();
    }
    {
        wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.SetPipeline(m_computePipeline2);
        computePass.SetBindGroup(0, m_ctx->globalsBindGroup);
        computePass.SetBindGroup(1, m_bindGroup);

        uint32_t workgroupSize = 48;
        uint32_t workgroupCount = (static_cast<uint32_t>(m_state.particles.size()) + workgroupSize - 1) / workgroupSize;
        computePass.DispatchWorkgroups(workgroupCount, 1, 1);

        computePass.End();
    }
}