#if defined(WIN32)
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#elif __APPLE__
#include <MoltenVK/mvk_vulkan.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"

#include <GLFW/glfw3.h>

#pragma clang diagnostic pop
#endif

#include <cglm/cglm.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "hashtable.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const uint32_t validationLayersCount = 1;
const char* validationLayers[1] = {
    "VK_LAYER_KHRONOS_validation"
};

const int MAX_FRAMES_IN_FLIGHT = 2;

const uint32_t deviceExtensionsCount = 1;
const char* deviceExtensions[1] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

GLFWwindow* window;

VkInstance instance;
VkDebugUtilsMessengerEXT debugMessenger;
VkSurfaceKHR surface;

VkPhysicalDevice physicalDevice;
VkDevice device;

VkQueue graphicsQueue;
VkQueue presentQueue;

VkSwapchainKHR oldSwapchain;
VkSwapchainKHR swapchain;
VkImage* swapchainImages;
uint32_t swapchainImageCount;
VkFormat swapchainImageFormat;
VkExtent2D swapchainExtent;
VkImageView* swapchainImageViews;
VkFramebuffer* swapchainFramebuffers;

VkRenderPass renderPass;
VkDescriptorSetLayout descriptorSetLayout;

VkCommandPool commandPool;

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

VkBuffer textVertexBuffer;
VkDeviceMemory textVertexBufferMemory;
VkBuffer textIndexBuffer;
VkDeviceMemory textIndexBufferMemory;
uint32_t textIndexCount;

VkDescriptorPool descriptorPool;

VkCommandBuffer* commandBuffers;

VkSemaphore* imageAvailableSemaphores;
VkSemaphore* renderFinishedSemaphores;
VkFence* inFlightFences;
VkFence* imagesInFlight;
size_t currentFrame = 0;

bool framebufferResized = false;

const uint32_t objectCount = 1;
DrawableObject objects[1];

HashTable* objectShaders;

bmchar fontChars[255];
bmchar otherFontChars[255];

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        func(instance, debugMessenger, pAllocator);
    }
}

VkVertexInputBindingDescription getVertexBindingDescription() {
    VkVertexInputBindingDescription bindingDescription;
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

VkVertexInputAttributeDescription* getVertexAttributeDescriptions(int* attributeDescriptionCount) {
    *attributeDescriptionCount = 1;

    VkVertexInputAttributeDescription* attributeDescriptions = malloc(1 * sizeof(VkVertexInputAttributeDescription));
    if (attributeDescriptions == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0 * sizeof(float);

    return attributeDescriptions;
}

const vertex vertices[] = {
    { { -0.5f, -0.5f, 0.0f } },
    { { 0.5f, -0.5f, 0.0f } },
    { { 0.5f, 0.5f, 0.0f } },
    { { -0.5f, 0.5f, 0.0f } }
};

const uint32_t indices[] = {
    0, 1, 2, 2, 3, 0
};

bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties* availableLayers = malloc(layerCount * sizeof(VkLayerProperties));
    if (availableLayers == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (uint32_t i = 0; i < validationLayersCount; i++) {
        bool layerFound = false;

        for (uint32_t j = 0; j < layerCount; j++) {
            if (strcmp(validationLayers[i], availableLayers[j].layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            free(availableLayers);
            return false;
        }
    }

    free(availableLayers);
    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    printf("validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    framebufferResized = true;
}

void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo) {
    *createInfo = (VkDebugUtilsMessengerCreateInfoEXT){ 0 };
    createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo->pfnUserCallback = debugCallback;
}

const char** getRequiredExtensions(uint32_t* extensionCount) {
    uint32_t glfwExtensionCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    if (enableValidationLayers) {
        const char** tempExtensions = malloc((glfwExtensionCount + 1) * sizeof(const char*));
        if (tempExtensions == NULL) {
            printf("ran out of memory\n");
            exit(-1);
        }
        for (uint32_t i = 0; i < glfwExtensionCount; i++) {
            tempExtensions[i] = extensions[i];
        }
        tempExtensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        *extensionCount = glfwExtensionCount + 1;
        return tempExtensions;
    }
    *extensionCount = glfwExtensionCount;
    return extensions;
}

void createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        printf("validation layers requested, but not available\n");
        exit(-1);
    }

    VkApplicationInfo appInfo = (VkApplicationInfo){ 0 };
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = (VkInstanceCreateInfo){ 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t extensionCount = 0;
    const char** extensions = getRequiredExtensions(&extensionCount);

    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;
    
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = validationLayersCount;
        createInfo.ppEnabledLayerNames = validationLayers;

        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = NULL;
    }
    
    if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS) {
        printf("failed to create instance\n");
        exit(-1);
    }

    if (enableValidationLayers) {
        free(extensions);
    }
}

void setupDebugMessenger() {
    if (!enableValidationLayers) {
        debugMessenger = NULL;
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(&createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, NULL, &debugMessenger) != VK_SUCCESS) {
        printf("failed to set up debug messenger\n");
        exit(-1);
    }
}

void createSurface() {
    if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
        printf("failed to create window surface\n");
        exit(-1);
    }
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices = (QueueFamilyIndices){ 0 };

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties* queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    if (queueFamilies == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            setQueueFamilyIndex(&indices.graphicsFamily, i);
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            setQueueFamilyIndex(&indices.presentFamily, i);
        }

        if (queueFamilyIndicesAreComplete(indices)) {
            break;
        }
    }

    free(queueFamilies);

    return indices;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    VkExtensionProperties* availableExtensions = malloc(extensionCount * sizeof(VkExtensionProperties));
    if (availableExtensions == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);
    
    uint32_t found = 0;
    
    for (uint32_t i = 0; i < extensionCount; i++) {
        for (uint32_t j = 0; j < deviceExtensionsCount; j++) {
            if (strcmp(availableExtensions[i].extensionName, deviceExtensions[j]) == 0) {
                found++;
            }
            if (found >= deviceExtensionsCount) {
                return true;
            }
        }
    }

    free(availableExtensions);

    return false;
}

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    SwapchainSupportDetails details = (SwapchainSupportDetails){ 0 };

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);

    if (formatCount != 0) {
        details.formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
        if (details.formats == NULL) {
            printf("ran out of memory\n");
            exit(-1);
        }
        details.formatCount = formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats);
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);

    if (presentModeCount != 0) {
        details.presentModes = malloc(presentModeCount * sizeof(VkPresentModeKHR));
        if (details.presentModes == NULL) {
            printf("ran out of memory\n");
            exit(-1);
        }
        details.presentModeCount = presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes);
    }

    return details;
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapchainAdequate = false;
    if (extensionsSupported) {
        SwapchainSupportDetails swapchainSupport = querySwapchainSupport(device, surface);
        swapchainAdequate = swapchainSupport.formatCount != 0 && swapchainSupport.presentModeCount != 0;
        free(swapchainSupport.formats);
        free(swapchainSupport.presentModes);
    }

    return queueFamilyIndicesAreComplete(indices) && extensionsSupported && swapchainAdequate;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const VkSurfaceFormatKHR* availableFormats, int formatCount) {
    for (uint32_t i = 0; i < formatCount; i++) {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormats[i];
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR* availablePresentModes, int presentModeCount) {
    for (uint32_t i = 0; i < presentModeCount; i++) {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentModes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR* capabilites) {
    if (capabilites->currentExtent.width != UINT32_MAX) {
        return capabilites->currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };

        uint32_t widthMin = capabilites->maxImageExtent.width < actualExtent.width ? capabilites->maxImageExtent.width : actualExtent.width;
        actualExtent.width = widthMin > capabilites->minImageExtent.width ? widthMin : capabilites->minImageExtent.width;
        uint32_t heightMin = capabilites->maxImageExtent.height < actualExtent.height ? capabilites->maxImageExtent.height : actualExtent.height;
        actualExtent.height = heightMin > capabilites->minImageExtent.height ? heightMin : capabilites->minImageExtent.height;

        return actualExtent;
    }
}

void createSwapchain() {
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats, swapchainSupport.formatCount);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes, swapchainSupport.presentModeCount);
    VkExtent2D extent = chooseSwapExtent(&swapchainSupport.capabilities);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = (VkSwapchainCreateInfoKHR){ 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.index, indices.presentFamily.index };

    if (indices.graphicsFamily.index != indices.presentFamily.index) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    oldSwapchain = swapchain;

    createInfo.oldSwapchain = oldSwapchain;

    if (vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain) != VK_SUCCESS) {
        printf("failed to create swapchain\n");
        exit(-1);
    }

    free(swapchainSupport.formats);
    free(swapchainSupport.presentModes);

    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL);
    swapchainImages = malloc(imageCount * sizeof(VkImage));
    if (swapchainImages == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }
    swapchainImageCount = imageCount;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages);

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;
}

void pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

    if (deviceCount == 0) {
        printf("failed to find GPUs with Vulkan support\n");
        exit(-1);
    }

    VkPhysicalDevice* devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    if (devices == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    for (uint32_t i = 0; i < deviceCount; i++) {
        if (isDeviceSuitable(devices[i], surface)) {
            physicalDevice = devices[i];
            break;
        }
    }

    free(devices);

    if (physicalDevice == VK_NULL_HANDLE) {
        printf("failed to find a suitable GPU\n");
        exit(-1);
    }
}

void createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
    
    uint32_t uniqueQueueFamilies[2] = { indices.graphicsFamily.index, indices.presentFamily.index };

    int queueCount = 2;
    if (uniqueQueueFamilies[0] == uniqueQueueFamilies[1]) {
        queueCount = 1;
    }

    VkDeviceQueueCreateInfo* queueCreateInfos = malloc(queueCount * sizeof(VkDeviceQueueCreateInfo));
    if (queueCreateInfos == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }

    float queuePriority = 1.0f;

    for (uint32_t i = 0; i < queueCount; i++) {
        VkDeviceQueueCreateInfo queueCreateInfo = (VkDeviceQueueCreateInfo){ 0 };
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[i] = queueCreateInfo;
    }

    VkPhysicalDeviceFeatures deviceFeatures = (VkPhysicalDeviceFeatures){ 0 };

    VkDeviceCreateInfo createInfo = (VkDeviceCreateInfo){ 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = queueCount;
    createInfo.pQueueCreateInfos = queueCreateInfos;

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = deviceExtensionsCount;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    
    if (vkCreateDevice(physicalDevice, &createInfo, NULL, &device) != VK_SUCCESS) {
        printf("failed to create logical device\n");
        exit(-1);
    }

    free(queueCreateInfos);

    vkGetDeviceQueue(device, indices.graphicsFamily.index, 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.index, 0, &presentQueue);
}

void createImageViews() {
    swapchainImageViews = malloc(swapchainImageCount * sizeof(VkImageView));
    if (swapchainImageViews == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }

    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        VkImageViewCreateInfo createInfo = (VkImageViewCreateInfo){ 0 };
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, NULL, &swapchainImageViews[i]) != VK_SUCCESS) {
            printf("failed to create image views\n");
            exit(-1);
        }
    }
}

static char* readFile(const char* filename, long* filesize) {
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("failed to open file\n");
        exit(-1);
    }

    fseek(fp, 0L, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);
    char* buffer = malloc(sz * sizeof(char));
    if (buffer == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }
    fread(buffer, sizeof(char), sz / sizeof(char), fp);

    fclose(fp);

    *filesize = sz;

    return buffer;
}

VkShaderModule createShaderModule(const char* code, long codeSize, VkDevice device) {
    VkShaderModuleCreateInfo createInfo = (VkShaderModuleCreateInfo){ 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    createInfo.pCode = (const uint32_t*)code;

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        printf("failed to create shader module\n");
        exit(-1);
    }

    return shaderModule;
}

void createRenderPass() {
    VkAttachmentDescription colorAttachment = (VkAttachmentDescription){ 0 };
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = (VkAttachmentReference){ 0 };
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = (VkSubpassDescription){ 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = (VkSubpassDependency){ 0 };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = (VkRenderPassCreateInfo){ 0 };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS) {
        printf("failed to create render pass\n");
        exit(-1);
    }
}

void createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding = (VkDescriptorSetLayoutBinding){ 0 };
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo = (VkDescriptorSetLayoutCreateInfo){ 0 };
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;
    
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayout) != VK_SUCCESS) {
        printf("failed to create descriptor set layout\n");
        exit(-1);
    }
}

void createGraphicsPipeline(VkPipeline* pipeline, VkPipelineLayout* pipelineLayout, const char* shaderName) {
    long vertShaderSize;
    long fragShaderSize;
    char vertShaderPath[50];
    char fragShaderPath[50];
    sprintf(vertShaderPath, "shaders/%s.vert.spv", shaderName);
    sprintf(fragShaderPath, "shaders/%s.frag.spv", shaderName);
    char* vertShaderCode = readFile(vertShaderPath, &vertShaderSize);
    char* fragShaderCode = readFile(fragShaderPath, &fragShaderSize);

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, vertShaderSize, device);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, fragShaderSize, device);

    free(vertShaderCode);
    free(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = (VkPipelineShaderStageCreateInfo){ 0 };
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = (VkPipelineShaderStageCreateInfo){ 0 };
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = (VkPipelineVertexInputStateCreateInfo){ 0 };
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription bindingDescription = getVertexBindingDescription();
    int attributeDescriptionCount;
    VkVertexInputAttributeDescription* attributeDescriptions = getVertexAttributeDescriptions(&attributeDescriptionCount);

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptionCount;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = (VkPipelineInputAssemblyStateCreateInfo){ 0 };
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = (VkViewport){ 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = (VkRect2D){ 0 };
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState = (VkPipelineViewportStateCreateInfo){ 0 };
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = (VkPipelineRasterizationStateCreateInfo){ 0 };
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = (VkPipelineMultisampleStateCreateInfo){ 0 };
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = NULL;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = (VkPipelineColorBlendAttachmentState){ 0 };
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = (VkPipelineColorBlendStateCreateInfo){ 0 };
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = (VkPipelineLayoutCreateInfo){ 0 };
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, pipelineLayout) != VK_SUCCESS) {
        printf("failed to create pipeline layout\n");
        exit(-1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = (VkGraphicsPipelineCreateInfo){ 0 };
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = *pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pipeline) != VK_SUCCESS) {
        printf("failed to create graphics pipeline\n");
        exit(-1);
    }

    free(attributeDescriptions);

    vkDestroyShaderModule(device, vertShaderModule, NULL);
    vkDestroyShaderModule(device, fragShaderModule, NULL);
}

void createFramebuffers() {
    swapchainFramebuffers = malloc(swapchainImageCount * sizeof(VkFramebuffer));
    if (swapchainFramebuffers == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        VkImageView attachments[] = {
            swapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = (VkFramebufferCreateInfo){ 0 };
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapchainFramebuffers[i]) != VK_SUCCESS) {
            printf("failed to create framebuffer\n");
            exit(-1);
        }
    }
}

void createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

    VkCommandPoolCreateInfo poolInfo = (VkCommandPoolCreateInfo){ 0 };
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.index;
    poolInfo.flags = 0;

    if (vkCreateCommandPool(device, &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
        printf("failed to create command pool\n");
        exit(-1);
    }
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    printf("failed to find suitable memory type\n");
    exit(-1);
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory) {
    VkBufferCreateInfo bufferInfo = (VkBufferCreateInfo){ 0 };
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
        printf("failed to create vertex buffer\n");
        exit(-1);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = (VkMemoryAllocateInfo){ 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
        printf("failed to allocation vertex buffer memory\n");
        exit(-1);
    }

    vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo = (VkCommandBufferAllocateInfo){ 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = (VkCommandBufferBeginInfo){ 0 };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    VkBufferCopy copyRegion = (VkBufferCopy){ 0 };
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = (VkSubmitInfo){ 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);
}

void createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices, (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);
}

void createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    
    for (uint32_t i = 0; i < objectCount; i++) {
        objects[i].uniformBuffers = malloc(swapchainImageCount * sizeof(VkBuffer));
        objects[i].uniformBuffersMemory = malloc(swapchainImageCount * sizeof(VkDeviceMemory));
        if (objects[i].uniformBuffers == NULL || objects[i].uniformBuffersMemory == NULL) {
            printf("ran out of memory\n");
            exit(-1);
        }
        
        for (uint32_t j = 0; j < swapchainImageCount; j++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &objects[i].uniformBuffers[j], &objects[i].uniformBuffersMemory[j]);
        }
    }
}

void createDescriptorPool() {
    VkDescriptorPoolSize poolSize;
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = objectCount * swapchainImageCount;
    
    VkDescriptorPoolCreateInfo poolInfo = (VkDescriptorPoolCreateInfo){ 0 };
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = objectCount * swapchainImageCount;
    
    if (vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool) != VK_SUCCESS) {
        printf("failed to create descriptor pool\n");
        exit(-1);
    }
}

void createDescriptorSets(DrawableObject* object) {
    VkDescriptorSetLayout* layouts = malloc(swapchainImageCount * sizeof(VkDescriptorSetLayout));
    if (layouts == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        layouts[i] = descriptorSetLayout;
    }
    VkDescriptorSetAllocateInfo allocInfo = (VkDescriptorSetAllocateInfo){ 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = swapchainImageCount;
    allocInfo.pSetLayouts = layouts;
    
    object->descriptorSets = malloc(swapchainImageCount * sizeof(VkDescriptorSet));
    if (object->descriptorSets == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }
    if (vkAllocateDescriptorSets(device, &allocInfo, object->descriptorSets) != VK_SUCCESS) {
        printf("failed to allocate descriptor sets\n");
        exit(-1);
    }

    free(layouts);
    
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        VkDescriptorBufferInfo bufferInfo = (VkDescriptorBufferInfo){ 0 };
        bufferInfo.buffer = object->uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);
        
        VkWriteDescriptorSet descriptorWrite = (VkWriteDescriptorSet){ 0 };
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = object->descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        
        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);
    }
}

void createCommandBuffers() {
    commandBuffers = malloc(swapchainImageCount * sizeof(VkCommandBuffer));
    if (commandBuffers == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }

    VkCommandBufferAllocateInfo allocInfo = (VkCommandBufferAllocateInfo){ 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = swapchainImageCount;

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers) != VK_SUCCESS) {
        printf("failed to allocate command buffers\n");
        exit(-1);
    }

    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        VkCommandBufferBeginInfo beginInfo = (VkCommandBufferBeginInfo){ 0 };
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            printf("failed to begin recording command buffer");
            exit(-1);
        }

        VkRenderPassBeginInfo renderPassInfo = (VkRenderPassBeginInfo){ 0 };
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapchainFramebuffers[i];
        renderPassInfo.renderArea.offset.x = 0;
        renderPassInfo.renderArea.offset.y = 0;
        renderPassInfo.renderArea.extent = swapchainExtent;

        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        for (uint32_t j = 0; j < objectCount; j++) {
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, objects[j].graphicsPipeline);
            VkBuffer vertexBuffers[] = { textVertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(commandBuffers[i], textIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            
            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, objects[j].pipelineLayout, 0, 1, &(objects[j].descriptorSets[i]), 0, NULL);
            vkCmdDrawIndexed(commandBuffers[i], textIndexCount, 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            printf("failed to record command buffer\n");
            exit(-1);
        }
    }
}

void createSyncObjects() {
    imageAvailableSemaphores = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    renderFinishedSemaphores = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    inFlightFences = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));
    imagesInFlight = malloc(swapchainImageCount * sizeof(VkFence));
    if (imageAvailableSemaphores == NULL || renderFinishedSemaphores == NULL || inFlightFences == NULL || imagesInFlight == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }

    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        imagesInFlight[i] = VK_NULL_HANDLE;
    }

    VkSemaphoreCreateInfo semaphoreInfo = (VkSemaphoreCreateInfo){ 0 };
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = (VkFenceCreateInfo){ 0 };
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphores[i]) != VK_SUCCESS || vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphores[i]) != VK_SUCCESS || vkCreateFence(device, &fenceInfo, NULL, &inFlightFences[i]) != VK_SUCCESS) {
            printf("failed to create semaphores\n");
            exit(-1);
        }
    }
}

void setupObjectShaderMap() {
    for (uint32_t i = 0; i < objectCount; i++) {
        int foundElem = ht_search(objectShaders, objects[i].shader);
        if (foundElem != -1) {
            DrawableObject temp = objects[foundElem];
            objects[foundElem] = objects[i];
            objects[i] = temp;
        }
        ht_insert(objectShaders, objects[i].shader, i);
    }
}

void setupObjects() {
    for (uint32_t i = 0; i < objectCount; i++) {
        int foundIndex = ht_search(objectShaders, objects[i].shader);
        if (foundIndex >= i) {
            if (i == 0) {
                createGraphicsPipeline(&objects[i].graphicsPipeline, &objects[i].pipelineLayout, objects[i].shader);
            }
            else if (strcmp(objects[i - 1].shader, objects[i].shader) != 0) {
                createGraphicsPipeline(&objects[i].graphicsPipeline, &objects[i].pipelineLayout, objects[i].shader);
            }
            else {
                objects[i].graphicsPipeline = objects[i - 1].graphicsPipeline;
                objects[i].pipelineLayout = objects[i - 1].pipelineLayout;
            }
        }
        else {
            createGraphicsPipeline(&objects[i].graphicsPipeline, &objects[i].pipelineLayout, objects[i].shader);
        }
        createDescriptorSets(&objects[i]);
    }
}

void writeBMCharsToBin(char* in_file, char* out_file, bmchar* out_arr, size_t out_arr_size) {
    FILE* fp = fopen(in_file, "r");
    if (fp == NULL) {
        printf("failed to open file\n");
        exit(-1);
    }

    char chunk[128];

    size_t len = sizeof(chunk);
    char* line = malloc(len);
    if (line == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }

    line[0] = '\0';

    bool foundChar = false;

    while (fgets(chunk, sizeof(chunk), fp) != NULL) {
        size_t len_used = strlen(line);
        size_t chunk_used = strlen(chunk);

        if (len - len_used < chunk_used) {
            len *= 2;
            if ((line = realloc(line, len)) == NULL) {
                printf("failed to reallocate memory\n");
                free(line);
                exit(1);
            }
        }

        strncpy(line + len_used, chunk, len - len_used);
        len_used += chunk_used;

        if (line[len_used - 1] == '\n') {
            char* token = strtok(line, " ");
            if (strcmp(token, "char") == 0) {
                if (!foundChar) foundChar = true;
                token = strtok(NULL, " id=");
                int charid = atoi(token);
                token = strtok(NULL, " x=");
                out_arr[charid].x = atoi(token);
                token = strtok(NULL, " y=");
                out_arr[charid].y = atoi(token);
                token = strtok(NULL, " width=");
                out_arr[charid].width = atoi(token);
                token = strtok(NULL, " height=");
                out_arr[charid].height = atoi(token);
                token = strtok(NULL, " xoffset=");
                out_arr[charid].xoffset = atoi(token);
                token = strtok(NULL, " yoffset=");
                out_arr[charid].yoffset = atoi(token);
                token = strtok(NULL, " xadvance=");
                out_arr[charid].xadvance = atoi(token);
                token = strtok(NULL, " page=");
                out_arr[charid].page = atoi(token);
            } else {
                if (foundChar) break;
            }
            line[0] = '\0';
        }
    }
    
    free(line);
    fclose(fp);
    
    char* buffer = malloc(out_arr_size);
    if (buffer == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }
    memcpy(buffer, out_arr, out_arr_size);
    
    fp = fopen(out_file, "wb");
    if (fp == NULL) {
        printf("failed to open file\n");
        exit(-1);
    }
    
    fwrite(buffer, sizeof(bmchar), out_arr_size / sizeof(bmchar), fp);
    
    free(buffer);
    fclose(fp);
}

void loadBMCharsFromBin(char* in_file, bmchar* out_arr, size_t out_arr_size) {
    FILE* fp = fopen(in_file, "rb");
    if (fp == NULL) {
        printf("failed to open file\n");
        exit(-1);
    }
    
    char* buffer = malloc(out_arr_size);
    if (buffer == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }
    fread(buffer, sizeof(bmchar), out_arr_size / sizeof(bmchar), fp);
    memcpy(out_arr, buffer, out_arr_size);
    
    free(buffer);
    fclose(fp);
}


void createObject(DrawableObject* object, vec2 pos, char* shader) {
    glm_mat4_identity(object->ubo.model);
    vec3 pos3 = (vec3){ pos[0], pos[1], 0.0f };
    glm_translate(object->ubo.model, pos3);
    object->shader = shader;
}

void generateText(const char* text, const uint32_t text_length) {
    vertex* vertices = malloc(text_length * 4 * sizeof(vertex));
    uint32_t* textIndices = malloc(text_length * 6 * sizeof(uint32_t));
    if (vertices == NULL || textIndices == NULL) {
        printf("ran out of memory\n");
        exit(-1);
    }
    
    uint32_t indexOffset = 0;

    float posx = 0.0f;
    float posy = 0.0f;
    
    textIndexCount = 0;
    uint32_t textVertexCount = 0;
    
    for (uint32_t i = 0; i < text_length; i++) {
        if (text[i] == '\0') break;
        
        bmchar* charInfo = &fontChars[(int)text[i]];

        if (charInfo->width == 0) {
            charInfo->width = 36;
        }

        float charw = ((float)(charInfo->width) / 36.0f);
        float dimx = 1.0f * charw;
        float charh = ((float)(charInfo->height) / 36.0f);
        float dimy = 1.0f * charh;

        float xo = charInfo->xoffset / 36.0f;
        float yo = charInfo->yoffset / 36.0f;

        posy = yo;

        vertices[i * 4] = (vertex){ posx + dimx + xo, posy + dimy, 0.0f };
        vertices[(i * 4) + 1] = (vertex){ posx + xo, posy + dimy, 0.0f };
        vertices[(i * 4) + 2] = (vertex){ posx + xo, posy, 0.0f };
        vertices[(i * 4) + 3] = (vertex){ posx + dimx + xo, posy, 0.0f };
        
        textVertexCount += 4;

        uint32_t letterIndices[] = { 0, 1, 2, 2, 3, 0 };
        for (uint32_t j = 0; j < 6; j++) {
            textIndices[(i * 6) + j] = (indexOffset + letterIndices[j]);
        }
        indexOffset += 4;
        
        textIndexCount += 6;

        float advance = ((float)(charInfo->xadvance) / 36.0f);
        posx += advance;
    }

    // Center
    for (uint32_t i = 0; i < textVertexCount; i++) {
        vertices[i].pos[0] -= posx / 2.0f;
        vertices[i].pos[1] -= 0.5f;
    }
    
    void* data;
    
    createBuffer(textVertexCount * sizeof(vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &textVertexBuffer, &textVertexBufferMemory);
    
    data = NULL;
    
    vkMapMemory(device, textVertexBufferMemory, 0, textVertexCount * sizeof(vertex), 0, &data);
    memcpy(data, vertices, textVertexCount * sizeof(vertex));
    vkUnmapMemory(device, textVertexBufferMemory);
    
    free(vertices);
    
    createBuffer(textIndexCount * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &textIndexBuffer, &textIndexBufferMemory);
    
    data = NULL;
    
    vkMapMemory(device, textIndexBufferMemory, 0, textIndexCount * sizeof(uint32_t), 0, &data);
    memcpy(data, textIndices, textIndexCount * sizeof(uint32_t));
    vkUnmapMemory(device, textIndexBufferMemory);
    
    free(textIndices);
}

void cleanupSwapchain() {
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        vkDestroyFramebuffer(device, swapchainFramebuffers[i], NULL);
    }

    free(swapchainFramebuffers);

    vkFreeCommandBuffers(device, commandPool, swapchainImageCount, commandBuffers);

    free(commandBuffers);
    
    for (uint32_t i = 0; i < objectCount; i++) {
        if (i == 0 || strcmp(objects[i - 1].shader, objects[i].shader) != 0) {
            vkDestroyPipeline(device, objects[i].graphicsPipeline, NULL);
            vkDestroyPipelineLayout(device, objects[i].pipelineLayout, NULL);
        }
    }
    
    vkDestroyRenderPass(device, renderPass, NULL);

    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        vkDestroyImageView(device, swapchainImageViews[i], NULL);
    }

    free(swapchainImageViews);
    free(swapchainImages);

    vkDestroySwapchainKHR(device, oldSwapchain, NULL);
    
    for (uint32_t i = 0; i < objectCount; i++) {
        for (uint32_t j = 0; j < swapchainImageCount; j++) {
            vkDestroyBuffer(device, objects[i].uniformBuffers[j], NULL);
            vkFreeMemory(device, objects[i].uniformBuffersMemory[j], NULL);
        }
        
        free(objects[i].uniformBuffers);
        free(objects[i].uniformBuffersMemory);
        free(objects[i].descriptorSets);
    }
    
    vkDestroyDescriptorPool(device, descriptorPool, NULL);
}

void recreateSwapchain() {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapchain();

    createSwapchain();
    createImageViews();
    createRenderPass();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    setupObjects();
    createCommandBuffers();
}

void updateUniformBuffer(uint32_t currentImage) {
    for (uint32_t i = 0; i < objectCount; i++) {
        glm_mat4_identity(objects[i].ubo.view);
        
        glm_ortho_default((float)swapchainExtent.width / (float)swapchainExtent.height, objects[i].ubo.projection);
        objects[i].ubo.projection[1][1] *= -1;
        
        void* data;
        vkMapMemory(device, objects[i].uniformBuffersMemory[currentImage], 0, sizeof(objects[i].ubo), 0, &data);
        memcpy(data, &objects[i].ubo, sizeof(objects[i].ubo));
        vkUnmapMemory(device, objects[i].uniformBuffersMemory[currentImage]);
    }
}

void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        printf("failed to acquire swapchain image\n");
        exit(-1);
    }
    
    updateUniformBuffer(imageIndex);

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    VkSubmitInfo submitInfo = (VkSubmitInfo){ 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        printf("failed to submit draw command buffer\n");
        exit(-1);
    }

    VkPresentInfoKHR presentInfo = (VkPresentInfoKHR){ 0 };
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;

    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = NULL;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        printf("failed to present swap chain image\n");
        exit(-1);
    }

    currentFrame = (currentFrame + 1) & MAX_FRAMES_IN_FLIGHT;
}

void drawLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}

void cleanup() {
    cleanupSwapchain();
    
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);

    vkDestroyBuffer(device, indexBuffer, NULL);
    vkFreeMemory(device, indexBufferMemory, NULL);

    vkDestroyBuffer(device, vertexBuffer, NULL);
    vkFreeMemory(device, vertexBufferMemory, NULL);
    
    vkDestroyBuffer(device, textVertexBuffer, NULL);
    vkFreeMemory(device, textVertexBufferMemory, NULL);
    
    vkDestroyBuffer(device, textIndexBuffer, NULL);
    vkFreeMemory(device, textIndexBufferMemory, NULL);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], NULL);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], NULL);
        vkDestroyFence(device, inFlightFences[i], NULL);
    }

    free(imageAvailableSemaphores);
    free(renderFinishedSemaphores);
    free(inFlightFences);
    free(imagesInFlight);

    vkDestroyCommandPool(device, commandPool, NULL);

    vkDestroySwapchainKHR(device, swapchain, NULL);
    
    vkDestroyDevice(device, NULL);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
    }

    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    free_hashtable(objectShaders);

    glfwDestroyWindow(window);

    glfwTerminate();
}

int main() {
    objectShaders = create_table(objectCount);
    initWindow();
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    //writeBMCharsToBin("fonts/Bitter.fnt", "fonts/Bitter.fnt.bin", fontChars, sizeof(fontChars));
    loadBMCharsFromBin("fonts/Bitter.fnt.bin", fontChars, sizeof(fontChars));
    generateText("hello", sizeof("hello"));
    createObject(&objects[0], (vec2){ 0.0f, 0.0f }, "shader");
    //createObject(&objects[1], (vec2){ 0.5f, 0.0f }, "shader");
    setupObjectShaderMap();
    setupObjects();
    createCommandBuffers();
    createSyncObjects();
    drawLoop();
    cleanup();
}
