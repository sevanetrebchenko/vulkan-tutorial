
#ifndef VULKAN_TUTORIAL_SWAP_CHAIN_SUPPORT_DATA_H
#define VULKAN_TUTORIAL_SWAP_CHAIN_SUPPORT_DATA_H

#include <vulkan/vulkan.h>
#include <vector>

namespace VT {
    
    struct SwapChainSupportData {
        VkSurfaceCapabilitiesKHR surfaceCapabilities_;
        std::vector<VkSurfaceFormatKHR> formats_;
        std::vector<VkPresentModeKHR> presentationModes_;
    };
    
}

#endif //VULKAN_TUTORIAL_SWAP_CHAIN_SUPPORT_DATA_H
