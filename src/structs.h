#ifndef structs_h
#define structs_h

#include <vulkan/vulkan.h>
#include <cglm/cglm.h>

typedef struct QueueFamilyIndex {
    uint32_t index;
    bool set;
} QueueFamilyIndex;

void setQueueFamilyIndex(QueueFamilyIndex* qfi, uint32_t newIndex);

typedef struct QueueFamilyIndices {
    QueueFamilyIndex graphicsFamily;
    QueueFamilyIndex presentFamily;
} QueueFamilyIndices;

bool queueFamilyIndicesAreComplete(QueueFamilyIndices indices);

typedef struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    int formatCount;
    VkPresentModeKHR* presentModes;
    int presentModeCount;
} SwapchainSupportDetails;

typedef struct UniformBufferObject {
    CGLM_ALIGN(16) mat4 model;
    CGLM_ALIGN(16) mat4 view;
    CGLM_ALIGN(16) mat4 projection;
} UniformBufferObject;

typedef struct DrawableObject {
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSet* descriptorSets;
    VkBuffer* uniformBuffers;
    VkDeviceMemory* uniformBuffersMemory;
    UniformBufferObject ubo;
    char* shader;
} DrawableObject;

typedef struct bmchar {
    int x, y;
    int width;
    int height;
    int xoffset;
    int yoffset;
    int xadvance;
    int page;
} bmchar;

typedef struct vertex {
    float pos[3];
    float tex[2];
} vertex;

typedef struct texture {
    VkSampler sampler;
    VkImage image;
    VkImageLayout imageLayout;
    VkDeviceMemory deviceMemory;
    VkImageView view;
    uint32_t width, height;
    uint32_t mipLevels;
} texture;

#endif /* structs_h */
