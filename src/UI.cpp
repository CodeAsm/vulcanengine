#include "UI.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <array>
#include <stdexcept>

void UI::init(
    GLFWwindow* window,
    const VulkanBase& base)
{
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForVulkan(window, true)) {
        throw std::runtime_error(
            "failed to initialize ImGui GLFW backend"
        );
    }

    // ImGui gets its own descriptor pool.
    std::array<VkDescriptorPoolSize, 11> poolSizes = {{
        { VK_DESCRIPTOR_TYPE_SAMPLER,                1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,  1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,  1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000 }
    }};

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

    poolInfo.flags =
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    poolInfo.maxSets =
        1000 * static_cast<uint32_t>(poolSizes.size());

    poolInfo.poolSizeCount =
        static_cast<uint32_t>(poolSizes.size());

    poolInfo.pPoolSizes =
        poolSizes.data();

    if (vkCreateDescriptorPool(
            base.device,
            &poolInfo,
            nullptr,
            &descriptorPool) != VK_SUCCESS) {

        throw std::runtime_error(
            "failed to create ImGui descriptor pool"
        );
    }

    ImGui_ImplVulkan_InitInfo initInfo{};

    initInfo.Instance = base.instance;
    initInfo.PhysicalDevice = base.physicalDevice;
    initInfo.Device = base.device;

    initInfo.QueueFamily =
        base.queueFamilies.graphicsFamily.value();

    initInfo.Queue = base.graphicsQueue;

    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = descriptorPool;

    initInfo.MinImageCount =
        static_cast<uint32_t>(base.swapchainImages.size());

    initInfo.ImageCount =
        static_cast<uint32_t>(base.swapchainImages.size());

    // ImGui 1.91.1 uses these directly.
    initInfo.RenderPass = base.renderPass;
    initInfo.Subpass = 0;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        vkDestroyDescriptorPool(
            base.device,
            descriptorPool,
            nullptr
        );

        descriptorPool = VK_NULL_HANDLE;

        throw std::runtime_error(
            "failed to initialize ImGui Vulkan backend"
        );
    }

    // Upload the font atlas.
    //
    // In this ImGui version CreateFontsTexture() submits its own
    // upload work through the backend.
    if (!ImGui_ImplVulkan_CreateFontsTexture()) {
        throw std::runtime_error(
            "failed to create ImGui font texture"
        );
    }

    initialized = true;
}

void UI::newFrame()
{
    if (!initialized) {
        return;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();
}

void UI::render(
    VkCommandBuffer commandBuffer)
{
    if (!initialized) {
        return;
    }

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(
        ImGui::GetDrawData(),
        commandBuffer
    );
}

bool UI::wantsMouse() const
{
    if (!initialized) {
        return false;
    }

    return ImGui::GetIO().WantCaptureMouse;
}

bool UI::wantsKeyboard() const
{
    if (!initialized) {
        return false;
    }

    return ImGui::GetIO().WantCaptureKeyboard;
}

void UI::cleanup()
{
    if (!initialized) {
        return;
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();

    initialized = false;
}
