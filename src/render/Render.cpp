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
// clang-format on

std::vector<Line> lines{};

void Renderer::init(GPUContext *ctx, SimulationState *simState)
{
    m_ctx = ctx;
    m_simState = simState;
    m_cam = Camera();

    glm::vec3 lookAtPos(ctx->globals.gridSize / 2, 0, ctx->globals.gridSize / 2);
    m_cam.setTarget(lookAtPos);
    m_cam.buildViewMatrix();

    m_uniforms.projection = m_cam.getProjectionMatrix();
    m_uniforms.view = m_cam.getViewMatrix();

    const float LINE_THICKNESS = 1.0f;
    const float DEST = m_ctx->globals.gridSize;

    /* World bounding box */

    // front face
    lines.push_back(Line{.p1 = glm::vec3(0, DEST, 0), .p2 = glm::vec3(DEST, DEST, 0), .thickness = LINE_THICKNESS});
    lines.push_back(Line{.p1 = glm::vec3(0, 0, 0), .p2 = glm::vec3(DEST, 0, 0), .thickness = LINE_THICKNESS});
    lines.push_back(Line{.p1 = glm::vec3(0, 0, 0), .p2 = glm::vec3(0, DEST, 0), .thickness = LINE_THICKNESS});
    lines.push_back(Line{.p1 = glm::vec3(DEST, 0, 0), .p2 = glm::vec3(DEST, DEST, 0), .thickness = LINE_THICKNESS});

    // rear face
    lines.push_back(Line{.p1 = glm::vec3(0, DEST, DEST), .p2 = glm::vec3(DEST, DEST, DEST), .thickness = LINE_THICKNESS});
    lines.push_back(Line{.p1 = glm::vec3(0, 0, DEST), .p2 = glm::vec3(DEST, 0, DEST), .thickness = LINE_THICKNESS});
    lines.push_back(Line{.p1 = glm::vec3(0, 0, DEST), .p2 = glm::vec3(0, DEST, DEST), .thickness = LINE_THICKNESS});
    lines.push_back(Line{.p1 = glm::vec3(DEST, 0, DEST), .p2 = glm::vec3(DEST, DEST, DEST), .thickness = LINE_THICKNESS});

    // connecting edges
    lines.push_back(Line{.p1 = glm::vec3(0, 0, 0), .p2 = glm::vec3(0, 0, DEST), .thickness = LINE_THICKNESS});
    lines.push_back(Line{.p1 = glm::vec3(DEST, 0, 0), .p2 = glm::vec3(DEST, 0, DEST), .thickness = LINE_THICKNESS});
    lines.push_back(Line{.p1 = glm::vec3(0, DEST, 0), .p2 = glm::vec3(0, DEST, DEST), .thickness = LINE_THICKNESS});
    lines.push_back(Line{.p1 = glm::vec3(DEST, DEST, 0), .p2 = glm::vec3(DEST, DEST, DEST), .thickness = LINE_THICKNESS});

    initBuffers();
    initBindGroupLayouts();
    initBindGroups();
    initPipeline();

    wgpu::TextureDescriptor depthTexDesc{};
    depthTexDesc.size = {2560, 1440, 1};
    depthTexDesc.format = wgpu::TextureFormat::Depth24Plus; // must match pipeline
    depthTexDesc.usage = wgpu::TextureUsage::RenderAttachment;
    depthTexDesc.mipLevelCount = 1;
    depthTexDesc.sampleCount = 1;

    wgpu::Texture depthTexture = m_ctx->device.CreateTexture(&depthTexDesc);
    m_depthView = depthTexture.CreateView();
}

void Renderer::initBindGroupLayouts()
{
    // Particle & line layout
    {
        wgpu::BindGroupLayoutEntry entry{};
        entry.binding = 0;
        entry.visibility = wgpu::ShaderStage::Vertex;
        entry.buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

        wgpu::BindGroupLayoutDescriptor desc{};
        desc.entryCount = 1;
        desc.entries = &entry;
        m_renderBindGroupLayout = m_ctx->device.CreateBindGroupLayout(&desc);

        if (!m_renderBindGroupLayout)
            std::cout << "ERROR: Failed to create render bind group layout!" << std::endl;
    }

    // Uniforms
    {
        wgpu::BindGroupLayoutEntry entry{};
        entry.binding = 0;
        entry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
        entry.buffer.type = wgpu::BufferBindingType::Uniform;

        wgpu::BindGroupLayoutDescriptor desc{};
        desc.entryCount = 1;
        desc.entries = &entry;
        m_uniformsBindGroupLayout = m_ctx->device.CreateBindGroupLayout(&desc);

        if (!m_uniformsBindGroupLayout)
            std::cout << "ERROR: Failed to create uniforms bind group layout!" << std::endl;
    }
}

void Renderer::initBuffers()
{
    // Quad vertex buffer (for particles)
    {
        wgpu::BufferDescriptor desc{};
        desc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
        desc.size = sizeof(vertices);
        desc.mappedAtCreation = false;
        desc.label = "Quad";
        m_particleBuffer = m_ctx->device.CreateBuffer(&desc);
        m_ctx->device.GetQueue().WriteBuffer(m_particleBuffer, 0, vertices, sizeof(vertices));

        if (m_particleBuffer == nullptr)
            std::runtime_error("Failed to initialize quad vertex buffer.");
    }

    // Line buffer
    {
        wgpu::BufferDescriptor desc{};
        desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
        desc.mappedAtCreation = false;
        desc.size = lines.size() * sizeof(Line);
        desc.label = "Line";
        m_lineBuffer = m_ctx->device.CreateBuffer(&desc);
        m_ctx->device.GetQueue().WriteBuffer(m_lineBuffer, 0, lines.data(), lines.size() * sizeof(Line));

        if (m_lineBuffer == nullptr)
            std::runtime_error("ERROR: Failed to initialize line buffer.");
    }

    // Uniforms
    {
        wgpu::BufferDescriptor desc{};
        desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        desc.mappedAtCreation = false;
        desc.size = sizeof(Uniforms);
        desc.label = "Uniforms";
        m_uniformBuffer = m_ctx->device.CreateBuffer(&desc);
        m_ctx->device.GetQueue().WriteBuffer(m_uniformBuffer, 0, &m_uniforms, sizeof(m_uniforms));

        if (m_uniformBuffer == nullptr)
            std::runtime_error("ERROR: Failed to initialize uniform buffer.");
    }
}

void Renderer::initBindGroups()
{
    // Particles
    {
        wgpu::BindGroupEntry entry{};
        entry.binding = 0;
        entry.buffer = m_simState->particleBuffer;
        entry.offset = 0;
        entry.size = m_simState->particles.size() * sizeof(Particle);

        wgpu::BindGroupDescriptor desc{};
        desc.layout = m_renderBindGroupLayout;
        desc.entryCount = 1;
        desc.entries = &entry;
        m_particleBindGroup = m_ctx->device.CreateBindGroup(&desc);

        if (!m_particleBindGroup)
            std::cout << "ERROR: Failed to create render bind group!" << std::endl;
    }

    // Lines
    {
        wgpu::BindGroupEntry entry{};
        entry.binding = 0;
        entry.buffer = m_lineBuffer;
        entry.offset = 0;
        entry.size = static_cast<uint32_t>(lines.size() * sizeof(Line));

        wgpu::BindGroupDescriptor desc{};
        desc.layout = m_renderBindGroupLayout;
        desc.entryCount = 1;
        desc.entries = &entry;
        m_lineRenderBindGroup = m_ctx->device.CreateBindGroup(&desc);

        if (!m_lineRenderBindGroup)
            std::cout << "ERROR: Failed to create line render bind group!" << std::endl;
    }

    // Uniforms
    {
        wgpu::BindGroupEntry entry{};
        entry.binding = 0;
        entry.buffer = m_uniformBuffer;
        entry.offset = 0;
        entry.size = sizeof(Uniforms);

        wgpu::BindGroupDescriptor desc{};
        desc.layout = m_uniformsBindGroupLayout;
        desc.entryCount = 1;
        desc.entries = &entry;
        m_uniformsBindGroup = m_ctx->device.CreateBindGroup(&desc);
    }
}

void Renderer::initPipeline()
{
    std::cout << "Creating render pipeline" << std::endl;

    wgpu::BindGroupLayout layouts[] = {
        m_ctx->globalsBindGroupLayout,
        m_renderBindGroupLayout,
        m_uniformsBindGroupLayout,
    };

    wgpu::PipelineLayoutDescriptor plDesc{};
    plDesc.bindGroupLayoutCount = 3;
    plDesc.bindGroupLayouts = layouts;
    wgpu::PipelineLayout renderPipelineLayout = m_ctx->device.CreatePipelineLayout(&plDesc);

    wgpu::ColorTargetState colorTarget{};
    colorTarget.format = m_ctx->format;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;

    // Attributes used by both particle & line shaders
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

    // particle shader
    {
        wgpu::ShaderModule particleShader = Util::loadShaderModule(SHADER_DIR "/particle.wgsl", m_ctx->device);

        wgpu::FragmentState frag{};
        frag.module = particleShader;
        frag.entryPoint = "fs_main";
        frag.targetCount = 1;
        frag.targets = &colorTarget;

        wgpu::DepthStencilState depthStencil{};
        depthStencil.format = wgpu::TextureFormat::Depth24Plus;
        depthStencil.depthWriteEnabled = true;
        depthStencil.depthCompare = wgpu::CompareFunction::Less;

        wgpu::RenderPipelineDescriptor rp{};
        rp.layout = renderPipelineLayout;
        rp.vertex.module = particleShader;
        rp.vertex.entryPoint = "vs_main";
        rp.vertex.bufferCount = 1;
        rp.vertex.buffers = &vbl;
        rp.fragment = &frag;
        rp.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        rp.multisample.count = 1;
        rp.depthStencil = &depthStencil;

        m_particlePipeline = m_ctx->device.CreateRenderPipeline(&rp);
    }

    // line shader
    {
        wgpu::ShaderModule lineShader = Util::loadShaderModule(SHADER_DIR "/line.wgsl", m_ctx->device);

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
    m_uniforms.projection = m_cam.getProjectionMatrix();
    m_uniforms.view = m_cam.getViewMatrix();

    m_ctx->device.GetQueue().WriteBuffer(m_uniformBuffer, 0, &m_uniforms, sizeof(m_uniforms));

    wgpu::SurfaceTexture surfaceTexture;
    m_ctx->surface.GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment attachment{};
    attachment.view = surfaceTexture.texture.CreateView();
    attachment.loadOp = wgpu::LoadOp::Clear;
    attachment.storeOp = wgpu::StoreOp::Store;

    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    depthAttachment.view = m_depthView;
    depthAttachment.depthClearValue = 1.0f;
    depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
    depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
    depthAttachment.stencilLoadOp = wgpu::LoadOp::Undefined; // no stencil
    depthAttachment.stencilStoreOp = wgpu::StoreOp::Undefined;

    wgpu::RenderPassDescriptor renderpass{};
    renderpass.colorAttachmentCount = 1;
    renderpass.colorAttachments = &attachment;
    renderpass.depthStencilAttachment = &depthAttachment;

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);

    // Draw particles
    pass.SetPipeline(m_particlePipeline);
    pass.SetBindGroup(0, m_ctx->globalsBindGroup);
    pass.SetBindGroup(1, m_particleBindGroup);
    pass.SetBindGroup(2, m_uniformsBindGroup);
    pass.SetVertexBuffer(0, m_particleBuffer);
    pass.Draw(6, static_cast<uint32_t>(m_simState->particles.size()));

    // Draw lines
    // pass.SetPipeline(m_linePipeline);
    // pass.SetBindGroup(0, m_ctx->globalsBindGroup);
    // pass.SetBindGroup(1, m_lineRenderBindGroup);
    // pass.SetBindGroup(2, m_uniformsBindGroup);
    // pass.SetVertexBuffer(0, m_particleBuffer);
    // pass.Draw(6, static_cast<uint32_t>(lines.size()));

    pass.End();
}

void Renderer::onResize(uint32_t width, uint32_t height)
{
    m_cam.buildProjectionMatrix(width, height);
    m_uniforms.projection = m_cam.getProjectionMatrix();

    m_ctx->device.GetQueue().WriteBuffer(m_uniformBuffer, 0, &m_uniforms, sizeof(m_uniforms));
}