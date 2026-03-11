#include "Render.h"
#include "../core/Util.h"
#include "../core/Application.h"

struct Line
{
    glm::vec3 p1;
    float _pad1;
    glm::vec3 p2;
    float thickness;
};

// clang-format off
float vertices[] = {
    // triangle 1
    -0.5f, -0.5f, 0.0f, 0.0f,
     0.5f, -0.5f, 1.0f, 0.0f,
     0.5f,  0.5f, 1.0f, 1.0f,

    // triangle 2
    -0.5f, -0.5f, 0.0f, 0.0f,
     0.5f,  0.5f, 1.0f, 1.0f,
    -0.5f,  0.5f, 1.0f, 0.0f,
};

float wrld[] = {
    -0.5f, -0.5f,
     0.5f, -0.5f,
     0.5f,  0.5f,
    -0.5f, -0.5f,
     0.5f,  0.5f,
    -0.5f,  0.5f,
};
// clang-format on

std::vector<Line> lines{};

void Renderer::init(GPUContext *ctx, SimulationState *simState)
{
    m_ctx = ctx;
    m_simState = simState;

    // top & bottom world bounds
    lines.push_back(Line{.p1 = glm::vec3(0, m_ctx->globals.gridSize, 0), .p2 = glm::vec3(m_ctx->globals.gridSize, m_ctx->globals.gridSize, 0), .thickness = .1f});
    lines.push_back(Line{.p1 = glm::vec3(0, 0, 0), .p2 = glm::vec3(m_ctx->globals.gridSize, 0, 0), .thickness = .1f});
    // left & right world bounds
    lines.push_back(Line{.p1 = glm::vec3(0, 0, 0), .p2 = glm::vec3(0, m_ctx->globals.gridSize, 0), .thickness = .1f});
    lines.push_back(Line{.p1 = glm::vec3(m_ctx->globals.gridSize, 0, 0), .p2 = glm::vec3(m_ctx->globals.gridSize, m_ctx->globals.gridSize, 0), .thickness = .1f});

    initBindGroupLayouts();
    initBuffers();
    initBindGroups();
    initPipeline();
}

void Renderer::initBindGroupLayouts()
{
    // Render layout
    wgpu::BindGroupLayoutEntry renderEntry{};
    renderEntry.binding = 0;
    renderEntry.visibility = wgpu::ShaderStage::Vertex;
    renderEntry.buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

    wgpu::BindGroupLayoutDescriptor renderDesc{};
    renderDesc.entryCount = 1;
    renderDesc.entries = &renderEntry;
    m_renderBindGroupLayout = m_ctx->device.CreateBindGroupLayout(&renderDesc);

    if (!m_renderBindGroupLayout)
        std::cout << "ERROR: Failed to create render bind group layout!" << std::endl;
}

void Renderer::initBuffers()
{
    // Quad vertex buffer
    wgpu::BufferDescriptor vbDesc{};
    vbDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    vbDesc.size = sizeof(vertices);
    vbDesc.mappedAtCreation = false;
    vbDesc.label = "Quad";
    m_vb = m_ctx->device.CreateBuffer(&vbDesc);
    m_ctx->device.GetQueue().WriteBuffer(m_vb, 0, vertices, sizeof(vertices));

    if (m_vb == nullptr)
        std::runtime_error("Failed to initialize quad vertex buffer.");

    // Line buffer
    wgpu::BufferDescriptor nDesc{};
    nDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    nDesc.mappedAtCreation = false;
    nDesc.size = lines.size() * sizeof(Line);
    nDesc.label = "Line";
    m_lineBuffer = m_ctx->device.CreateBuffer(&nDesc);
    m_ctx->device.GetQueue().WriteBuffer(m_lineBuffer, 0, lines.data(), lines.size() * sizeof(Line));

    if (m_lineBuffer == nullptr)
        std::runtime_error("ERROR: Failed to initialize line buffer.");
}

void Renderer::initBindGroups()
{
    uint32_t lineBufferSize = static_cast<uint32_t>(lines.size() * sizeof(Line));

    // Render bind group
    wgpu::BindGroupEntry renderEntry{};
    renderEntry.binding = 0;
    renderEntry.buffer = m_simState->particleBuffer;
    renderEntry.offset = 0;
    renderEntry.size = m_simState->particles.size() * sizeof(Particle);

    wgpu::BindGroupDescriptor renderDesc{};
    renderDesc.layout = m_renderBindGroupLayout;
    renderDesc.entryCount = 1;
    renderDesc.entries = &renderEntry;
    m_renderBindGroup = m_ctx->device.CreateBindGroup(&renderDesc);

    if (!m_renderBindGroup)
        std::cout << "ERROR: Failed to create render bind group!" << std::endl;

    // Line bind group
    wgpu::BindGroupEntry lineEntry{};
    lineEntry.binding = 0;
    lineEntry.buffer = m_lineBuffer;
    lineEntry.offset = 0;
    lineEntry.size = lineBufferSize;

    wgpu::BindGroupDescriptor lineRenderDesc{};
    lineRenderDesc.layout = m_renderBindGroupLayout;
    lineRenderDesc.entryCount = 1;
    lineRenderDesc.entries = &lineEntry;
    m_lineRenderBindGroup = m_ctx->device.CreateBindGroup(&lineRenderDesc);

    if (!m_lineRenderBindGroup)
        std::cout << "ERROR: Failed to create line render bind group!" << std::endl;
}

void Renderer::initPipeline()
{
    std::cout << "Creating render pipeline" << std::endl;

    wgpu::ShaderModule particleShader = Util::loadShaderModule(SHADER_DIR "/particle.wgsl", m_ctx->device);
    wgpu::ShaderModule lineShader = Util::loadShaderModule(SHADER_DIR "/line.wgsl", m_ctx->device);

    wgpu::VertexAttribute attrs[2];

    // pos
    attrs[0].shaderLocation = 0;
    attrs[0].format = wgpu::VertexFormat::Float32x2;
    attrs[0].offset = 0;

    // uv
    attrs[1].shaderLocation = 1;
    attrs[1].format = wgpu::VertexFormat::Float32x2;
    attrs[1].offset = sizeof(float) * 2;

    wgpu::VertexBufferLayout vbl{};
    vbl.arrayStride = sizeof(float) * 4;
    vbl.stepMode = wgpu::VertexStepMode::Vertex;
    vbl.attributeCount = 2;
    vbl.attributes = attrs;

    wgpu::BindGroupLayout layouts[] = {
        m_ctx->globalsBindGroupLayout,
        m_renderBindGroupLayout,
    };

    wgpu::PipelineLayoutDescriptor plDesc{};
    plDesc.bindGroupLayoutCount = 2;
    plDesc.bindGroupLayouts = layouts;
    wgpu::PipelineLayout renderPipelineLayout = m_ctx->device.CreatePipelineLayout(&plDesc);

    wgpu::ColorTargetState colorTarget{};
    colorTarget.format = m_ctx->format;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;

    // particle shader
    {
        wgpu::FragmentState frag{};
        frag.module = particleShader;
        frag.entryPoint = "fs_main";
        frag.targetCount = 1;
        frag.targets = &colorTarget;

        wgpu::RenderPipelineDescriptor rp{};
        rp.layout = renderPipelineLayout;
        rp.vertex.module = particleShader;
        rp.vertex.entryPoint = "vs_main";
        rp.vertex.bufferCount = 1;
        rp.vertex.buffers = &vbl;
        rp.fragment = &frag;
        rp.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        rp.multisample.count = 1;

        m_pipeline = m_ctx->device.CreateRenderPipeline(&rp);
    }

    // line shader
    {
        wgpu::FragmentState frag{};
        frag.module = lineShader;
        frag.entryPoint = "fs_main";
        frag.targetCount = 1;
        frag.targets = &colorTarget;

        wgpu::RenderPipelineDescriptor rp{};
        rp.layout = renderPipelineLayout;
        rp.vertex.module = lineShader;
        rp.vertex.entryPoint = "vs_main";
        rp.vertex.bufferCount = 1;
        rp.vertex.buffers = &vbl;
        rp.fragment = &frag;
        rp.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        rp.multisample.count = 1;

        m_linePipeline = m_ctx->device.CreateRenderPipeline(&rp);
    }

    if (!m_linePipeline)
        std::runtime_error("ERROR: Failed to create render pipeline!");

    std::cout << "Render pipeline created" << std::endl;
}

void Renderer::onFrame(wgpu::CommandEncoder encoder)
{
    wgpu::SurfaceTexture surfaceTexture;
    m_ctx->surface.GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment attachment{};
    attachment.view = surfaceTexture.texture.CreateView();
    attachment.loadOp = wgpu::LoadOp::Clear;
    attachment.storeOp = wgpu::StoreOp::Store;

    wgpu::RenderPassDescriptor renderpass{};
    renderpass.colorAttachmentCount = 1;
    renderpass.colorAttachments = &attachment;

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);

    // Draw particles
    pass.SetPipeline(m_pipeline);
    pass.SetBindGroup(0, m_ctx->globalsBindGroup);
    pass.SetBindGroup(1, m_renderBindGroup);
    pass.SetVertexBuffer(0, m_vb);
    pass.Draw(6, static_cast<uint32_t>(m_simState->particles.size()));

    // Draw lines
    pass.SetPipeline(m_linePipeline);
    pass.SetBindGroup(0, m_ctx->globalsBindGroup);
    pass.SetBindGroup(1, m_lineRenderBindGroup);
    pass.SetVertexBuffer(0, m_vb);
    pass.Draw(6, static_cast<uint32_t>(lines.size()));

    pass.End();
}