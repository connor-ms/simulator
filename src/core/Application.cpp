#include "Application.h"
#include "Util.h"
#include <random>
#include <glm/ext/matrix_clip_space.hpp>

struct Particle
{
    float center[2];
    float radius;
    float _pad;
};

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

int instanceCount = 1000;
std::vector<Particle> instances(instanceCount);
std::vector<Line> lines{};

void Application::Render()
{
    wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();

    // Begin compute pass
    wgpu::ComputePassDescriptor computePassDesc{};

    wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
    computePass.SetPipeline(m_computePipeline);
    computePass.SetBindGroup(0, m_globalBindGroup);
    computePass.SetBindGroup(1, m_computeBindGroup);

    uint32_t workgroupSize = 48;
    uint32_t workgroupCount = (static_cast<uint32_t>(instances.size()) + workgroupSize - 1) / workgroupSize;
    computePass.DispatchWorkgroups(workgroupCount, 1, 1);

    computePass.End();
    // End compute pass

    // Begin render pass
    wgpu::SurfaceTexture surfaceTexture;
    m_surface.GetCurrentTexture(&surfaceTexture);

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
    pass.SetBindGroup(0, m_globalBindGroup);
    pass.SetBindGroup(1, m_renderBindGroup);
    pass.SetVertexBuffer(0, m_vb);
    pass.Draw(6, static_cast<uint32_t>(instances.size()));

    // Draw lines
    pass.SetPipeline(m_linePipeline);
    pass.SetBindGroup(0, m_globalBindGroup);
    pass.SetBindGroup(1, m_lineRenderBindGroup);
    pass.SetVertexBuffer(0, m_vb);
    pass.Draw(6, static_cast<uint32_t>(lines.size()));

    m_Gui.update(pass);

    pass.End();
    // End render pass

    wgpu::CommandBuffer commands = encoder.Finish();
    m_device.GetQueue().Submit(1, &commands);
}

bool Application::initWindow()
{
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW!" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_kWidth, m_kHeight, "WebGPU window", nullptr, nullptr);

    if (!m_window)
    {
        std::cout << "Failed to create window!" << std::endl;
        return false;
    }

    // clang-format off
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int width, int height)
    {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) {
            that->onResize(width, height);
        }
    });
    glfwSetCursorPosCallback(m_window, [](GLFWwindow *window, double xpos, double ypos)
    {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) that->onMouseMove(xpos, ypos);
    });
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow *window, int button, int action, int mods)
    {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) that->onMouseButton(button, action, mods);
    });
    glfwSetScrollCallback(m_window, [](GLFWwindow *window, double xoffset, double yoffset)
    {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) that->onScroll(xoffset, yoffset);
    });
    // clang-format on

    return true;
}

bool Application::initInstance()
{
    static const auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor instanceDesc{.requiredFeatureCount = 1, .requiredFeatures = &kTimedWaitAny};
    m_instance = wgpu::CreateInstance(&instanceDesc);

    if (!m_instance)
    {
        std::cout << "Failed to initialize WebGPU!" << std::endl;
        return false;
    }

    std::cout << "Requesting adapter..." << std::endl;
    wgpu::Future f1 = m_instance.RequestAdapter(
        nullptr, wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message)
        {
            if (status != wgpu::RequestAdapterStatus::Success)
            {
                std::cout << "Failed: " << message.data << std::endl;
                exit(0);
            }
            m_adapter = std::move(adapter);
        });
    m_instance.WaitAny(f1, UINT64_MAX);
    std::cout << "Got adapter!" << std::endl;

    std::cout << "Requesting device..." << std::endl;
    wgpu::DeviceDescriptor desc{};
    desc.SetUncapturedErrorCallback(
        [](const wgpu::Device &, wgpu::ErrorType errorType, wgpu::StringView message)
        {
            std::cout << "Device error: " << message.data << std::endl;
        });

    wgpu::Future f2 = m_adapter.RequestDevice(
        &desc, wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message)
        {
            if (status != wgpu::RequestDeviceStatus::Success)
            {
                std::cout << "Failed: " << message.data << "\n";
                exit(0);
            }
            m_device = std::move(device);
        });
    m_instance.WaitAny(f2, UINT64_MAX);
    std::cout << "Got device!" << std::endl;

    return true;
}

bool Application::initSurface()
{
    std::cout << "Creating surface..." << std::endl;
    m_surface = wgpu::glfw::CreateSurfaceForWindow(m_instance, m_window);

    wgpu::SurfaceCapabilities capabilities;
    m_surface.GetCapabilities(m_adapter, &capabilities);
    m_format = capabilities.formats[0];

    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    wgpu::SurfaceConfiguration config{.device = m_device, .format = m_format, .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)};
    m_surface.Configure(&config);
    std::cout << "Surface created!" << std::endl;

    return true;
}

void Application::initBindGroupLayout()
{
    std::cout << "initBindGroupLayout" << std::endl;

    // Compute layout
    wgpu::BindGroupLayoutEntry computeEntry{};
    computeEntry.binding = 0;
    computeEntry.visibility = wgpu::ShaderStage::Compute;
    computeEntry.buffer.type = wgpu::BufferBindingType::Storage;

    wgpu::BindGroupLayoutDescriptor computeDesc{};
    computeDesc.entryCount = 1;
    computeDesc.entries = &computeEntry;
    m_computeBindGroupLayout = m_device.CreateBindGroupLayout(&computeDesc);

    if (!m_computeBindGroupLayout)
        std::cout << "ERROR: Failed to create compute bind group layout!" << std::endl;

    // Render layout
    wgpu::BindGroupLayoutEntry renderEntry{};
    renderEntry.binding = 0;
    renderEntry.visibility = wgpu::ShaderStage::Vertex;
    renderEntry.buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

    wgpu::BindGroupLayoutDescriptor renderDesc{};
    renderDesc.entryCount = 1;
    renderDesc.entries = &renderEntry;
    m_renderBindGroupLayout = m_device.CreateBindGroupLayout(&renderDesc);

    if (!m_renderBindGroupLayout)
        std::cout << "ERROR: Failed to create render bind group layout!" << std::endl;

    // Global layout
    wgpu::BindGroupLayoutEntry globalEntry{};
    globalEntry.binding = 0;
    globalEntry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
    globalEntry.buffer.type = wgpu::BufferBindingType::Uniform;

    wgpu::BindGroupLayoutDescriptor globalDesc{};
    globalDesc.entryCount = 1;
    globalDesc.entries = &globalEntry;
    m_globalBindGroupLayout = m_device.CreateBindGroupLayout(&globalDesc);

    if (!m_globalBindGroupLayout)
        std::cout << "ERROR: Failed to create global bind group layout!" << std::endl;

    std::cout << "initBindGroupLayout Done" << std::endl;
}

bool Application::initBuffers()
{
    std::cout << "initBuffers" << std::endl;

    // Quad vertex buffer
    wgpu::BufferDescriptor vbDesc{};
    vbDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    vbDesc.size = sizeof(vertices);
    vbDesc.mappedAtCreation = false;
    vbDesc.label = "Quad";
    m_vb = m_device.CreateBuffer(&vbDesc);
    m_device.GetQueue().WriteBuffer(m_vb, 0, vertices, sizeof(vertices));

    if (m_vb == nullptr)
    {
        std::cout << "ERROR: Failed to initialize quad vertex buffer." << std::endl;
        return false;
    }

    // Particle buffer
    wgpu::BufferDescriptor pbDesc{};
    pbDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    pbDesc.mappedAtCreation = false;
    pbDesc.size = instances.size() * sizeof(Particle);
    pbDesc.label = "Particle";
    m_particleBuffer = m_device.CreateBuffer(&pbDesc);
    m_device.GetQueue().WriteBuffer(m_particleBuffer, 0, instances.data(), instances.size() * sizeof(Particle));

    if (m_particleBuffer == nullptr)
    {
        std::cout << "ERROR: Failed to initialize particle buffer." << std::endl;
        return false;
    }

    // Line buffer
    wgpu::BufferDescriptor nDesc{};
    nDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    nDesc.mappedAtCreation = false;
    nDesc.size = lines.size() * sizeof(Line);
    nDesc.label = "Line";
    m_lineBuffer = m_device.CreateBuffer(&nDesc);
    m_device.GetQueue().WriteBuffer(m_lineBuffer, 0, lines.data(), lines.size() * sizeof(Line));

    if (m_lineBuffer == nullptr)
    {
        std::cout << "ERROR: Failed to initialize line buffer." << std::endl;
        return false;
    }

    // Global buffer
    wgpu::BufferDescriptor gDesc{};
    gDesc.size = sizeof(Globals);
    gDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    gDesc.label = "Global";
    m_globalBuffer = m_device.CreateBuffer(&gDesc);
    m_device.GetQueue().WriteBuffer(m_globalBuffer, 0, &m_globals, sizeof(Globals));

    if (m_globalBuffer == nullptr)
    {
        std::cout << "ERROR: Failed to initialize global buffer." << std::endl;
        return false;
    }

    return true;
}

void Application::initBindGroup()
{
    std::cout << "initBindGroup" << std::endl;
    uint32_t particleBufferSize = static_cast<uint32_t>(instances.size() * sizeof(Particle));
    uint32_t lineBufferSize = static_cast<uint32_t>(lines.size() * sizeof(Line));

    // Compute bind group
    wgpu::BindGroupEntry computeEntry{};
    computeEntry.binding = 0;
    computeEntry.buffer = m_particleBuffer;
    computeEntry.offset = 0;
    computeEntry.size = particleBufferSize;

    wgpu::BindGroupDescriptor computeDesc{};
    computeDesc.layout = m_computeBindGroupLayout;
    computeDesc.entryCount = 1;
    computeDesc.entries = &computeEntry;
    m_computeBindGroup = m_device.CreateBindGroup(&computeDesc);

    if (!m_computeBindGroup)
        std::cout << "ERROR: Failed to create compute bind group!" << std::endl;

    // Render bind group
    wgpu::BindGroupEntry renderEntry{};
    renderEntry.binding = 0;
    renderEntry.buffer = m_particleBuffer;
    renderEntry.offset = 0;
    renderEntry.size = particleBufferSize;

    wgpu::BindGroupDescriptor renderDesc{};
    renderDesc.layout = m_renderBindGroupLayout;
    renderDesc.entryCount = 1;
    renderDesc.entries = &renderEntry;
    m_renderBindGroup = m_device.CreateBindGroup(&renderDesc);

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
    m_lineRenderBindGroup = m_device.CreateBindGroup(&lineRenderDesc);

    if (!m_lineRenderBindGroup)
        std::cout << "ERROR: Failed to create line render bind group!" << std::endl;

    // Global bind group
    wgpu::BindGroupEntry globalEntry{};
    globalEntry.binding = 0;
    globalEntry.buffer = m_globalBuffer;
    globalEntry.offset = 0;
    globalEntry.size = sizeof(Globals);

    wgpu::BindGroupDescriptor globalDesc{};
    globalDesc.layout = m_globalBindGroupLayout;
    globalDesc.entryCount = 1;
    globalDesc.entries = &globalEntry;
    m_globalBindGroup = m_device.CreateBindGroup(&globalDesc);

    if (!m_renderBindGroup)
        std::cout << "ERROR: Failed to create global bind group!" << std::endl;

    std::cout << "initBindGroup Done" << std::endl;
}

bool Application::initRenderPipeline()
{
    std::cout << "Creating render pipeline" << std::endl;

    wgpu::ShaderModule particleShader = Util::loadShaderModule(SHADER_DIR "/particle.wgsl", m_device);
    wgpu::ShaderModule lineShader = Util::loadShaderModule(SHADER_DIR "/line.wgsl", m_device);
    if (!particleShader || !lineShader)
    {
        std::cout << "ERROR: Failed to load render shaders!" << std::endl;
        return false;
    }

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
        m_globalBindGroupLayout,
        m_renderBindGroupLayout,
    };

    wgpu::PipelineLayoutDescriptor plDesc{};
    plDesc.bindGroupLayoutCount = 2;
    plDesc.bindGroupLayouts = layouts;
    wgpu::PipelineLayout renderPipelineLayout = m_device.CreatePipelineLayout(&plDesc);

    wgpu::ColorTargetState colorTarget{};
    colorTarget.format = m_format;
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

        m_pipeline = m_device.CreateRenderPipeline(&rp);
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

        m_linePipeline = m_device.CreateRenderPipeline(&rp);
    }

    if (!m_linePipeline)
    {
        std::cout << "ERROR: Failed to create render pipeline!" << std::endl;
        return false;
    }
    std::cout << "Render pipeline created" << std::endl;

    return true;
}

bool Application::initCompute()
{
    std::cout << "initCompute" << std::endl;

    wgpu::ShaderModule computeShaderModule = Util::loadShaderModule(SHADER_DIR "/compute.wgsl", m_device);

    if (!computeShaderModule)
    {
        std::cout << "ERROR: Failed to load compute.wgsl!" << std::endl;
        return false;
    }

    if (!m_computeBindGroupLayout)
    {
        std::cout << "ERROR: m_computeBindGroupLayout is null!" << std::endl;
        return false;
    }

    wgpu::BindGroupLayout layouts[] = {
        m_globalBindGroupLayout,
        m_computeBindGroupLayout,
    };

    wgpu::PipelineLayoutDescriptor plDesc{};
    plDesc.bindGroupLayoutCount = 2;
    plDesc.bindGroupLayouts = layouts;
    m_computePipelineLayout = m_device.CreatePipelineLayout(&plDesc);

    if (!m_computePipelineLayout)
    {
        std::cout << "ERROR: Failed to create compute pipeline layout!" << std::endl;
        return false;
    }

    wgpu::ComputePipelineDescriptor cpDesc{};
    cpDesc.layout = m_computePipelineLayout;
    cpDesc.compute.module = computeShaderModule;
    cpDesc.compute.entryPoint = "computeSomething";
    cpDesc.label = "Compute";

    m_computePipeline = m_device.CreateComputePipeline(&cpDesc);
    if (!m_computePipeline)
    {
        std::cout << "ERROR: Failed to create compute pipeline!" << std::endl;
        return false;
    }

    std::cout << "initCompute Done" << std::endl;
    return true;
}

bool Application::onInit()
{
    if (!initWindow())
        return false;

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

    m_globals = Globals{
        .windowSize = glm::vec2(fbWidth, fbHeight),
        .worldSize = glm::vec4(256, 256, 256, 0),
    };

    float halfWidth = m_globals.windowSize.x * 0.5f;
    float halfHeight = m_globals.windowSize.y * 0.5f;

    m_globals.proj = glm::ortho(
        -halfWidth,
        halfWidth,
        -halfHeight,
        halfHeight,
        -1.f, 1.f);

    // top & bottom world bounds
    lines.push_back(Line{.p1 = glm::vec3(-m_globals.worldSize.x, m_globals.worldSize.y, 0), .p2 = glm::vec3(m_globals.worldSize.x, m_globals.worldSize.y, 0), .thickness = 1.f});
    lines.push_back(Line{.p1 = glm::vec3(-m_globals.worldSize.x, -m_globals.worldSize.y, 0), .p2 = glm::vec3(m_globals.worldSize.x, -m_globals.worldSize.y, 0), .thickness = 1.f});
    // left & right world bounds
    lines.push_back(Line{.p1 = glm::vec3(-m_globals.worldSize.x, -m_globals.worldSize.y, 0), .p2 = glm::vec3(-m_globals.worldSize.x, m_globals.worldSize.y, 0), .thickness = 1.f});
    lines.push_back(Line{.p1 = glm::vec3(m_globals.worldSize.x, -m_globals.worldSize.y, 0), .p2 = glm::vec3(m_globals.worldSize.x, m_globals.worldSize.y, 0), .thickness = 1.f});

    std::random_device rand;
    std::mt19937 generator(rand());
    std::uniform_real_distribution<float> dist(-m_globals.worldSize.x, m_globals.worldSize.x);

    for (int i = 0; i < instanceCount; i++)
        instances.push_back({{dist(generator), dist(generator)}, 10.f, 0.0f});

    if (!initInstance())
        return false;

    if (!initSurface())
        return false;

    initBindGroupLayout();

    if (!initBuffers())
        return false;

    initBindGroup();

    if (!initRenderPipeline())
        return false;

    if (!initCompute())
        return false;

    m_Gui = GUI();
    if (!m_Gui.init(m_device, m_format, m_window))
        return false;

    return true;
}

void Application::onFrame()
{
    glfwPollEvents();
    Render();
#ifndef __EMSCRIPTEN__
    m_surface.Present();
    m_device.Tick();
#endif
}

void Application::onResize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

    wgpu::SurfaceConfiguration config{.device = m_device, .format = m_format, .width = static_cast<uint32_t>(fbWidth), .height = static_cast<uint32_t>(fbHeight)};
    m_surface.Configure(&config);

    m_globals.windowSize = glm::vec2(fbWidth, fbHeight);

    float halfWidth = m_globals.windowSize.x * 0.5f;
    float halfHeight = m_globals.windowSize.y * 0.5f;

    m_globals.proj = glm::ortho(
        -halfWidth,
        halfWidth,
        -halfHeight,
        halfHeight,
        -1.f, 1.f);

    m_device.GetQueue().WriteBuffer(m_globalBuffer, 0, &m_globals, sizeof(Globals));
}

bool Application::isRunning()
{
    return !glfwWindowShouldClose(m_window);
}