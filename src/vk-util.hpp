#ifndef VKUTIL_HPP
#define VKUTIL_HPP

#include <vulkan/vulkan.hpp>

namespace vkdemo {
    namespace vkutil {
        static void InferSrcStageAndAccessMasks(vk::ImageMemoryBarrier2 &barrier) {
            // Code from Sascha Willems: https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp

            // Source layouts (old)
            // Source access mask controls actions that have to be finished on the old layout
            // before it will be transitioned to the new layout
            switch (barrier.oldLayout) {
                case vk::ImageLayout::eUndefined:
                    // Image layout is undefined (or does not matter)
                    // Only valid as initial layout
                    // No flags required, listed only for completeness
                    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
                    break;

                case vk::ImageLayout::ePreinitialized:
                    // Image is preinitialized
                    // Only valid as initial layout for linear images, preserves memory contents
                    // Make sure host writes have been finished
                    barrier.srcAccessMask = vk::AccessFlagBits2::eHostWrite;
                    barrier.srcStageMask |= vk::PipelineStageFlagBits2::eHost;
                    break;

                case vk::ImageLayout::eColorAttachmentOptimal:
                    // Image is a color attachment
                    // Make sure any writes to the color buffer have been finished
                    barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
                    barrier.srcStageMask |= vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                    break;

                case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                    // Image is a depth/stencil attachment
                    // Make sure any writes to the depth/stencil buffer have been finished
                    barrier.srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
                    break;

                case vk::ImageLayout::eTransferSrcOptimal:
                    // Image is a transfer source
                    // Make sure any reads from the image have been finished
                    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferRead;
                    barrier.srcStageMask |= vk::PipelineStageFlagBits2::eAllTransfer;
                    break;

                case vk::ImageLayout::eTransferDstOptimal:
                    // Image is a transfer destination
                    // Make sure any writes to the image have been finished
                    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
                    barrier.srcStageMask |= vk::PipelineStageFlagBits2::eAllTransfer;
                    break;

                case vk::ImageLayout::eShaderReadOnlyOptimal:
                    // Image is read by a shader
                    // Make sure any shader reads from the image have been finished
                    barrier.srcAccessMask = vk::AccessFlagBits2::eShaderRead;
                    barrier.srcStageMask |= vk::PipelineStageFlagBits2::eAllGraphics;
                    break;

                case vk::ImageLayout::ePresentSrcKHR:
                    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
                    barrier.srcStageMask |= vk::PipelineStageFlagBits2::eBottomOfPipe;
                    break;
                default:
                    // Other source layouts aren't handled (yet)
                    assert(false && "InferSrcStageAndAccessMasks not implemented for given layout.");
                    break;
            }
        }

        static void InferDstStageAndAccessMasks(vk::ImageMemoryBarrier2 &barrier) {
            // Code from Sascha Willems: https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
            // Target layouts (new)
            // Destination access mask controls the dependency for the new image layout
            switch (barrier.newLayout) {
                case vk::ImageLayout::eTransferDstOptimal:
                    // Image will be used as a transfer destination
                    // Make sure any writes to the image have been finished
                    barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
                    barrier.dstStageMask |= vk::PipelineStageFlagBits2::eAllTransfer;
                    break;

                case vk::ImageLayout::eTransferSrcOptimal:
                    // Image will be used as a transfer source
                    // Make sure any reads from the image have been finished
                    barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead;
                    barrier.dstStageMask |= vk::PipelineStageFlagBits2::eAllTransfer;
                    break;

                case vk::ImageLayout::eColorAttachmentOptimal:
                    // Image will be used as a color attachment
                    // Make sure any writes and reads to/from the color buffer have been finished
                    barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;

                    // Make image available from output stage and onwards
                    barrier.dstStageMask |= vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                    break;

                case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                    // Image layout will be used as a depth/stencil attachment
                    // Make sure any writes to depth/stencil buffer have been finished
                    barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;

                    // Make image available from early fragment test stage and onwards
                    barrier.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;
                    break;

                case vk::ImageLayout::eShaderReadOnlyOptimal:
                    // Image will be read in a shader (sampler, input attachment)
                    // Make sure any writes to the image have been finished
                    barrier.srcAccessMask |= vk::AccessFlagBits2::eHostWrite | vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eColorAttachmentWrite;

                    // Make barrier depend on these stages finishing (host and transfers)
                    barrier.srcStageMask |= vk::PipelineStageFlagBits2::eHost | vk::PipelineStageFlagBits2::eAllTransfer | vk::PipelineStageFlagBits2::eColorAttachmentOutput;

                    barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;

                    // Make image available from vertex stage and onwards
                    barrier.dstStageMask |= vk::PipelineStageFlagBits2::eVertexShader;
                    break;
                case vk::ImageLayout::ePresentSrcKHR:
                    barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
                    barrier.dstStageMask |= vk::PipelineStageFlagBits2::eBottomOfPipe;
                    break;
                default:
                    // Other source layouts aren't handled (yet)
                    assert(false && "InferDstStageAndAccessMasks not implemented for given layout.");
                    break;
            }
        }
    } // namespace vkutil
} // namespace vkdemo

#endif // VKUTIL_HPP
