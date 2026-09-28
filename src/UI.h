#pragma once

#include "VulkanBase.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <functional>
#include <filesystem>

// User-adjustable transform applied on top of the model's auto-fit
// (centered + normalized) base transform. Shared between keyboard/mouse
// input in Application and the sliders drawn by UI::draw().
struct ObjectTransform {
    glm::vec3 position{0.0f};
    glm::vec3 rotationDegrees{0.0f};
    float scale = 1.0f;
};

// Everything the UI needs to read or edit for one frame. Pointers are
// non-owning and only need to stay valid for the duration of UI::draw().
struct UIState {
    ObjectTransform* transform = nullptr;
    float* cameraMoveSpeed = nullptr;

    std::string modelLabel = "None (showing default test cube)";
    std::string statusMessage;
    size_t meshCount = 0;
    size_t vertexCount = 0;

    glm::vec3 cameraPosition{0.0f};
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
};

class UI {
public:
    void init(
        GLFWwindow* window,
        const VulkanBase& base
    );

    void newFrame();

    // Builds every ImGui window for this frame (menu bar, file browser,
    // object/camera panel). Call after newFrame() and before render().
    void draw(UIState& state);

    void render(
        VkCommandBuffer commandBuffer
    );

    void cleanup();

    bool wantsMouse() const;
    bool wantsKeyboard() const;

    // Invoked with the full path of the file the user picked in the
    // browser once they press "Load". Set this once during setup.
    void setModelSelectedCallback(std::function<void(const std::string&)> callback);
	std::function<void()> onExit;

private:
    void drawMainMenuBar();
    void drawFileBrowser();
    void drawObjectPanel(UIState& state);
    void refreshDirectoryListing();

    bool initialized = false;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    std::function<void(const std::string&)> onModelSelected;

    bool showFileBrowser = false;
    std::filesystem::path currentDirectory;
    std::vector<std::filesystem::path> directoryEntries; // sub-folders, sorted
    std::vector<std::filesystem::path> fileEntries;       // matching model files, sorted
    std::filesystem::path selectedFile;
    std::string manualPathBuffer;
    std::string fileBrowserError;
};
