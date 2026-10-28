#ifndef SHADER_REGISTRY_HPP
#define SHADER_REGISTRY_HPP

#include <filesystem>
#include <glslang/Public/ResourceLimits.h>
#include <slang/slang.h>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "slang/slang-com-ptr.h"

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

    /// @brief Class responsible for loading shaders and creating vulkan shader modules. Singleton-esque usage. Supports Slang and Vulkan GLSL.
    class ShaderRegistry {
    public:
        ShaderRegistry(const DeviceContext &device_context);
        virtual ~ShaderRegistry();

        /// @brief Initializes the shader loading libraries
        void Initialize();

        /// @brief Returns a Slang shader module, given its filename.
        /// @param name The name of the shader module
        /// @return A pointer to the module, if it exists. Nullptr otherwise.
        [[nodiscard]] SlangShaderModule *GetSlangModule(const std::string &name);

        /// @brief Returns a GLSL shader module, given its filename.
        /// @param name The name of the shader module
        /// @return A pointer to the module, if it exists. Nullptr otherwise.
        [[nodiscard]] GlslShaderModule *GetGlslModule(const std::string &name);

        /// @brief Loads the application's shaders.
        void LoadShaders();

    private:
        static constexpr bool             kOptimizeGLSL = true;
        static constexpr std::string_view kShaderFolder{"shader"};

        /// @brief Holds all of the expected shader files by the application
        const std::vector<std::string> kShaderFilenames{"particle-compute.slang", "particle-draw.slang", "postprocess.slang", "particle-compute.comp",
                                                        "particle-draw.vert",     "particle-draw.frag",  "dual-kawase-blur-down.comp",  "dual-kawase-blur-up.comp"};


        /// @brief Loads a GLSL shader, given its file path.
        /// @param filepath The shader file path.
        /// @param built_in_resources The glslang BuiltInResource array (the glslang defaults are always passed)
        void load_glsl(const std::filesystem::path &filepath, const TBuiltInResource *built_in_resources);

        /// @brief Loads a Slang shader, given its file path.
        /// @param filepath The shader file path.
        void load_slang(const std::filesystem::path &filepath);

        /// @brief Prints Slang diagnostics regarding a shader.
        /// @param blob The Slang blob.
        /// @param shader_name The shader name.
        static void print_diagnostics(const Slang::ComPtr<slang::IBlob> &blob, std::string_view shader_name);

        const DeviceContext &m_device_context;
        bool                 m_initialized = false;

        Slang::ComPtr<slang::IGlobalSession> m_slang_global_session{};
        Slang::ComPtr<slang::ISession>       m_vulkan_session{};

        std::unordered_map<std::string, SlangShaderModule> m_slang_shader_modules{};
        std::unordered_map<std::string, GlslShaderModule>  m_glsl_shader_modules{};
    };
} // namespace vkdemo

#endif // SHADER_REGISTRY_HPP
