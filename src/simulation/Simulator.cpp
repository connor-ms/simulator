#include "Simulator.h"
#include <iostream>

#include "../core/Util.h"
#include "Grid.h"

void Simulator::init(GPUContext *ctx)
{
    m_ctx = ctx;
    m_state.particles = createParticleArray(m_ctx->globals.particleCount, m_ctx->globals.worldSize.x / 2, m_ctx->globals.worldSize.y / 2, 0.05);

    initBindGroupLayouts();
    initBuffers();
    initBindGroups();
    initPipeline();
}

void Simulator::initBindGroupLayouts()
{
    wgpu::BindGroupLayoutEntry layouts[2];

    layouts[0].binding = 0;
    layouts[0].visibility = wgpu::ShaderStage::Compute;
    layouts[0].buffer.type = wgpu::BufferBindingType::Storage;

    layouts[1].binding = 1;
    layouts[1].visibility = wgpu::ShaderStage::Compute;
    layouts[1].buffer.type = wgpu::BufferBindingType::Storage;

    wgpu::BindGroupLayoutDescriptor computeDesc{};
    computeDesc.entryCount = 2;
    computeDesc.entries = layouts;
    m_bindGroupLayout = m_ctx->device.CreateBindGroupLayout(&computeDesc);

    if (!m_bindGroupLayout)
        std::cout << "ERROR: Failed to create compute bind group layout!" << std::endl;
}

void Simulator::initBuffers()
{
    // Particle buffer
    {
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

    // GridNode buffer
    {
        wgpu::BufferDescriptor desc{};
        desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
        desc.mappedAtCreation = false;
        desc.size = std::pow(m_ctx->globals.gridSize, 2) * sizeof(GridNode);
        desc.label = "GridNodes";
        m_state.gridBuffer = m_ctx->device.CreateBuffer(&desc);

        if (m_state.gridBuffer == nullptr)
        {
            std::cout << "ERROR: Failed to initialize grid buffer." << std::endl;
        }
    }
}

void Simulator::initBindGroups()
{
    uint32_t particleBufferSize = static_cast<uint32_t>(m_state.particles.size() * sizeof(Particle));

    wgpu::BindGroupEntry entries[2];

    entries[0].binding = 0;
    entries[0].buffer = m_state.particleBuffer;
    entries[0].offset = 0;
    entries[0].size = particleBufferSize;

    entries[1].binding = 1;
    entries[1].buffer = m_state.gridBuffer;
    entries[1].offset = 0;
    entries[1].size = std::pow(m_ctx->globals.gridSize, 2) * sizeof(GridNode);

    wgpu::BindGroupDescriptor computeDesc{};
    computeDesc.layout = m_bindGroupLayout;
    computeDesc.entryCount = 2;
    computeDesc.entries = entries;
    m_bindGroup = m_ctx->device.CreateBindGroup(&computeDesc);

    if (!m_bindGroup)
        std::cout << "ERROR: Failed to create compute bind group!" << std::endl;
}

void Simulator::initPipeline()
{
    std::cout << "initCompute" << std::endl;

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
        std::runtime_error("Failed to create compute pipeline layout!");
    }

    // clear grid
    {
        wgpu::ShaderModule clearGrid = Util::loadShaderModule(SHADER_DIR "/clearGrid.wgsl", m_ctx->device);

        if (!clearGrid)
        {
            std::runtime_error("Failed to load clearGrid.wgsl!");
        }

        wgpu::ComputePipelineDescriptor cpDesc{};
        cpDesc.layout = m_computePipelineLayout;
        cpDesc.compute.module = clearGrid;
        cpDesc.compute.entryPoint = "clearGrid";
        cpDesc.label = "clearGrid";

        m_clearGridPipeline = m_ctx->device.CreateComputePipeline(&cpDesc);

        if (!m_clearGridPipeline)
        {
            std::runtime_error("Failed to create clearGrid pipeline!");
        }
    }

    // particle to grid
    {
        wgpu::ShaderModule p2g = Util::loadShaderModule(SHADER_DIR "/p2g.wgsl", m_ctx->device);

        if (!p2g)
        {
            std::runtime_error("Failed to load p2g.wgsl!");
        }

        wgpu::ComputePipelineDescriptor cpDesc{};
        cpDesc.layout = m_computePipelineLayout;
        cpDesc.compute.module = p2g;
        cpDesc.compute.entryPoint = "p2g";
        cpDesc.label = "p2g";

        m_p2gPipeline = m_ctx->device.CreateComputePipeline(&cpDesc);

        if (!m_p2gPipeline)
        {
            std::runtime_error("Failed to create p2g pipeline!");
        }

        wgpu::ComputePipelineDescriptor cp2Desc{};
        cp2Desc.layout = m_computePipelineLayout;
        cp2Desc.compute.module = p2g;
        cp2Desc.compute.entryPoint = "p2g_2";
        cp2Desc.label = "p2g_2";

        m_p2g2Pipeline = m_ctx->device.CreateComputePipeline(&cp2Desc);

        if (!m_p2g2Pipeline)
        {
            std::runtime_error("Failed to create p2g pipeline!");
        }
    }

    // update grid
    {
        wgpu::ShaderModule updateGrid = Util::loadShaderModule(SHADER_DIR "/updateGrid.wgsl", m_ctx->device);

        if (!updateGrid)
        {
            std::runtime_error("Failed to load updateGrid.wgsl!");
        }

        wgpu::ComputePipelineDescriptor cpDesc{};
        cpDesc.layout = m_computePipelineLayout;
        cpDesc.compute.module = updateGrid;
        cpDesc.compute.entryPoint = "updateGrid";
        cpDesc.label = "updateGrid";

        m_updateGridPipeline = m_ctx->device.CreateComputePipeline(&cpDesc);

        if (!m_updateGridPipeline)
        {
            std::runtime_error("Failed to create updateGrid pipeline!");
        }
    }

    // g2p
    {
        wgpu::ShaderModule g2p = Util::loadShaderModule(SHADER_DIR "/g2p.wgsl", m_ctx->device);

        if (!g2p)
        {
            std::runtime_error("Failed to load updateGrid.wgsl!");
        }

        wgpu::ComputePipelineDescriptor cpDesc{};
        cpDesc.layout = m_computePipelineLayout;
        cpDesc.compute.module = g2p;
        cpDesc.compute.entryPoint = "g2p";
        cpDesc.label = "g2p";

        m_g2pPipeline = m_ctx->device.CreateComputePipeline(&cpDesc);

        if (!m_g2pPipeline)
        {
            std::runtime_error("Failed to create updateGrid pipeline!");
        }
    }

    std::cout << "initCompute Done" << std::endl;
}

void Simulator::onFrame(wgpu::CommandEncoder encoder)
{
    for (int i = 0; i < 5; i++)
    {
        // Clear grid pass
        {
            wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
            computePass.SetPipeline(m_clearGridPipeline);
            computePass.SetBindGroup(0, m_ctx->globalsBindGroup);
            computePass.SetBindGroup(1, m_bindGroup);

            uint32_t workgroupSize = 8;
            uint32_t workgroupCount = (static_cast<uint32_t>(m_ctx->globals.gridSize) + workgroupSize - 1) / workgroupSize;
            computePass.DispatchWorkgroups(workgroupCount, workgroupCount, 1);

            computePass.End();
        }

        // Particle to grid pass 1
        {
            wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
            computePass.SetPipeline(m_p2gPipeline);
            computePass.SetBindGroup(0, m_ctx->globalsBindGroup);
            computePass.SetBindGroup(1, m_bindGroup);

            uint32_t workgroupSize = 64;
            uint32_t workgroupCount = (static_cast<uint32_t>(m_ctx->globals.particleCount) + workgroupSize - 1) / workgroupSize;
            computePass.DispatchWorkgroups(workgroupCount, 1, 1);

            computePass.End();
        }

        // Particle to grid pass 2
        {
            wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
            computePass.SetPipeline(m_p2g2Pipeline);
            computePass.SetBindGroup(0, m_ctx->globalsBindGroup);
            computePass.SetBindGroup(1, m_bindGroup);

            uint32_t workgroupSize = 64;
            uint32_t workgroupCount = (static_cast<uint32_t>(m_ctx->globals.particleCount) + workgroupSize - 1) / workgroupSize;
            computePass.DispatchWorkgroups(workgroupCount, 1, 1);

            computePass.End();
        }

        // Update grid pass
        {
            wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
            computePass.SetPipeline(m_updateGridPipeline);
            computePass.SetBindGroup(0, m_ctx->globalsBindGroup);
            computePass.SetBindGroup(1, m_bindGroup);

            uint32_t workgroupSize = 8;
            uint32_t workgroupCount = (static_cast<uint32_t>(m_ctx->globals.gridSize) + workgroupSize - 1) / workgroupSize;
            computePass.DispatchWorkgroups(workgroupCount, workgroupCount, 1);

            computePass.End();
        }

        // Grid to particle pass
        {
            wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
            computePass.SetPipeline(m_g2pPipeline);
            computePass.SetBindGroup(0, m_ctx->globalsBindGroup);
            computePass.SetBindGroup(1, m_bindGroup);

            uint32_t workgroupSize = 64;
            uint32_t workgroupCount = (static_cast<uint32_t>(m_ctx->globals.particleCount) + workgroupSize - 1) / workgroupSize;
            computePass.DispatchWorkgroups(workgroupCount, 1, 1);

            computePass.End();
        }
    }
}