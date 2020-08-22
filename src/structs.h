#ifndef structs_h
#define structs_h

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct QueueFamilyIndex {
    uint32_t index;
    bool set = false;
    
    void setIndex(uint32_t newIndex) {
        index = newIndex;
        set = true;
    }
};

struct QueueFamilyIndices {
    QueueFamilyIndex graphicsFamily;
    QueueFamilyIndex presentFamily;

    bool isComplete() {
        return graphicsFamily.set && presentFamily.set;
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    int formatCount;
    VkPresentModeKHR* presentModes;
    int presentModeCount;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

struct DrawableObject {
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSet* descriptorSets;
    VkBuffer* uniformBuffers;
    VkDeviceMemory* uniformBuffersMemory;
    UniformBufferObject ubo;
    char* shader;
};

#endif /* structs_h */
