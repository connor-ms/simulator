#pragma once

#include <filesystem>
#include <webgpu/webgpu_cpp.h>

class Util
{
public:
    static wgpu::ShaderModule loadShaderModule(const std::filesystem::path &path, wgpu::Device device);
};