#pragma once

#include "VulkanBase.h"
#include "Mesh.h"
#include "Texture.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <vector>
#include <string>

class Pipeline {
public:
    void init(const VulkanBase& base);
    void cleanup(const VulkanBase& base);

    void createTextureDescriptor(
        const VulkanBase& base,
        Texture& texture,
        VkDescriptorSet& descriptorSet
    );

    void updateScene(
        const VulkanBase& base,
        uint32_t frameIndex,
        const Camera& camera
    );

    void draw(
        const VulkanBase& base,
        VkCommandBuffer commandBuffer,
        uint32_t frameIndex,
        const std::vector<GPUMesh>& meshes,
        const std::vector<VkDescriptorSet>& textureDescriptorSets,
        const glm::mat4& modelMatrix
    );

    VkDescriptorSetLayout sceneDescriptorSetLayout =
        VK_NULL_HANDLE;

    VkDescriptorSetLayout textureDescriptorSetLayout =
        VK_NULL_HANDLE;

    VkDescriptorPool descriptorPool =
        VK_NULL_HANDLE;

    VkPipelineLayout pipelineLayout =
        VK_NULL_HANDLE;

    VkPipeline graphicsPipeline =
        VK_NULL_HANDLE;

private:
    struct SceneUBO {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec4 lightDir;
        glm::vec4 viewPos;
    };

    struct UniformBuffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mapped = nullptr;
    };

    std::vector<UniformBuffer> sceneUniformBuffers;
    std::vector<VkDescriptorSet> sceneDescriptorSets;

    VkShaderModule createShaderModule(
        const VulkanBase& base,
        const std::string& filename
    );

    void createDescriptorSetLayouts(
        const VulkanBase& base
    );

    void createDescriptorPool(
        const VulkanBase& base
    );

    void createUniformBuffers(
        const VulkanBase& base
    );

    void createDescriptorSets(
        const VulkanBase& base
    );

    void createGraphicsPipeline(
        const VulkanBase& base
    );

    static std::vector<char> readFile(
        const std::string& filename
    );
};
