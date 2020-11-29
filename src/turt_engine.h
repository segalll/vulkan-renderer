#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_glfw.h"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <array>
#include <optional>
#include <set>
#include <unordered_map>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct Shader {

};

struct Drawable {
    const char* texture;
    const char* model;

    std::vector<Vertex> vertices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    std::vector<uint32_t> indices;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    UniformBufferObject ubo;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    std::vector<VkDescriptorSet> descriptorSets;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    uint32_t mipLevels;
    VkImageView textureImageView;
    VkSampler textureSampler;

    void (*update)(Drawable* self);
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Camera {
    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 up;
    float yaw;
    float pitch;
    glm::mat4 viewMatrix;
};

class VulkanEngine {
public:
    void run();

private:
    GLFWwindow* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_8_BIT;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    std::vector<Drawable> objects;

    VkDescriptorPool descriptorPool;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    Camera camera;

    bool firstMouse = true;

    double lastX = 400;
    double lastY = 300;

    bool framebufferResized = false;

    void initWindow();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);

    void processInputs();

    void initVulkan();

    void mainLoop();

    void cleanupSwapchain();

    void cleanup();

    void recreateSwapchain();

    void createInstance();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();

    void createSurface();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSwapchain();

    void createImageViews();

    void createRenderPass();

    void createDescriptorSetLayout();

    void createGraphicsPipeline();

    void createFramebuffers();

    void createCommandPool();

    void createColorResources();

    void createDepthResources();

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    VkFormat findDepthFormat();

    void createTextureImage(Drawable& object);

    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    VkSampleCountFlagBits getMaxUsableSampleCount();

    void createTextureImageView(Drawable& object);

    void createTextureSampler(Drawable& object);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void loadModel(Drawable& object);

    void createVertexBuffer(Drawable& object);

    void createIndexBuffer(Drawable& object);

    void createUniformBuffers(Drawable& object);

    void createDescriptorPool();

    void createDescriptorSets(Drawable& object);

    VkCommandBuffer beginSingleTimeCommands();

    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void createCommandBuffers();

    void recordCommandBuffer(uint32_t currentImage);

    void createSyncObjects();

    void updateUniformBuffer(uint32_t currentImage);

    void drawFrame();

    void createDrawable(const char* texture, const char* model, void (*updateFunc)(Drawable* self));

    VkShaderModule createShaderModule(const std::vector<char>& code);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities);

    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDeviceIn);

    bool isDeviceSuitable(VkPhysicalDevice physicalDeviceIn);

    bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDeviceIn);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDeviceIn);

    std::vector<const char*> getRequiredExtensions();

    bool checkValidationLayerSupport();

    std::vector<char> readFile(const char* filename);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};