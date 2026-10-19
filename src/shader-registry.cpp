#include "shader-registry.hpp"

#include "device-context.hpp"
#include "util.hpp"

#include <array>
#include <filesystem>
#include <ranges>

namespace vkdemo {
    ShaderRegistry::ShaderRegistry(const DeviceContext &device_context) : m_device_context(device_context) {}

    ShaderRegistry::~ShaderRegistry() {
        for (const auto& module : m_shader_modules | std::views::values) {
            m_device_context.Device.destroyShaderModule(module.Module);
        }
    }

    void ShaderRegistry::Initialize() {
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

    ShaderModule *ShaderRegistry::GetModule(const std::string &name) {
        if (m_shader_modules.contains(name) == false)
            return nullptr;

        return &m_shader_modules.at(name);
    }

    void ShaderRegistry::LoadShaders() {
        for (const auto &shader_filename: kShaderFilenames) {
            ShaderModule                slang_module{};
            std::filesystem::path       shader_filepath = std::filesystem::path(kShaderFolder) / shader_filename;
            Slang::ComPtr<slang::IBlob> diagnostics_blob;

            try {
                check(std::filesystem::exists(shader_filepath), std::format("Shader {} is missing!", shader_filename));
            } catch (const std::exception& exception) {
                std::cerr << "Error while checking if shader file exists: " << exception.what() << std::endl;
            }

            slang_module.SlangModule = m_vulkan_session->loadModuleFromSource(shader_filename.c_str(), shader_filepath.string().c_str(), nullptr, diagnostics_blob.writeRef());
            print_diagnostics(diagnostics_blob, shader_filename);
            check(slang_module.SlangModule != nullptr, "Failed to load module for " + shader_filename);

            SlangResult result = slang_module.SlangModule->link(slang_module.LinkedComponent.writeRef(), diagnostics_blob.writeRef());
            print_diagnostics(diagnostics_blob, shader_filename);
            check(result >= 0, "Failed to link components for " + shader_filename);

            result = slang_module.LinkedComponent->getTargetCode(0, slang_module.SPIRV.writeRef(), diagnostics_blob.writeRef());
            print_diagnostics(diagnostics_blob, shader_filename);
            check(result >= 0, "Failed to get target code for " + shader_filename);

            vk::ShaderModuleCreateInfo shader_module_ci{};
            shader_module_ci.codeSize = slang_module.SPIRV->getBufferSize();
            shader_module_ci.pCode = static_cast<uint32_t const*>(slang_module.SPIRV->getBufferPointer());

            slang_module.Module = m_device_context.Device.createShaderModule(shader_module_ci, nullptr);
            check(slang_module.Module != VK_NULL_HANDLE, "Failed to create shader module for shader " + shader_filename);

            m_shader_modules.emplace(shader_filename, std::move(slang_module));
            std::cout << "Loaded shader " << shader_filename << std::endl;
        }
    }

    void ShaderRegistry::print_diagnostics(const Slang::ComPtr<slang::IBlob> &blob, const std::string_view shader_name) {
        if (blob != nullptr) {
            const std::string message(static_cast<const char *>(blob->getBufferPointer()));
            std::cout << std::format("Slang diagnostic for shader program {}:\n{}", shader_name, message);
        }
    }
} // namespace vkdemo
