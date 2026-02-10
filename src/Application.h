#pragma once

#include <iostream>

#include <GLFW/glfw3.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
#include <dawn/webgpu_cpp_print.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include "GUI.h"

class Application
{
public:
    bool onInit();
    void onFrame();
    void onCompute();
    void onFinish() {};
    bool isRunning();

    void onResize(uint32_t width, uint32_t height);
    void onMouseMove(double xpos, double ypos) {};
    void onMouseButton(int button, int action, int mods) {};
    void onScroll(double xoffset, double yoffset) {};

private:
    bool initWindow();
    bool initInstance();
    bool initSurface();
    bool initRenderPipeline();
    bool initBuffers();
    bool initCompute();
    void initBindGroupLayout();
    void initBindGroup();

    void Render();

private:
    GLFWwindow *m_window;
    wgpu::Instance m_instance;
    wgpu::Adapter m_adapter;
    wgpu::Device m_device;
    wgpu::Surface m_surface;
    wgpu::TextureFormat m_format;

    wgpu::RenderPipeline m_pipeline;
    wgpu::Buffer m_vb;
    wgpu::Buffer m_ib;
    wgpu::Buffer m_uniformBuffer;

    wgpu::RenderPipeline m_pWorld;
    wgpu::Buffer m_worldbuf;

    wgpu::ComputePipeline m_computePipeline;
    wgpu::BindGroupLayout m_bindGroupLayout;
    wgpu::PipelineLayout m_pipelineLayout;
    float m_bufferSize;
    wgpu::Buffer m_inputBuffer;
    wgpu::Buffer m_outputBuffer;
    wgpu::Buffer m_mapBuffer;
    wgpu::BindGroup m_bindGroup;
    std::vector<float> m_input;

    GUI m_Gui;

    const uint32_t m_kWidth = 512;
    const uint32_t m_kHeight = 512;
};