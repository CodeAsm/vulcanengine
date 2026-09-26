#pragma once

#include "VulkanBase.h"
#include "Camera.h"
#include "Pipeline.h"
#include "UI.h"
#include "Mesh.h"
#include "Texture.h"
#include "ModelLoader.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <memory>

class Application {
public:
    void run();

private:
    void initWindow();
    void initVulkan();
    void initTestCube();
    void mainLoop();
    void cleanup();

    void processInput(float deltaTime);
    void drawFrame();

    // Loads a model from disk (OBJ always, FBX if built with Assimp),
    // uploads its meshes and textures to the GPU, and swaps it in as the
    // thing being rendered. Any previously loaded model is torn down first.
    void loadModel(const std::string& path);
    void unloadCurrentModel();

    // Combines the user-controlled transform (keyboard/mouse/UI) with the
    // model's auto-fit base transform computed at load time.
    glm::mat4 buildObjectModelMatrix() const;

    static void framebufferResizeCallback(
        GLFWwindow* window,
        int width,
        int height
    );

    static void mouseButtonCallback(
        GLFWwindow* window,
        int button,
        int action,
        int mods
    );

    static void cursorPositionCallback(
        GLFWwindow* window,
        double xpos,
        double ypos
    );

    static void scrollCallback(
        GLFWwindow* window,
        double xoffset,
        double yoffset
    );

    GLFWwindow* window = nullptr;

    VulkanBase vulkan;
    Camera camera;
    Pipeline pipeline;
    UI ui;

    // Default placeholder shown until the user loads a real model.
    GPUMesh cubeMesh;
    Texture cubeTexture;
    VkDescriptorSet cubeTextureDescriptorSet = VK_NULL_HANDLE;

    // The currently loaded model (from ModelLoader + uploadMesh), one GPU
    // mesh per material group, one texture/descriptor set per material.
    std::vector<GPUMesh> modelMeshes;
    std::vector<std::unique_ptr<Texture>> modelTextures;
    std::vector<VkDescriptorSet> modelTextureDescriptorSets;
    bool modelLoaded = false;

    // Auto-fit transform computed when a model is loaded: centers it on the
    // origin and scales it so its largest dimension is ~2 units, since
    // downloaded models can be in wildly different units/scales.
    glm::mat4 modelBaseTransform{1.0f};

    // User-controlled transform, adjustable via keyboard, mouse, and the UI.
    ObjectTransform objectTransform;

    std::string modelLabel = "None (showing default test cube)";
    std::string statusMessage;
    size_t modelMeshCount = 0;
    size_t modelVertexCount = 0;

    bool rightMouseDown = false; // true while the LEFT mouse button (camera look) is held
    bool objectDragDown = false; // true while the RIGHT mouse button (object manipulation) is held
    bool firstMouse = true;
    bool firstObjectDrag = true;

    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    double lastObjectMouseX = 0.0;
    double lastObjectMouseY = 0.0;

    size_t currentFrame = 0;
};