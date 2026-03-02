#include "Application.h"
#include "Util.h"
#include <random>
#include <glm/ext/matrix_clip_space.hpp>

const int GRID_OBJ_SIZE = 16;
const int GRID_RES = 32;

const int INST_SIZE = 40000;

void Application::Render()
{
    wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();

    // Begin compute pass
    m_sim.onFrame(encoder);
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

    m_renderer.onFrame(pass);
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

    // Grid buffer
    wgpu::BufferDescriptor gridDesc{};
    gridDesc.size = GRID_RES * GRID_OBJ_SIZE;
    gridDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    gridDesc.label = "Grid";
    m_gridBuffer = m_device.CreateBuffer(&gridDesc);
    // m_device.GetQueue().WriteBuffer(m_gridBuffer, 0, &m_globals, sizeof(Globals));

    if (m_gridBuffer == nullptr)
    {
        std::cout << "ERROR: Failed to initialize grid buffer." << std::endl;
        return false;
    }

    return true;
}

void Application::initBindGroup()
{
    std::cout << "initBindGroup" << std::endl;

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

    if (!m_globalBindGroup)
        std::cout << "ERROR: Failed to create global bind group!" << std::endl;

    std::cout << "initBindGroup Done" << std::endl;
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

    if (!initInstance())
        return false;

    if (!initSurface())
        return false;

    initBindGroupLayout();

    if (!initBuffers())
        return false;

    initBindGroup();
    m_sim = Simulator();
    m_sim.init(m_device, m_globalBindGroupLayout, m_globalBindGroup);
    m_renderer = Renderer();
    m_renderer.init(m_device, m_format, m_sim.m_particleBuffer, m_globals, m_globalBindGroupLayout, m_globalBindGroup);

    m_Gui = GUI();
    if (!m_Gui.init(m_device, m_format, m_window))
        return false;

    // exit(1);

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