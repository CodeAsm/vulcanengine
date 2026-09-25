#pragma once

#include "VulkanBase.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

class UI {
public:
    void init(
        GLFWwindow* window,
        const VulkanBase& base
    );

    void newFrame();

    void render(
        VkCommandBuffer commandBuffer
    );

    void cleanup();

    bool wantsMouse() const;
    bool wantsKeyboard() const;

private:
    bool initialized = false;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
};
