#include "Application.h"
#include "Util.h"

struct Vertex
{
    float pos[2];
    float uv[2];
};

static const Vertex quad[] = {
    {{-1, -1}, {0, 0}},
    {{1, -1}, {1, 0}},
    {{1, 1}, {1, 1}},
    {{-1, -1}, {0, 0}},
    {{1, 1}, {1, 1}},
    {{-1, 1}, {0, 1}},
};

wgpu::Buffer CreateVertexBuffer(
    wgpu::Device device)
{
    wgpu::BufferDescriptor desc{};
    desc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    desc.size = sizeof(quad);

    wgpu::Buffer buffer = device.CreateBuffer(&desc);
    device.GetQueue().WriteBuffer(buffer, 0, quad, sizeof(quad));
    return buffer;
}

void Application::Render()
{
    wgpu::SurfaceTexture surfaceTexture;
    m_surface.GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment attachment{.view = surfaceTexture.texture.CreateView(), .loadOp = wgpu::LoadOp::Clear, .storeOp = wgpu::StoreOp::Store};

    wgpu::RenderPassDescriptor renderpass{.colorAttachmentCount = 1, .colorAttachments = &attachment};

    wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);

    pass.SetPipeline(m_pipeline);
    pass.SetVertexBuffer(0, m_vb);
    pass.Draw(3);

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

    wgpu::SurfaceConfiguration config{.device = m_device, .format = m_format, .width = m_kWidth, .height = m_kHeight};
    m_surface.Configure(&config);
    std::cout << "Surface created: " << m_surface.Get() << std::endl;

    return true;
}

bool Application::initRenderPipeline()
{
    std::cout << "Creating render pipeline" << std::endl;
    // wgpu::ShaderSourceWGSL wgsl{{.code = shaderCode}};

    // wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgsl};
    // wgpu::ShaderModule shaderModule = m_device.CreateShaderModule(&shaderModuleDescriptor);
    wgpu::ShaderModule shaderModule = Util::loadShaderModule(RESOURCE_DIR "/shader.wgsl", m_device);

    wgpu::VertexAttribute attrs[2];
    attrs[0].shaderLocation = 0;
    attrs[0].format = wgpu::VertexFormat::Float32x2;
    attrs[0].offset = 0;

    attrs[1].shaderLocation = 1;
    attrs[1].format = wgpu::VertexFormat::Float32x2;
    attrs[1].offset = sizeof(float) * 2;

    wgpu::VertexBufferLayout vbl{};
    vbl.arrayStride = sizeof(Vertex);
    vbl.attributeCount = 2;
    vbl.attributes = attrs;

    wgpu::ColorTargetState colorTargetState{.format = m_format};

    // wgpu::FragmentState fragmentState{.module = shaderModule, .targetCount = 1, .targets = &colorTargetState};

    wgpu::RenderPipelineDescriptor rp{};
    rp.vertex.module = shaderModule;
    rp.vertex.entryPoint = "vs_main";
    rp.vertex.bufferCount = 1;
    rp.vertex.buffers = &vbl;

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
    rp.primitive.cullMode = wgpu::CullMode::None;

    // wgpu::RenderPipelineDescriptor descriptor{.vertex = {.module = shaderModule}, .fragment = &fragmentState};
    // m_pipeline = m_device.CreateRenderPipeline(&descriptor);
    m_pipeline = m_device.CreateRenderPipeline(&rp);
    std::cout << "Render pipeline created";

    return m_pipeline != nullptr;
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

    m_vb = CreateVertexBuffer(m_device);

    return true;
}

void Application::onFrame()
{
    glfwPollEvents();
    Render();
#ifndef __EMSCRIPTEN__
    m_surface.Present();
#endif
    m_instance.ProcessEvents();
}