#pragma once

#include "VulkanBase.h"
#include "Camera.h"
#include "Pipeline.h"
#include "UI.h"
#include "Mesh.h"
#include "Texture.h"

#include <GLFW/glfw3.h>

#include <vector>

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

    GLFWwindow* window = nullptr;

    VulkanBase vulkan;
    Camera camera;
    Pipeline pipeline;
    UI ui;

    GPUMesh cubeMesh;
    Texture cubeTexture;
    VkDescriptorSet cubeTextureDescriptorSet = VK_NULL_HANDLE;

    bool rightMouseDown = false;
    bool firstMouse = true;

    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    size_t currentFrame = 0;
};