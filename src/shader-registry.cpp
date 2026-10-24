#include "shader-registry.hpp"

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include "device-context.hpp"
#include "util.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <ranges>


namespace vkdemo {

    ShaderRegistry::ShaderRegistry(const DeviceContext &device_context) : m_device_context(device_context) {}

    ShaderRegistry::~ShaderRegistry() {

        for (const auto &module: m_slang_shader_modules | std::views::values) {
            m_device_context.Device.destroyShaderModule(module.Module);
        }

        slang::shutdown();
        glslang::FinalizeProcess();
    }

    void ShaderRegistry::Initialize() {
        // Initialize Glslang
        glslang::InitializeProcess();

        // Initialize Slang
        slang::createGlobalSession(m_slang_global_session.writeRef());

        // Create the slang session to compile shader
        slang::TargetDesc target_desc;
        target_desc.format  = SLANG_SPIRV;
        target_desc.profile = m_slang_global_session->findProfile("spirv_1_6"); // guaranteed supported version by Vulkan 1.4

        slang::SessionDesc session_desc{};
        session_desc.targets                 = &target_desc;
        session_desc.targetCount             = 1;
        session_desc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

        std::array options = {
                slang::CompilerOptionEntry{slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1}},
                slang::CompilerOptionEntry{slang::CompilerOptionName::MatrixLayoutColumn, {slang::CompilerOptionValueKind::Int, 1}},
        };

        session_desc.compilerOptionEntries    = options.data();
        session_desc.compilerOptionEntryCount = options.size();

        check(m_slang_global_session->createSession(session_desc, m_vulkan_session.writeRef()) >= 0, "Failed to create vulkan slang session");
    }

    SlangShaderModule *ShaderRegistry::GetSlangModule(const std::string &name) {
        if (m_slang_shader_modules.contains(name) == false)
            return nullptr;

        return &m_slang_shader_modules.at(name);
    }

    GlslShaderModule *ShaderRegistry::GetGlslModule(const std::string &name) {
        if (m_glsl_shader_modules.contains(name) == false)
            return nullptr;

        return &m_glsl_shader_modules.at(name);
    }

    void ShaderRegistry::LoadShaders() {
        for (const auto &shader_filename: kShaderFilenames) {
            std::filesystem::path shader_filepath = std::filesystem::path(kShaderFolder) / shader_filename;

            if (shader_filepath.extension() == ".slang")
                load_slang(shader_filepath);
            else
                load_glsl(shader_filepath);
        }
    }

    void ShaderRegistry::load_glsl(const std::filesystem::path &filepath) {
        static constexpr int kGLSLVersion = 450;
        const std::string &extension = filepath.extension().string();

        std::string source{};
        try {
            std::ifstream file{filepath};
            if (!file.is_open()) {
                std::cerr << std::format("Could not load shader file {}.", filepath.string());
                return;
            }

            source.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        } catch (const std::exception &exception) {
            std::cerr << std::format("Could not load shader file {}. Error: {}", filepath.string(), exception.what());
            return;
        }

        EShLanguage language;
        if (extension == ".vert")
            language = EShLangVertex;
        else if (extension == ".frag")
            language = EShLangFragment;
        else if (extension == ".comp")
            language = EShLangCompute;
        else {
            std::cerr << std::format("Unrecognized GLSL file extension. Shader file {} could not be loaded.", filepath.string());
            return;
        }

        glslang::TShader shader{language};
        shader.setSourceFile(source.data());
        // shader.setStrings(reinterpret_cast<const char *const *>(source.data()), 1);

        shader.setEnvInput(glslang::EShSource::EShSourceGlsl, language, glslang::EShClient::EShClientVulkan, kGLSLVersion);
        shader.setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_4);
        shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_6);

        TBuiltInResource res{};
        if (shader.parse(&res, kGLSLVersion, true, EShMsgDefault) == false) {
            std::cerr << std::format("Failed to parse GLSL shader {}.", filepath.string());
        }

        glslang::TProgram program{};
        program.addShader(&shader);
        if (program.link(EShMessages::EShMsgDefault) == false) {
            std::cerr << std::format("Failed to parse GLSL shader {}: {}", filepath.string(), program.getInfoLog());
        }

        glslang::TIntermediate* intermediate = program.getIntermediate(language);
        GlslShaderModule glsl_module{};

        // todo set options
        glslang::SpvOptions options{};
        glslang::GlslangToSpv(*intermediate, glsl_module.Binary, &options);

        vk::ShaderModuleCreateInfo shader_module_ci{};
        shader_module_ci.codeSize = glsl_module.Binary.size();
        shader_module_ci.pCode    = glsl_module.Binary.data();

        glsl_module.Module = m_device_context.Device.createShaderModule(shader_module_ci, nullptr);

        const std::string          &filename = filepath.filename().string();
        check(glsl_module.Module != VK_NULL_HANDLE, "Failed to create shader module for shader " + filename);
        m_glsl_shader_modules.emplace(filename, std::move(glsl_module));
    }

    void ShaderRegistry::load_slang(const std::filesystem::path &filepath) {
        const std::string          &filename = filepath.filename().string();
        SlangShaderModule           slang_module{};
        Slang::ComPtr<slang::IBlob> diagnostics_blob;

        try {
            check(std::filesystem::exists(filepath), std::format("Shader {} is missing!", filepath.string()));
        } catch (const std::exception &exception) {
            std::cerr << "Error while checking if shader file exists: " << exception.what() << std::endl;
        }

        slang_module.SlangModule = m_vulkan_session->loadModuleFromSource(filename.c_str(), filepath.string().c_str(), nullptr, diagnostics_blob.writeRef());
        print_diagnostics(diagnostics_blob, filename);
        check(slang_module.SlangModule != nullptr, "Failed to load module for " + filename);

        SlangResult result = slang_module.SlangModule->link(slang_module.LinkedComponent.writeRef(), diagnostics_blob.writeRef());
        print_diagnostics(diagnostics_blob, filename);
        check(result >= 0, "Failed to link components for " + filename);

        result = slang_module.LinkedComponent->getTargetCode(0, slang_module.SPIRV.writeRef(), diagnostics_blob.writeRef());
        print_diagnostics(diagnostics_blob, filename);
        check(result >= 0, "Failed to get target code for " + filename);

        vk::ShaderModuleCreateInfo shader_module_ci{};
        shader_module_ci.codeSize = slang_module.SPIRV->getBufferSize();
        shader_module_ci.pCode    = static_cast<uint32_t const *>(slang_module.SPIRV->getBufferPointer());

        slang_module.Module = m_device_context.Device.createShaderModule(shader_module_ci, nullptr);
        check(slang_module.Module != VK_NULL_HANDLE, "Failed to create shader module for shader " + filename);

        m_slang_shader_modules.emplace(filename, std::move(slang_module));
        std::cout << "Loaded Slang shader " << filename << std::endl;
    }

    void ShaderRegistry::print_diagnostics(const Slang::ComPtr<slang::IBlob> &blob, const std::string_view shader_name) {
        if (blob != nullptr) {
            const std::string message(static_cast<const char *>(blob->getBufferPointer()));
            std::cout << std::format("Slang diagnostic for shader program {}:\n{}", shader_name, message);
        }
    }
} // namespace vkdemo
