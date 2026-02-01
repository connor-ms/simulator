#include "Application.h"

const char shaderCode[] = R"(
    @vertex fn vertexMain(@builtin(vertex_index) i : u32) ->
      @builtin(position) vec4f {
        const pos = array(vec2f(0, 1), vec2f(-1, -1), vec2f(1, -1));
        return vec4f(pos[i], 0, 1);
    }
    @fragment fn fragmentMain() -> @location(0) vec4f {
        return vec4f(1, 0, 0, 1);
    }
)";

void Application::Render()
{
    wgpu::SurfaceTexture surfaceTexture;
    m_surface.GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment attachment{.view = surfaceTexture.texture.CreateView(), .loadOp = wgpu::LoadOp::Clear, .storeOp = wgpu::StoreOp::Store};

    wgpu::RenderPassDescriptor renderpass{.colorAttachmentCount = 1, .colorAttachments = &attachment};

    std::cout << "frame" << std::endl;

    wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
    pass.SetPipeline(m_pipeline);
    pass.Draw(3);
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
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int, int)
    {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) that->onResize();
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
    wgpu::ShaderSourceWGSL wgsl{{.code = shaderCode}};

    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgsl};
    wgpu::ShaderModule shaderModule = m_device.CreateShaderModule(&shaderModuleDescriptor);

    wgpu::ColorTargetState colorTargetState{.format = m_format};

    wgpu::FragmentState fragmentState{.module = shaderModule, .targetCount = 1, .targets = &colorTargetState};

    wgpu::RenderPipelineDescriptor descriptor{.vertex = {.module = shaderModule}, .fragment = &fragmentState};
    m_pipeline = m_device.CreateRenderPipeline(&descriptor);
    std::cout << "Render pipeline created";

    return m_pipeline != nullptr;
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