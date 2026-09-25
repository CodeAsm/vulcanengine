#pragma once

#include "VulkanBase.h"
#include "Camera.h"

#include <GLFW/glfw3.h>

class Application {
public:
    void run();

private:
    void initWindow();
    void initVulkan();
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

    bool rightMouseDown = false;
    bool firstMouse = true;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    float lastFrameTime = 0.0f;
};
