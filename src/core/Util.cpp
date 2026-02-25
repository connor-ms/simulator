#include "Util.h"

#include <iostream>
#include <fstream>

wgpu::ShaderModule Util::loadShaderModule(const std::filesystem::path &path, wgpu::Device device, bool includeGlobals)
{
    std::string globalText = "";
    if (includeGlobals)
    {
        std::ifstream globals(SHADER_DIR "/globals.wgsl");
        if (!globals.is_open())
        {
            std::cout << "Failed to open globals shader!" << std::endl;
            return nullptr;
        }

        globals.seekg(0, std::ios::end);
        size_t size = globals.tellg();
        globalText = std::string(size, ' ');
        globals.seekg(0);
        globals.read(globalText.data(), size);
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cout << "Failed to open shader file " << path << std::endl;
        return nullptr;
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::string code(size, ' ');
    file.seekg(0);
    file.read(code.data(), size);

    std::string fullSource = globalText + code;

    wgpu::ShaderSourceWGSL wgsl{{.code = fullSource.c_str()}};
    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgsl};

    return device.CreateShaderModule(&shaderModuleDescriptor);
}