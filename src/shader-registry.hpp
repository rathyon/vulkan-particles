#ifndef SHADER_REGISTRY_HPP
#define SHADER_REGISTRY_HPP

#include <string_view>
#include <unordered_map>
#include <vector>

#include <slang/slang.h>
#include "slang/slang-com-ptr.h"

#include <vulkan/vulkan.hpp>

namespace vkdemo {
    class DeviceContext;

    struct ShaderModule {
        Slang::ComPtr<slang::IModule>        SlangModule{};
        Slang::ComPtr<ISlangBlob>            SPIRV{};
        Slang::ComPtr<slang::IComponentType> LinkedComponent{};
        vk::ShaderModule                     Module{};
    };

    class ShaderRegistry {
    public:
        ShaderRegistry(const DeviceContext &device_context);
        virtual ~ShaderRegistry();

        void                        Initialize();
        [[nodiscard]] ShaderModule *GetModule(const std::string &name);
        void                        LoadShaders();

    private:
        static constexpr std::string_view kShaderFolder{"shaders"};
        const std::vector<std::string>    kShaderFilenames{"fullscreen.slang", "simple-compute.slang"};


        static void print_diagnostics(const Slang::ComPtr<slang::IBlob> &blob, const std::string_view shader_name);

        const DeviceContext &m_device_context;

        Slang::ComPtr<slang::IGlobalSession> m_slang_global_session{};
        Slang::ComPtr<slang::ISession>       m_vulkan_session{};

        std::unordered_map<std::string, ShaderModule> m_shader_modules{};
    };
} // namespace vkdemo

#endif // SHADER_REGISTRY_HPP
