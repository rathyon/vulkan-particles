#ifndef SHADER_LIBRARY_HPP
#define SHADER_LIBRARY_HPP

#include <string_view>
#include <unordered_map>
#include <vector>

#include <slang/slang.h>
#include "slang/slang-com-ptr.h"

#include <vulkan/vulkan.hpp>

namespace vkdemo {
    class DeviceContext;

    class ShaderLibrary {
    public:
        ShaderLibrary(const DeviceContext &device_context);
        virtual ~ShaderLibrary();

        void Initialize();
        void LoadShaders();

    private:
        static constexpr std::string_view kShaderFolder{"shaders"};
        const std::vector<std::string>    kShaderFilenames{"fullscreen.slang", "test-shader.comp.slang"};

        struct ShaderModule {
            Slang::ComPtr<slang::IModule> SlangModule{};
            Slang::ComPtr<ISlangBlob>     SPIRV{};
            vk::ShaderModule              Module{};
        };

        static void print_diagnostics(const Slang::ComPtr<slang::IBlob> &blob, const std::string_view shader_name);

        const DeviceContext &m_device_context;

        Slang::ComPtr<slang::IGlobalSession> m_slang_global_session{};
        Slang::ComPtr<slang::ISession>       m_vulkan_session{};

        std::unordered_map<std::string, ShaderModule> m_shader_modules{};
    };
} // namespace vkdemo

#endif // SHADER_LIBRARY_HPP
