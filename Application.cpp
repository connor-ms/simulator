#include "Application.h"

#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

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

    // ImGui::Begin("Hello, world!");
    // ImGui::Text("This is some useful text.");
    // if (ImGui::Button("Click me"))
    // {
    //     std::cout << "clicked" << std::endl;
    // }
    // ImGui::End();

    wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);

    pass.SetPipeline(m_pipeline);
    pass.Draw(3);

    updateGui(pass);

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

    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    wgpu::SurfaceConfiguration config{.device = m_device, .format = m_format, .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)};
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

bool Application::initGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO();

    ImGui_ImplWGPU_InitInfo info = {};
    info.Device = m_device.Get();
    info.RenderTargetFormat = static_cast<WGPUTextureFormat>(m_format);

    ImGui_ImplGlfw_InitForOther(m_window, true);
    ImGui_ImplWGPU_Init(&info);
    return true;
}

void Application::updateGui(wgpu::RenderPassEncoder encoder)
{
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        static float f = 0.0f;
        static int counter = 0;
        static bool show_demo_window = true;
        static bool show_another_window = false;
        static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGuiIO &io = ImGui::GetIO();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), encoder.Get());
}

void Application::onResize()
{
    initSurface();
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

    if (!initGui())
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