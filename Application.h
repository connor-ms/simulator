#pragma once

#include <iostream>

#include <GLFW/glfw3.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
#include <dawn/webgpu_cpp_print.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

class Application
{
public:
    bool onInit();
    void onFrame();
    void onFinish();
    bool isRunning();

    void onResize() {};
    void onMouseMove(double xpos, double ypos) {};
    void onMouseButton(int button, int action, int mods) {};
    void onScroll(double xoffset, double yoffset) {};

private:
    bool initWindow();
    bool initInstance();
    bool initSurface();
    bool initRenderPipeline();

    void Render();

private:
    GLFWwindow *m_window;
    wgpu::Instance m_instance;
    wgpu::Adapter m_adapter;
    wgpu::Device m_device;
    wgpu::RenderPipeline m_pipeline;
    wgpu::Surface m_surface;
    wgpu::TextureFormat m_format;

    const uint32_t m_kWidth = 512;
    const uint32_t m_kHeight = 512;
};