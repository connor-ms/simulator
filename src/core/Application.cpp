#include "Application.h"
#include "Util.h"
#include <random>
#include <glm/ext/matrix_clip_space.hpp>

void Application::init()
{
    initWindow();
    initInstance();
    initSurface();
    initGlobals();
    initBindGroupLayout();
    initBuffers();
    initBindGroup();

    m_sim.init(&m_ctx);
    m_renderer.init(&m_ctx, m_sim.getState());
    m_gui.init(&m_ctx, m_window);
}

void Application::initWindow()
{
    if (!glfwInit())
    {
        std::runtime_error("Failed to initialize GLFW!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_kWidth, m_kHeight, "WebGPU window", nullptr, nullptr);

    if (!m_window)
    {
        std::runtime_error("Failed to create window!");
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
}

void Application::initInstance()
{
    static const auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor instanceDesc{.requiredFeatureCount = 1, .requiredFeatures = &kTimedWaitAny};
    m_ctx.instance = wgpu::CreateInstance(&instanceDesc);

    if (!m_ctx.instance)
    {
        std::cout << "Failed to initialize WebGPU!" << std::endl;
        std::runtime_error("Failed to create window!");
    }

    std::cout << "Requesting adapter..." << std::endl;
    wgpu::Future f1 = m_ctx.instance.RequestAdapter(
        nullptr, wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message)
        {
            if (status != wgpu::RequestAdapterStatus::Success)
            {
                std::cout << "Failed: " << message.data << std::endl;
                exit(0);
            }
            m_ctx.adapter = std::move(adapter);
        });
    m_ctx.instance.WaitAny(f1, UINT64_MAX);
    std::cout << "Got adapter!" << std::endl;

    std::cout << "Requesting device..." << std::endl;
    wgpu::DeviceDescriptor desc{};
    desc.SetUncapturedErrorCallback(
        [](const wgpu::Device &, wgpu::ErrorType errorType, wgpu::StringView message)
        {
            std::cout << "Device error: " << message.data << std::endl;
        });

    wgpu::Future f2 = m_ctx.adapter.RequestDevice(
        &desc, wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message)
        {
            if (status != wgpu::RequestDeviceStatus::Success)
            {
                std::cout << "Failed: " << message.data << "\n";
                exit(0);
            }
            m_ctx.device = std::move(device);
        });
    m_ctx.instance.WaitAny(f2, UINT64_MAX);
    std::cout << "Got device!" << std::endl;
}

void Application::initSurface()
{
    std::cout << "Creating surface..." << std::endl;
    m_ctx.surface = wgpu::glfw::CreateSurfaceForWindow(m_ctx.instance, m_window);

    wgpu::SurfaceCapabilities capabilities;
    m_ctx.surface.GetCapabilities(m_ctx.adapter, &capabilities);
    m_ctx.format = capabilities.formats[0];

    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    wgpu::SurfaceConfiguration config{.device = m_ctx.device, .format = m_ctx.format, .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)};
    m_ctx.surface.Configure(&config);
    std::cout << "Surface created!" << std::endl;
}

void Application::initGlobals()
{
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

    m_ctx.globals = Globals{
        .windowSize = glm::vec2(fbWidth, fbHeight),
        .worldSize = glm::vec4(1000, 1000, 0, 0),
    };

    float halfWidth = m_ctx.globals.windowSize.x * 0.5f;
    float halfHeight = m_ctx.globals.windowSize.y * 0.5f;

    m_ctx.globals.proj = glm::ortho(
        -halfWidth,
        halfWidth,
        -halfHeight,
        halfHeight,
        -1.f, 1.f);

    m_ctx.globals.gridSize = 128;
    m_ctx.globals.dX = m_ctx.globals.worldSize.x / m_ctx.globals.gridSize;
    m_ctx.globals.idX = 1 / m_ctx.globals.dX;
    m_ctx.globals.dt = .001;
    m_ctx.globals.particleCount = 10000;
    m_ctx.globals.gravity = -98;

    m_ctx.globals.rest_density = 0.6;
    m_ctx.globals.dynamic_viscosity = 0.1;
    m_ctx.globals.eos_power = 2.0;
    m_ctx.globals.eos_stiffness = 1.0;
}

void Application::initBindGroupLayout()
{
    std::cout << "initBindGroupLayout" << std::endl;

    // Global layout
    wgpu::BindGroupLayoutEntry globalEntry{};
    globalEntry.binding = 0;
    globalEntry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
    globalEntry.buffer.type = wgpu::BufferBindingType::Uniform;

    wgpu::BindGroupLayoutDescriptor globalDesc{};
    globalDesc.entryCount = 1;
    globalDesc.entries = &globalEntry;
    m_ctx.globalsBindGroupLayout = m_ctx.device.CreateBindGroupLayout(&globalDesc);

    if (!m_ctx.globalsBindGroupLayout)
    {
        std::runtime_error("Failed to create global bind group layout!");
    }

    std::cout << "initBindGroupLayout Done" << std::endl;
}

void Application::initBuffers()
{
    std::cout << "initBuffers" << std::endl;

    // Global buffer
    wgpu::BufferDescriptor gDesc{};
    gDesc.size = sizeof(Globals);
    gDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    gDesc.label = "Global";
    m_ctx.globalsBuffer = m_ctx.device.CreateBuffer(&gDesc);
    m_ctx.device.GetQueue().WriteBuffer(m_ctx.globalsBuffer, 0, &m_ctx.globals, sizeof(Globals));

    if (m_ctx.globalsBuffer == nullptr)
    {
        std::runtime_error("Failed to initialize global buffer!");
    }
}

void Application::initBindGroup()
{
    std::cout << "initBindGroup" << std::endl;

    // Global bind group
    wgpu::BindGroupEntry globalEntry{};
    globalEntry.binding = 0;
    globalEntry.buffer = m_ctx.globalsBuffer;
    globalEntry.offset = 0;
    globalEntry.size = sizeof(Globals);

    wgpu::BindGroupDescriptor globalDesc{};
    globalDesc.layout = m_ctx.globalsBindGroupLayout;
    globalDesc.entryCount = 1;
    globalDesc.entries = &globalEntry;
    m_ctx.globalsBindGroup = m_ctx.device.CreateBindGroup(&globalDesc);

    if (!m_ctx.globalsBindGroup)
        std::cout << "ERROR: Failed to create global bind group!" << std::endl;

    std::cout << "initBindGroup Done" << std::endl;
}

void Application::onFrame()
{
    glfwPollEvents();

    wgpu::CommandEncoder encoder = m_ctx.device.CreateCommandEncoder();

    m_sim.onFrame(encoder);
    m_renderer.onFrame(encoder);
    m_gui.update(encoder);

    wgpu::CommandBuffer commands = encoder.Finish();
    m_ctx.device.GetQueue().Submit(1, &commands);

#ifndef __EMSCRIPTEN__
    m_ctx.surface.Present();
    m_ctx.device.Tick();
#endif
}

void Application::onResize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

    wgpu::SurfaceConfiguration config{.device = m_ctx.device, .format = m_ctx.format, .width = static_cast<uint32_t>(fbWidth), .height = static_cast<uint32_t>(fbHeight)};
    m_ctx.surface.Configure(&config);

    m_ctx.globals.windowSize = glm::vec2(fbWidth, fbHeight);

    float halfWidth = m_ctx.globals.windowSize.x * 0.5f;
    float halfHeight = m_ctx.globals.windowSize.y * 0.5f;

    m_ctx.globals.proj = glm::ortho(
        -halfWidth,
        halfWidth,
        -halfHeight,
        halfHeight,
        -1.f, 1.f);

    m_ctx.device.GetQueue().WriteBuffer(m_ctx.globalsBuffer, 0, &m_ctx.globals, sizeof(Globals));
}

bool Application::isRunning()
{
    return !glfwWindowShouldClose(m_window);
}