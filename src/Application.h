#pragma once

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "GUI.h"

struct Globals
{
    glm::vec2 windowSize;
    glm::vec2 _pad;
    glm::vec4 worldSize;
    glm::mat4x4 proj;
};

class Application
{
public:
    bool onInit();
    void onFrame();
    bool isRunning();
    void onResize(uint32_t width, uint32_t height);
    void onMouseMove(double xpos, double ypos) {}
    void onMouseButton(int button, int action, int mods) {}
    void onScroll(double xoffset, double yoffset) {}

private:
    bool initWindow();
    bool initInstance();
    bool initSurface();
    void initBindGroupLayout();
    bool initBuffers();
    void initBindGroup();
    bool initRenderPipeline();
    bool initCompute();

    void Render();

    const uint32_t m_kWidth = 512;
    const uint32_t m_kHeight = 512;

    GLFWwindow *m_window;
    wgpu::Instance m_instance;
    wgpu::Adapter m_adapter;
    wgpu::Device m_device;
    wgpu::Surface m_surface;
    wgpu::TextureFormat m_format;

    wgpu::Buffer m_vb;
    wgpu::Buffer m_worldbuf;
    wgpu::Buffer m_particleBuffer;
    wgpu::Buffer m_globalBuffer;

    wgpu::BindGroupLayout m_computeBindGroupLayout;
    wgpu::BindGroup m_computeBindGroup;
    wgpu::PipelineLayout m_computePipelineLayout;
    wgpu::ComputePipeline m_computePipeline;

    wgpu::BindGroupLayout m_renderBindGroupLayout;
    wgpu::BindGroup m_renderBindGroup;
    wgpu::RenderPipeline m_pipeline;
    wgpu::RenderPipeline m_pWorld;

    wgpu::BindGroupLayout m_globalBindGroupLayout;
    wgpu::BindGroup m_globalBindGroup;

    GUI m_Gui;
    Globals m_globals;
};