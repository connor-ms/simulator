#include "Util.h"

#include <iostream>
#include <fstream>

wgpu::ShaderModule Util::loadShaderModule(const std::filesystem::path &path, wgpu::Device device)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open shader file " << path << std::endl;
        return nullptr;
    }

    // Read contents of file
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::string shaderSource(size, ' ');
    file.seekg(0);
    file.read(shaderSource.data(), size);

    // Create shader from them
    wgpu::ShaderSourceWGSL wgsl{{.code = shaderSource.c_str()}};
    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgsl};

    return device.CreateShaderModule(&shaderModuleDescriptor);
}