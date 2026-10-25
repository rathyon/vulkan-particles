#ifndef SHADER_REGISTRY_HPP
#define SHADER_REGISTRY_HPP

#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <slang/slang.h>
#include "slang/slang-com-ptr.h"
#include <glslang/Public/ResourceLimits.h>

#include <vulkan/vulkan.hpp>

namespace vkdemo {
    class DeviceContext;

    struct SlangShaderModule {
        Slang::ComPtr<slang::IModule>        SlangModule{};
        Slang::ComPtr<ISlangBlob>            SPIRV{};
        Slang::ComPtr<slang::IComponentType> LinkedComponent{};
        vk::ShaderModule                     Module{};
    };

    struct GlslShaderModule {
        vk::ShaderModule      Module{};
        std::vector<uint32_t> Binary{};
    };

    class ShaderRegistry {
    public:
        ShaderRegistry(const DeviceContext &device_context);
        virtual ~ShaderRegistry();

        void                             Initialize();
        [[nodiscard]] SlangShaderModule *GetSlangModule(const std::string &name);
        [[nodiscard]] GlslShaderModule  *GetGlslModule(const std::string &name);
        void                             LoadShaders();

    private:
        static constexpr bool             kOptimizeGLSL = true;
        static constexpr std::string_view kShaderFolder{"shader"};

        const std::vector<std::string> kShaderFilenames{"particle-compute.slang", "particle-draw.slang", "postprocess.slang", "particle-compute.comp",
                                                        "particle-draw.vert",     "particle-draw.frag",  "postprocess.vert",  "postprocess.frag"};

        void        load_glsl(const std::filesystem::path &filepath, const TBuiltInResource* built_in_resources);
        void        load_slang(const std::filesystem::path &filepath);
        static void print_diagnostics(const Slang::ComPtr<slang::IBlob> &blob, std::string_view shader_name);

        const DeviceContext &m_device_context;

        Slang::ComPtr<slang::IGlobalSession> m_slang_global_session{};
        Slang::ComPtr<slang::ISession>       m_vulkan_session{};

        std::unordered_map<std::string, SlangShaderModule> m_slang_shader_modules{};
        std::unordered_map<std::string, GlslShaderModule>  m_glsl_shader_modules{};
    };
} // namespace vkdemo

#endif // SHADER_REGISTRY_HPP
