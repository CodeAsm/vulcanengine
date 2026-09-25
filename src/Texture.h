#pragma once
#include "VulkanBase.h"
#include <string>
 
class Texture {
public:
    void loadFromFile(const VulkanBase& base, const std::string& path);
    void createSolidColor(const VulkanBase& base, unsigned char r, unsigned char g, unsigned char b);
    void destroy(VkDevice device);
 
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
 
private:
    void uploadPixels(const VulkanBase& base, unsigned char* pixels, int width, int height);
    void createSampler(const VulkanBase& base);
};
 
