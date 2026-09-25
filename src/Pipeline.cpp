#include "Pipeline.h"

#include <fstream>
#include <stdexcept>
#include <cstring>
#include <array>

namespace {

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = VulkanBase::MAX_FRAMES_IN_FLIGHT;

VkDescriptorSet allocateDescriptorSet(
    VkDevice device,
    VkDescriptorPool pool,
    VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet set = VK_NULL_HANDLE;

    if (vkAllocateDescriptorSets(
            device,
            &allocInfo,
            &set) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to allocate descriptor set"
        );
    }

    return set;
}

} // namespace

std::vector<char> Pipeline::readFile(const std::string& filename)
{
    std::ifstream file(
        filename,
        std::ios::ate | std::ios::binary
    );

    if (!file.is_open()) {
        throw std::runtime_error(
            "failed to open shader file: " + filename
        );
    }

    const size_t fileSize =
        static_cast<size_t>(file.tellg());

    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    file.close();

    return buffer;
}

VkShaderModule Pipeline::createShaderModule(
    const VulkanBase& base,
    const std::string& filename)
{
    const std::vector<char> code = readFile(filename);

    if (code.empty() ||
        code.size() % sizeof(uint32_t) != 0) {
        throw std::runtime_error(
            "invalid SPIR-V shader: " + filename
        );
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType =
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode =
        reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;

    if (vkCreateShaderModule(
            base.device,
            &createInfo,
            nullptr,
            &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create shader module: " + filename
        );
    }

    return shaderModule;
}

void Pipeline::createDescriptorSetLayouts(
    const VulkanBase& base)
{
    // ---------------------------------------------------------------------
    // Set 0:
    //
    // layout(set = 0, binding = 0) uniform SceneUBO
    // ---------------------------------------------------------------------

    VkDescriptorSetLayoutBinding sceneBinding{};
    sceneBinding.binding = 0;
    sceneBinding.descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sceneBinding.descriptorCount = 1;
    sceneBinding.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT |
        VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo sceneLayoutInfo{};
    sceneLayoutInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sceneLayoutInfo.bindingCount = 1;
    sceneLayoutInfo.pBindings = &sceneBinding;

    if (vkCreateDescriptorSetLayout(
            base.device,
            &sceneLayoutInfo,
            nullptr,
            &sceneDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create scene descriptor set layout"
        );
    }

    // ---------------------------------------------------------------------
    // Set 1:
    //
    // layout(set = 1, binding = 0) uniform sampler2D
    // ---------------------------------------------------------------------

    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 0;
    textureBinding.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = 1;
    textureBinding.stageFlags =
        VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
    textureLayoutInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutInfo.bindingCount = 1;
    textureLayoutInfo.pBindings = &textureBinding;

    if (vkCreateDescriptorSetLayout(
            base.device,
            &textureLayoutInfo,
            nullptr,
            &textureDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create texture descriptor set layout"
        );
    }
}

void Pipeline::createDescriptorPool(
    const VulkanBase& base)
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};

    poolSizes[0].type =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount =
        MAX_FRAMES_IN_FLIGHT;

    poolSizes[1].type =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    // Models can have many materials. Reserve a reasonable number.
    poolSizes[1].descriptorCount = 1024;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

    poolInfo.poolSizeCount =
        static_cast<uint32_t>(poolSizes.size());

    poolInfo.pPoolSizes = poolSizes.data();

    poolInfo.maxSets =
        MAX_FRAMES_IN_FLIGHT + 1024;

    if (vkCreateDescriptorPool(
            base.device,
            &poolInfo,
            nullptr,
            &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create descriptor pool"
        );
    }
}

void Pipeline::createUniformBuffers(
    const VulkanBase& base)
{
    sceneUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (auto& ubo : sceneUniformBuffers) {
        base.createBuffer(
            sizeof(SceneUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            ubo.buffer,
            ubo.memory
        );

        if (vkMapMemory(
                base.device,
                ubo.memory,
                0,
                sizeof(SceneUBO),
                0,
                &ubo.mapped) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to map scene uniform buffer"
            );
        }
    }
}

void Pipeline::createDescriptorSets(
    const VulkanBase& base)
{
    sceneDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0;
         i < MAX_FRAMES_IN_FLIGHT;
         ++i) {

        sceneDescriptorSets[i] =
            allocateDescriptorSet(
                base.device,
                descriptorPool,
                sceneDescriptorSetLayout
            );

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer =
            sceneUniformBuffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(SceneUBO);

        VkWriteDescriptorSet write{};
        write.sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet =
            sceneDescriptorSets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType =
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(
            base.device,
            1,
            &write,
            0,
            nullptr
        );
    }
}

void Pipeline::createTextureDescriptor(
    const VulkanBase& base,
    Texture& texture,
    VkDescriptorSet& descriptorSet)
{
    descriptorSet = allocateDescriptorSet(
        base.device,
        descriptorPool,
        textureDescriptorSetLayout
    );

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout =
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture.view;
    imageInfo.sampler = texture.sampler;

    VkWriteDescriptorSet write{};
    write.sType =
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(
        base.device,
        1,
        &write,
        0,
        nullptr
    );
}

void Pipeline::createGraphicsPipeline(
    const VulkanBase& base)
{
    const std::string shaderDir = SHADER_DIR;

    const VkShaderModule vertShader =
        createShaderModule(
            base,
            shaderDir + "shader.vert.spv"
        );

    const VkShaderModule fragShader =
        createShaderModule(
            base,
            shaderDir + "shader.frag.spv"
        );

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage =
        VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertShader;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage =
        VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragShader;
    fragStage.pName = "main";

    const std::array<VkPipelineShaderStageCreateInfo, 2>
        shaderStages = {
            vertStage,
            fragStage
        };

    // ---------------------------------------------------------------------
    // Vertex input
    // ---------------------------------------------------------------------

    const auto bindingDescription =
        Vertex::getBindingDescription();

    const auto attributeDescriptions =
        Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions =
        &bindingDescription;

    vertexInput.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(
            attributeDescriptions.size()
        );

    vertexInput.pVertexAttributeDescriptions =
        attributeDescriptions.data();

    // ---------------------------------------------------------------------
    // Input assembly
    // ---------------------------------------------------------------------

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology =
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // ---------------------------------------------------------------------
    // Viewport / scissor
    // ---------------------------------------------------------------------

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // ---------------------------------------------------------------------
    // Rasterizer
    // ---------------------------------------------------------------------

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // ---------------------------------------------------------------------
    // Multisampling
    // ---------------------------------------------------------------------

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples =
        VK_SAMPLE_COUNT_1_BIT;

    // ---------------------------------------------------------------------
    // Depth
    // ---------------------------------------------------------------------

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp =
        VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // ---------------------------------------------------------------------
    // Color blending
    // ---------------------------------------------------------------------

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments =
        &colorBlendAttachment;

    // ---------------------------------------------------------------------
    // Dynamic viewport/scissor
    // ---------------------------------------------------------------------

    const std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates =
        dynamicStates.data();

    // ---------------------------------------------------------------------
    // Push constants
    //
    // shader:
    // layout(push_constant) uniform PushConsts {
    //     mat4 model;
    // } pc;
    // ---------------------------------------------------------------------

    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(glm::mat4);

    // ---------------------------------------------------------------------
    // Pipeline layout
    // ---------------------------------------------------------------------

    std::array<VkDescriptorSetLayout, 2> layouts = {
        sceneDescriptorSetLayout,
        textureDescriptorSetLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    pipelineLayoutInfo.setLayoutCount =
        static_cast<uint32_t>(layouts.size());

    pipelineLayoutInfo.pSetLayouts =
        layouts.data();

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges =
        &pushConstant;

    if (vkCreatePipelineLayout(
            base.device,
            &pipelineLayoutInfo,
            nullptr,
            &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create pipeline layout"
        );
    }

    // ---------------------------------------------------------------------
    // Graphics pipeline
    // ---------------------------------------------------------------------

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType =
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    pipelineInfo.stageCount =
        static_cast<uint32_t>(shaderStages.size());

    pipelineInfo.pStages =
        shaderStages.data();

    pipelineInfo.pVertexInputState =
        &vertexInput;

    pipelineInfo.pInputAssemblyState =
        &inputAssembly;

    pipelineInfo.pViewportState =
        &viewportState;

    pipelineInfo.pRasterizationState =
        &rasterizer;

    pipelineInfo.pMultisampleState =
        &multisampling;

    pipelineInfo.pDepthStencilState =
        &depthStencil;

    pipelineInfo.pColorBlendState =
        &colorBlending;

    pipelineInfo.pDynamicState =
        &dynamicState;

    pipelineInfo.layout =
        pipelineLayout;

    pipelineInfo.renderPass =
        base.renderPass;

    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(
            base.device,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create graphics pipeline"
        );
    }

    vkDestroyShaderModule(
        base.device,
        fragShader,
        nullptr
    );

    vkDestroyShaderModule(
        base.device,
        vertShader,
        nullptr
    );
}

void Pipeline::init(const VulkanBase& base)
{
    createDescriptorSetLayouts(base);
    createDescriptorPool(base);
    createUniformBuffers(base);
    createDescriptorSets(base);
    createGraphicsPipeline(base);
}

void Pipeline::updateScene(
    const VulkanBase& base,
    uint32_t frameIndex,
    const Camera& camera)
{
    SceneUBO ubo{};

    const float aspect =
        static_cast<float>(base.swapchainExtent.width) /
        static_cast<float>(base.swapchainExtent.height);

    ubo.view = camera.viewMatrix();

    ubo.proj = camera.projectionMatrix(aspect);

    // Vulkan uses a Y-down viewport convention in the
    // common GLM projection setup. GLM's Vulkan depth range
    // is handled by glm::perspective with GLM_FORCE_DEPTH_ZERO_TO_ONE
    // if enabled. Keep the projection correction here explicit.
    ubo.proj[1][1] *= -1.0f;

    ubo.lightDir = glm::vec4(
        -0.6f,
        -1.0f,
        -0.4f,
        0.0f
    );

    ubo.viewPos = glm::vec4(
        camera.position,
        1.0f
    );

    std::memcpy(
        sceneUniformBuffers[frameIndex].mapped,
        &ubo,
        sizeof(ubo)
    );
}

void Pipeline::draw(
    const VulkanBase& base,
    VkCommandBuffer commandBuffer,
    uint32_t frameIndex,
    const std::vector<GPUMesh>& meshes,
    const std::vector<VkDescriptorSet>& textureDescriptorSets,
    const glm::mat4& modelMatrix)
{
    vkCmdBindPipeline(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline
    );

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width =
        static_cast<float>(base.swapchainExtent.width);
    viewport.height =
        static_cast<float>(base.swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(
        commandBuffer,
        0,
        1,
        &viewport
    );

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = base.swapchainExtent;

    vkCmdSetScissor(
        commandBuffer,
        0,
        1,
        &scissor
    );

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        1,
        &sceneDescriptorSets[frameIndex],
        0,
        nullptr
    );

    vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(glm::mat4),
        &modelMatrix
    );

    for (size_t i = 0; i < meshes.size(); ++i) {
        const GPUMesh& mesh = meshes[i];

        if (mesh.indexCount == 0) {
            continue;
        }

        if (mesh.materialIndex < 0 ||
            static_cast<size_t>(mesh.materialIndex) >=
                textureDescriptorSets.size()) {
            continue;
        }

        VkDescriptorSet textureSet =
            textureDescriptorSets[
                static_cast<size_t>(mesh.materialIndex)
            ];

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            1,
            1,
            &textureSet,
            0,
            nullptr
        );

        VkBuffer vertexBuffers[] = {
            mesh.vertexBuffer
        };

        VkDeviceSize offsets[] = {
            0
        };

        vkCmdBindVertexBuffers(
            commandBuffer,
            0,
            1,
            vertexBuffers,
            offsets
        );

        vkCmdBindIndexBuffer(
            commandBuffer,
            mesh.indexBuffer,
            0,
            VK_INDEX_TYPE_UINT32
        );

        vkCmdDrawIndexed(
            commandBuffer,
            mesh.indexCount,
            1,
            0,
            0,
            0
        );
    }
}

void Pipeline::cleanup(const VulkanBase& base)
{
    for (auto& ubo : sceneUniformBuffers) {
        if (ubo.mapped) {
            vkUnmapMemory(
                base.device,
                ubo.memory
            );
            ubo.mapped = nullptr;
        }

        if (ubo.buffer) {
            vkDestroyBuffer(
                base.device,
                ubo.buffer,
                nullptr
            );
        }

        if (ubo.memory) {
            vkFreeMemory(
                base.device,
                ubo.memory,
                nullptr
            );
        }
    }

    sceneUniformBuffers.clear();
    sceneDescriptorSets.clear();

    if (graphicsPipeline) {
        vkDestroyPipeline(
            base.device,
            graphicsPipeline,
            nullptr
        );
        graphicsPipeline = VK_NULL_HANDLE;
    }

    if (pipelineLayout) {
        vkDestroyPipelineLayout(
            base.device,
            pipelineLayout,
            nullptr
        );
        pipelineLayout = VK_NULL_HANDLE;
    }

    if (descriptorPool) {
        vkDestroyDescriptorPool(
            base.device,
            descriptorPool,
            nullptr
        );
        descriptorPool = VK_NULL_HANDLE;
    }

    if (sceneDescriptorSetLayout) {
        vkDestroyDescriptorSetLayout(
            base.device,
            sceneDescriptorSetLayout,
            nullptr
        );
        sceneDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (textureDescriptorSetLayout) {
        vkDestroyDescriptorSetLayout(
            base.device,
            textureDescriptorSetLayout,
            nullptr
        );
        textureDescriptorSetLayout = VK_NULL_HANDLE;
    }
}
