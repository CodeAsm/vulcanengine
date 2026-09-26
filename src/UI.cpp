#include "UI.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <array>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <system_error>

namespace fs = std::filesystem;

namespace {

// Extensions the file browser will offer to load. FBX only actually works
// if ModelLoader was built with Assimp (HAVE_ASSIMP) -- we still list it so
// the user isn't confused about where their file went, but ModelLoader::load
// will throw a clear error if it's picked without Assimp support.
bool isModelFile(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                    [](unsigned char c) { return (char)std::tolower(c); });
    return ext == ".obj" || ext == ".fbx";
}

} // namespace

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

    std::error_code ec;
    currentDirectory = fs::current_path(ec);
    if (ec) {
        currentDirectory = fs::path(".");
    }
    refreshDirectoryListing();
}

void UI::setModelSelectedCallback(std::function<void(const std::string&)> callback)
{
    onModelSelected = std::move(callback);
}

void UI::refreshDirectoryListing()
{
    directoryEntries.clear();
    fileEntries.clear();
    fileBrowserError.clear();

    std::error_code ec;
    fs::directory_iterator it(currentDirectory, ec);
    if (ec) {
        fileBrowserError = "Cannot open directory: " + currentDirectory.string();
        return;
    }

    for (const auto& entry : it) {
        std::error_code entryEc;
        if (entry.is_directory(entryEc)) {
            directoryEntries.push_back(entry.path());
        } else if (isModelFile(entry.path())) {
            fileEntries.push_back(entry.path());
        }
    }

    auto byFilename = [](const fs::path& a, const fs::path& b) {
        return a.filename().string() < b.filename().string();
    };
    std::sort(directoryEntries.begin(), directoryEntries.end(), byFilename);
    std::sort(fileEntries.begin(), fileEntries.end(), byFilename);
}

void UI::draw(UIState& state)
{
    if (!initialized) {
        return;
    }

    drawMainMenuBar();

    if (showFileBrowser) {
        drawFileBrowser();
    }

    drawObjectPanel(state);
}

void UI::drawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Model...")) {
                showFileBrowser = true;
                selectedFile.clear();
                refreshDirectoryListing();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UI::drawFileBrowser()
{
    ImGui::SetNextWindowSize(ImVec2(520, 420), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Open Model", &showFileBrowser)) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Folder:");
    ImGui::SameLine();
    ImGui::TextDisabled("%s", currentDirectory.string().c_str());

    if (ImGui::Button("Up one level")) {
        fs::path parent = currentDirectory.parent_path();
        if (!parent.empty()) {
            currentDirectory = parent;
            selectedFile.clear();
            refreshDirectoryListing();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        refreshDirectoryListing();
    }

    ImGui::Separator();

    // Manual path entry, for typing/pasting a folder or file path directly.
    static char pathBuf[1024];
    std::snprintf(pathBuf, sizeof(pathBuf), "%s", manualPathBuffer.c_str());
    ImGui::SetNextItemWidth(-90.0f);
    if (ImGui::InputTextWithHint("##manualpath", "Paste a folder or .obj/.fbx path and press Go",
                                  pathBuf, sizeof(pathBuf))) {
        manualPathBuffer = pathBuf;
    }
    ImGui::SameLine();
    if (ImGui::Button("Go")) {
        fs::path typed(manualPathBuffer);
        std::error_code ec;
        if (fs::is_directory(typed, ec)) {
            currentDirectory = typed;
            selectedFile.clear();
            refreshDirectoryListing();
        } else if (fs::is_regular_file(typed, ec) && isModelFile(typed)) {
            selectedFile = typed;
            currentDirectory = typed.parent_path();
            refreshDirectoryListing();
        } else {
            fileBrowserError = "Not a folder or a .obj/.fbx file: " + manualPathBuffer;
        }
    }

    if (!fileBrowserError.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", fileBrowserError.c_str());
    }

    ImGui::Separator();

    ImGui::BeginChild("##filelist", ImVec2(0, -32), true);

    for (const auto& dir : directoryEntries) {
        std::string label = "[dir]  " + dir.filename().string();
        if (ImGui::Selectable(label.c_str())) {
            currentDirectory = dir;
            selectedFile.clear();
            refreshDirectoryListing();
        }
    }

    for (const auto& file : fileEntries) {
        bool isSelected = (file == selectedFile);
        std::string label = file.filename().string();
        if (ImGui::Selectable(label.c_str(), isSelected)) {
            selectedFile = file;
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            selectedFile = file;
            if (onModelSelected) onModelSelected(selectedFile.string());
            showFileBrowser = false;
        }
    }

    if (directoryEntries.empty() && fileEntries.empty()) {
        ImGui::TextDisabled("(empty, or no .obj/.fbx files here)");
    }

    ImGui::EndChild();

    ImGui::TextDisabled("Tip: textures are found automatically next to the model, or in a");
    ImGui::TextDisabled("'textures' subfolder -- that's usually how Sketchfab exports look.");

    bool canLoad = !selectedFile.empty();
    if (!canLoad) ImGui::BeginDisabled();
    if (ImGui::Button("Load", ImVec2(120, 0))) {
        if (onModelSelected) onModelSelected(selectedFile.string());
        showFileBrowser = false;
    }
    if (!canLoad) ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        showFileBrowser = false;
    }

    ImGui::End();
}

void UI::drawObjectPanel(UIState& state)
{
    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Model Viewer")) {
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("Model: %s", state.modelLabel.c_str());
    ImGui::Text("Meshes: %zu   Vertices: %zu", state.meshCount, state.vertexCount);

    if (!state.statusMessage.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "%s", state.statusMessage.c_str());
    }

    ImGui::Separator();

    if (ImGui::Button("Open Model...", ImVec2(-1, 0))) {
        showFileBrowser = true;
        selectedFile.clear();
        refreshDirectoryListing();
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Object transform");
    ImGui::TextDisabled("(also controlled by keyboard/mouse, see README)");

    if (state.transform) {
        ImGui::DragFloat3("Position", &state.transform->position.x, 0.02f);
        ImGui::DragFloat3("Rotation (deg)", &state.transform->rotationDegrees.x, 0.5f);
        ImGui::DragFloat("Scale", &state.transform->scale, 0.01f, 0.001f, 1000.0f);

        if (ImGui::Button("Reset transform")) {
            state.transform->position = glm::vec3(0.0f);
            state.transform->rotationDegrees = glm::vec3(0.0f);
            state.transform->scale = 1.0f;
        }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Camera");
    ImGui::Text("Pos: %.2f, %.2f, %.2f", state.cameraPosition.x, state.cameraPosition.y, state.cameraPosition.z);
    ImGui::Text("Yaw: %.1f  Pitch: %.1f", state.cameraYaw, state.cameraPitch);
    if (state.cameraMoveSpeed) {
        ImGui::DragFloat("Fly speed", state.cameraMoveSpeed, 0.05f, 0.1f, 50.0f);
    }
    ImGui::TextDisabled("Hold left mouse button + move to look around, WASD to fly.");

    ImGui::End();
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
