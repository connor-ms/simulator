#pragma once

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#include "../gui/GUI.h"
#include "../simulation/Simulator.h"
#include "../render/Render.h"
#include "GPUContext.h"

class Application
{
public:
    void init();
    void onFrame();

    bool isRunning();
    void onResize(uint32_t width, uint32_t height);
    void onMouseMove(double xpos, double ypos);
    void onMouseButton(int button, int action, int mods);

private:
    void initWindow();
    void initInstance();
    void initSurface();
    void initGlobals();
    void initBindGroupLayout();
    void initBuffers();
    void initBindGroup();

    GLFWwindow *m_window;
    GPUContext m_ctx;
    Simulator m_sim;
    Renderer m_renderer;
    GUI m_gui;

    bool m_mouseDown;
    glm::vec2 m_mousePos;

    const uint32_t m_kWidth = 1280;
    const uint32_t m_kHeight = 720;
};