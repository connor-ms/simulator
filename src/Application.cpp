#include "Application.h"
#include "Util.h"

struct Instance
{
    float center[2];
    float radius;
    float _pad;
};

// clang-format off
float vertices[] = {
    // triangle 1
    -0.5f, -0.5f, 0.0f, 0.0f,
    0.5f,  -0.5f, 1.0f, 0.0f,
    0.5f,   0.5f, 1.0f, 1.0f,

    // triangle 2
    -0.5f, -0.5f, 0.0f, 0.0f,
    0.5f,   0.5f, 1.0f, 1.0f,
    -0.5f,  0.5f, 1.0f, 0.0f,
};

float wrld[] = {
    -0.5f, -0.5f,
    0.5f,  -0.5f,
    0.5f,   0.5f,
    -0.5f, -0.5f,
    0.5f,   0.5f,
    -0.5f,  0.5f,
};
// clang-format on

float smth = 0.0f;

std::vector<Instance> instances = {
    {{-0.5f, 0.0f}, 0.01f, 0.0f},
    {{0.0f, 0.3f}, 0.01f, 0.0f},
    {{0.4f, -0.2f}, 0.01f, 0.0f},
};

void Application::Render()
{
    wgpu::SurfaceTexture surfaceTexture;
    m_surface.GetCurrentTexture(&surfaceTexture);
    wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();
    wgpu::RenderPassColorAttachment attachment{.view = surfaceTexture.texture.CreateView(), .loadOp = wgpu::LoadOp::Clear, .storeOp = wgpu::StoreOp::Store};
    wgpu::RenderPassDescriptor renderpass{.colorAttachmentCount = 1, .colorAttachments = &attachment};

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);

    pass.SetPipeline(m_pipeline);
    m_device.GetQueue().WriteBuffer(m_ib, 0, instances.data(), (instances.size() * sizeof(Instance)));
    pass.SetVertexBuffer(0, m_vb);
    pass.SetVertexBuffer(1, m_ib);

    smth += 0.01f;
    instances.at(0).center[0] = smth;

    pass.Draw(6, static_cast<uint32_t>(instances.size()));

    // pass.SetPipeline(m_pWorld);
    // pass.SetVertexBuffer(0, m_worldbuf);
    // pass.Draw(6);

    m_Gui.update(pass);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    m_device.GetQueue().Submit(1, &commands);
}

bool Application::initWindow()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_kWidth, m_kHeight, "WebGPU window", nullptr, nullptr);

    if (!m_window)
    {
        std::cerr << "Failed to create window!" << std::endl;
        return false;
    }

    // clang-format off
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int width, int height)
    {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) that->onResize(width, height);
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
        std::cerr << "Failed to initialize WebGPU!" << std::endl;
        return false;
    }

    std::cout << "Requesting adapter..." << std::endl;
    wgpu::Future f1 = m_instance.RequestAdapter(
        nullptr, wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message)
        {
            if (status != wgpu::RequestAdapterStatus::Success)
            {
                std::cerr << "Failed: " << message << "\n";
                exit(0);
            }
            m_adapter = std::move(adapter);
        });
    m_instance.WaitAny(f1, UINT64_MAX);
    std::cout << "Got adapter!" << std::endl;

    std::cout << "Requesting device..." << std::endl;
    wgpu::DeviceDescriptor desc{};
    desc.SetUncapturedErrorCallback([](const wgpu::Device &, wgpu::ErrorType errorType, wgpu::StringView message)
                                    { std::cout << "Device error: " << errorType << " - message: " << message << std::endl; });

    // desc.SetDeviceLostCallback(wgpu::CallbackMode::WaitAnyOnly,
    //                            [](const wgpu::DeviceLostReason &info)
    //                            {
    //                                std::cerr << "Device lost! reason=" << info << std::endl;
    //                            });

    wgpu::Future f2 = m_adapter.RequestDevice(
        &desc, wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message)
        {
            if (status != wgpu::RequestDeviceStatus::Success)
            {
                std::cout << "Failed: " << message << "\n";
                exit(0);
            }
            m_device = std::move(device);
        });
    m_instance.WaitAny(f2, UINT64_MAX);
    std::cout << "Got device!" << m_device.Get() << std::endl;

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
    std::cout << "Surface created: " << m_surface.Get() << std::endl;

    return true;
}

bool Application::initRenderPipeline()
{
    std::cout << "Creating render pipeline\n"
              << std::endl;
    wgpu::ShaderModule shaderModule = Util::loadShaderModule(RESOURCE_DIR "/shader.wgsl", m_device);

    wgpu::VertexAttribute attrs[2];
    attrs[0].shaderLocation = 0;
    attrs[0].format = wgpu::VertexFormat::Float32x2;
    attrs[0].offset = 0;

    attrs[1].shaderLocation = 1;
    attrs[1].format = wgpu::VertexFormat::Float32x2;
    attrs[1].offset = sizeof(float) * 2;

    wgpu::VertexBufferLayout vbl{};
    vbl.arrayStride = sizeof(float) * 4;
    vbl.stepMode = wgpu::VertexStepMode::Vertex;
    vbl.attributeCount = 2;
    vbl.attributes = attrs;

    wgpu::VertexAttribute instanceAttrs[2];
    instanceAttrs[0].shaderLocation = 2;
    instanceAttrs[0].format = wgpu::VertexFormat::Float32x2;
    instanceAttrs[0].offset = 0;

    instanceAttrs[1].shaderLocation = 3;
    instanceAttrs[1].format = wgpu::VertexFormat::Float32;
    instanceAttrs[1].offset = sizeof(float) * 2;

    wgpu::VertexBufferLayout instances{};
    instances.arrayStride = sizeof(Instance);
    instances.stepMode = wgpu::VertexStepMode::Instance;
    instances.attributeCount = 2;
    instances.attributes = instanceAttrs;

    wgpu::VertexBufferLayout layouts[2] = {vbl, instances};

    wgpu::RenderPipelineDescriptor rp{};
    rp.vertex.module = shaderModule;
    rp.vertex.entryPoint = "vs_main";
    rp.vertex.bufferCount = 2;
    rp.vertex.buffers = layouts;

    wgpu::ColorTargetState colorTarget{};
    colorTarget.format = m_format;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;

    wgpu::FragmentState frag{};
    frag.module = shaderModule;
    frag.entryPoint = "fs_main";
    frag.targetCount = 1;
    frag.targets = &colorTarget;

    rp.fragment = &frag;
    rp.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    // rp.primitive.cullMode = wgpu::CullMode::None;

    m_pipeline = m_device.CreateRenderPipeline(&rp);
    std::cout << "Render pipeline created\n";

    //=======================

    wgpu::ShaderModule worldShader = Util::loadShaderModule(RESOURCE_DIR "/world.wgsl", m_device);

    wgpu::VertexAttribute worldattr;
    worldattr.shaderLocation = 0;
    worldattr.format = wgpu::VertexFormat::Float32x2;
    worldattr.offset = 0;

    wgpu::VertexBufferLayout wrldVBR{};
    wrldVBR.arrayStride = sizeof(float) * 2;
    wrldVBR.stepMode = wgpu::VertexStepMode::Vertex;
    wrldVBR.attributeCount = 1;
    wrldVBR.attributes = &worldattr;

    wgpu::RenderPipelineDescriptor rp2{};
    rp2.vertex.module = worldShader;
    rp2.vertex.entryPoint = "vs_main";
    rp2.vertex.bufferCount = 1;
    rp2.vertex.buffers = &wrldVBR;

    wgpu::ColorTargetState wrldcolorTarget{};
    wrldcolorTarget.format = m_format;
    wrldcolorTarget.writeMask = wgpu::ColorWriteMask::All;

    wgpu::FragmentState frag2{};
    frag2.module = worldShader;
    frag2.entryPoint = "fs_main";
    frag2.targetCount = 1;
    frag2.targets = &wrldcolorTarget;

    rp2.fragment = &frag2;
    rp2.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    // rp.primitive.cullMode = wgpu::CullMode::None;

    m_pWorld = m_device.CreateRenderPipeline(&rp2);
    std::cout << "World render pipeline created\n";

    return m_pipeline != nullptr && m_pWorld != nullptr;
}

bool Application::initBuffers()
{
    std::cout << "initBuffers" << std::endl;
    // Vertex buffer:
    wgpu::BufferDescriptor vbDesc{};
    vbDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    vbDesc.size = sizeof(vertices);

    m_vb = m_device.CreateBuffer(&vbDesc);
    m_device.GetQueue().WriteBuffer(m_vb, 0, vertices, sizeof(vertices));

    // Instance buffer:
    wgpu::BufferDescriptor ibDesc{};
    ibDesc.size = instances.size() * sizeof(Instance);
    ibDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    ibDesc.mappedAtCreation = false;

    m_ib = m_device.CreateBuffer(&ibDesc);
    m_device.GetQueue().WriteBuffer(m_ib, 0, instances.data(), instances.size() * sizeof(Instance));

    // World buffer:
    wgpu::BufferDescriptor wvbDesc{};
    wvbDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    wvbDesc.size = sizeof(wrld);

    m_worldbuf = m_device.CreateBuffer(&wvbDesc);
    m_device.GetQueue().WriteBuffer(m_worldbuf, 0, wrld, sizeof(wrld));

    std::cout << "initBuffers - compute" << std::endl;
    // Compute input/output buffers:
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.mappedAtCreation = false;
    bufferDesc.size = m_bufferSize;
    bufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    m_inputBuffer = m_device.CreateBuffer(&bufferDesc);
    bufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
    m_outputBuffer = m_device.CreateBuffer(&bufferDesc);

    // Create an intermediary buffer to which we copy the output and that can be
    // used for reading into the CPU memory.
    bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    m_mapBuffer = m_device.CreateBuffer(&bufferDesc);

    m_input = std::vector<float>(m_bufferSize / sizeof(float));
    for (int i = 0; i < m_input.size(); ++i)
    {
        m_input[i] = 0.1f * i;
    }
    m_device.GetQueue().WriteBuffer(m_inputBuffer, 0, m_input.data(), m_bufferSize);
    std::cout << "initBuffers Done" << std::endl;

    return m_vb != nullptr && m_ib != nullptr && m_worldbuf != nullptr;
}

bool Application::initCompute()
{
    std::cout << "initCompute" << std::endl;
    wgpu::ShaderModule computeShaderModule = Util::loadShaderModule(RESOURCE_DIR "/compute.wgsl", m_device);

    if (m_bindGroupLayout == nullptr)
        std::cout << "BGL NULL!" << std::endl;

    std::cout << "initCompute - pipelineLayout" << std::endl;
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc{};
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &m_bindGroupLayout;
    m_pipelineLayout = m_device.CreatePipelineLayout(&pipelineLayoutDesc);

    std::cout << "initCompute - computePipeline" << std::endl;
    wgpu::ComputePipelineDescriptor computePipelineDesc{};
    computePipelineDesc.layout = m_pipelineLayout;
    computePipelineDesc.compute.module = computeShaderModule;
    computePipelineDesc.compute.entryPoint = "computeSomething";

    m_computePipeline = m_device.CreateComputePipeline(&computePipelineDesc);
    std::cout << "initCompute - done" << std::endl;

    return m_computePipeline != nullptr;
}

void Application::onResize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    wgpu::SurfaceConfiguration config{.device = m_device, .format = m_format, .width = width, .height = height};
    m_surface.Configure(&config);
}

bool Application::isRunning()
{
    return !glfwWindowShouldClose(m_window);
}

bool Application::onInit()
{
    m_bufferSize = 64 * sizeof(float);
    if (!initWindow())
        return false;

    if (!initInstance())
        return false;

    if (!initSurface())
        return false;

    if (!initRenderPipeline())
        return false;

    m_Gui = GUI();
    if (!m_Gui.init(m_device, m_format, m_window))
        return false;

    if (!initBuffers())
        return false;

    initBindGroupLayout();
    initBindGroup();
    initCompute();
    return true;
}

void Application::onFrame()
{
    glfwPollEvents();
    onCompute();
    Render();
#ifndef __EMSCRIPTEN__
    m_surface.Present();
#endif
    // m_instance.ProcessEvents();
    m_device.Tick();
}

void Application::onCompute()
{
    // Initialize a command encoder
    wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();

    wgpu::ComputePassDescriptor computePassDesc{};
    computePassDesc.timestampWrites = nullptr;

    wgpu::ComputePassEncoder computePass = encoder.BeginComputePass(&computePassDesc);

    computePass.SetPipeline(m_computePipeline);
    computePass.SetBindGroup(0, m_bindGroup, 0, nullptr);
    uint32_t invocationCount = m_bufferSize / sizeof(float);
    uint32_t workgroupSize = 32;
    // This ceils invocationCount / workgroupSize
    uint32_t workgroupCount = (invocationCount + workgroupSize - 1) / workgroupSize;
    // computePass.dispatchWorkgroups(workgroupCount, 1, 1);

    computePass.DispatchWorkgroups(workgroupCount, 1, 1);

    computePass.End();
    encoder.CopyBufferToBuffer(m_outputBuffer, 0, m_mapBuffer, 0, m_bufferSize);

    // Encode and submit the GPU commands
    wgpu::CommandBuffer commands = encoder.Finish();
    m_device.GetQueue().Submit(1, &commands);
    // Future Buffer::MapAsync(MapMode mode, size_t offset, size_t size, CallbackMode callbackMode,F callback, T userdata) const {

    bool done = false;
    m_mapBuffer.MapAsync(
        wgpu::MapMode::Read,
        0,
        m_bufferSize,
        wgpu::CallbackMode::AllowProcessEvents,
        [&](wgpu::MapAsyncStatus status, wgpu::StringView message)
        {
            if (status == wgpu::MapAsyncStatus::Success)
            {
                const float *output =
                    static_cast<const float *>(
                        m_mapBuffer.GetConstMappedRange(0, m_bufferSize));

                for (size_t i = 0; i < m_input.size(); ++i)
                {
                    std::cout << "input " << m_input[i]
                              << " became " << output[i] << std::endl;
                }

                m_mapBuffer.Unmap();
            }
            else
            {
                std::cerr << "MapAsync failed: "
                          << message.data << std::endl;
            }

            done = true;
        });

    while (!done)
    {
        m_instance.ProcessEvents();
        // m_device.Tick();
    }
}

void Application::initBindGroupLayout()
{
    // Create bind group layout
    std::cout << "initBindGroupLayout" << std::endl;
    std::vector<wgpu::BindGroupLayoutEntry> bindings(2);

    // Input buffer
    bindings[0].binding = 0;
    bindings[0].visibility = wgpu::ShaderStage::Compute;
    bindings[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
    // bindings[0].buffer.minBindingSize = sizeof(float); // MUST be > 0
    // bindings[0].buffer.hasDynamicOffset = false;

    // Output buffer
    bindings[1].binding = 1;
    bindings[1].visibility = wgpu::ShaderStage::Compute;
    bindings[1].buffer.type = wgpu::BufferBindingType::Storage;
    std::cout << sizeof(float) << std::endl;
    // bindings[1].buffer.minBindingSize = sizeof(float); // MUST be > 0
    // bindings[1].buffer.hasDynamicOffset = false;

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;
    bindGroupLayoutDesc.entryCount = (uint32_t)bindings.size();
    bindGroupLayoutDesc.entries = bindings.data();
    m_bindGroupLayout = m_device.CreateBindGroupLayout(&bindGroupLayoutDesc);
    std::cout << "initBindGroupLayout Done" << std::endl;
}

void Application::initBindGroup()
{
    std::cout << "initBindGroup" << std::endl;
    std::vector<wgpu::BindGroupEntry> entries(2);

    // Input buffer
    entries[0].binding = 0;
    entries[0].buffer = m_inputBuffer;
    entries[0].offset = 0;
    entries[0].size = m_bufferSize;

    // Output buffer
    entries[1].binding = 1;
    entries[1].buffer = m_outputBuffer;
    entries[1].offset = 0;
    entries[1].size = m_bufferSize;

    wgpu::BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = m_bindGroupLayout;
    bindGroupDesc.entryCount = (uint32_t)entries.size();
    bindGroupDesc.entries = entries.data();
    m_bindGroup = m_device.CreateBindGroup(&bindGroupDesc);
    std::cout << "initBindGroup Done" << std::endl;
}