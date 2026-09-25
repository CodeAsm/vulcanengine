#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdexcept>
#include <cstring>
 
void Texture::uploadPixels(const VulkanBase& base, unsigned char* pixels, int width, int height) {
    VkDeviceSize imageSize = (VkDeviceSize)width * height * 4;
 
    VkBuffer staging;
    VkDeviceMemory stagingMemory;
    base.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       staging, stagingMemory);
 
    void* mapped;
    vkMapMemory(base.device, stagingMemory, 0, imageSize, 0, &mapped);
    memcpy(mapped, pixels, (size_t)imageSize);
    vkUnmapMemory(base.device, stagingMemory);
 
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    base.createImage((uint32_t)width, (uint32_t)height, format, VK_IMAGE_TILING_OPTIMAL,
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);
 
    base.transitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    base.copyBufferToImage(staging, image, (uint32_t)width, (uint32_t)height);
    base.transitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
 
    vkDestroyBuffer(base.device, staging, nullptr);
    vkFreeMemory(base.device, stagingMemory, nullptr);
 
    view = base.createImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
    createSampler(base);
}
 
void Texture::createSampler(const VulkanBase& base) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(base.physicalDevice, &props);
 
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = props.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
 
    if (vkCreateSampler(base.device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler");
    }
}
 
void Texture::loadFromFile(const VulkanBase& base, const std::string& path) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        // Fall back to a magenta "missing texture" swatch instead of crashing the whole viewer.
        createSolidColor(base, 230, 0, 230);
        return;
    }
    uploadPixels(base, pixels, width, height);
    stbi_image_free(pixels);
}
 
void Texture::createSolidColor(const VulkanBase& base, unsigned char r, unsigned char g, unsigned char b) {
    unsigned char pixel[4] = {r, g, b, 255};
    uploadPixels(base, pixel, 1, 1);
}
 
void Texture::destroy(VkDevice device) {
    if (sampler) vkDestroySampler(device, sampler, nullptr);
    if (view) vkDestroyImageView(device, view, nullptr);
    if (image) vkDestroyImage(device, image, nullptr);
    if (memory) vkFreeMemory(device, memory, nullptr);
    sampler = VK_NULL_HANDLE; view = VK_NULL_HANDLE; image = VK_NULL_HANDLE; memory = VK_NULL_HANDLE;
}
