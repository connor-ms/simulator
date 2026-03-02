#pragma once

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include <GLFW/glfw3.h>

#include "../core/GPUContext.h"

class GUI
{
public:
    bool init(GPUContext *ctx, GLFWwindow *window);
    void update(wgpu::CommandEncoder encoder);

private:
    GPUContext *m_ctx;
};