#include "Application.h"

#include "Pipeline.h"
#include "UI.h"

#include <imgui.h>

#include <glm/glm.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>

namespace {

float currentTimeSeconds()
{
    using clock = std::chrono::steady_clock;

    static const auto start = clock::now();

    const auto now = clock::now();

    return std::chrono::duration<float>(
        now - start
    ).count();
}

} // namespace

void Application::initWindow()
{
    if (!glfwInit()) {
        throw std::runtime_error(
            "failed to initialize GLFW"
        );
    }

    glfwWindowHint(
        GLFW_CLIENT_API,
        GLFW_NO_API
    );

    glfwWindowHint(
        GLFW_RESIZABLE,
        GLFW_TRUE
    );

    window = glfwCreateWindow(
        1280,
        720,
        "Vulkan Model Viewer",
        nullptr,
        nullptr
    );

    if (!window) {
        glfwTerminate();

        throw std::runtime_error(
            "failed to create GLFW window"
        );
    }

    glfwSetWindowUserPointer(
        window,
        this
    );

    glfwSetFramebufferSizeCallback(
        window,
        framebufferResizeCallback
    );

    glfwSetMouseButtonCallback(
        window,
        mouseButtonCallback
    );

    glfwSetCursorPosCallback(
        window,
        cursorPositionCallback
    );
}

void Application::initVulkan()
{
    vulkan.init(window);
}

void Application::processInput(float deltaTime)
{
    if (!window) {
        return;
    }

    // Don't let camera controls interfere with ImGui text fields,
    // menus, etc.
    //
    // UI is queried here only when it exists later in the render loop.
    //
    // Keyboard movement:
    //   W / S = forward / backward
    //   A / D = left / right
    //   Space = up
    //   Ctrl  = down

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.processKeyboard(
            CameraMove::Forward,
            deltaTime
        );
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.processKeyboard(
            CameraMove::Backward,
            deltaTime
        );
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.processKeyboard(
            CameraMove::Left,
            deltaTime
        );
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.processKeyboard(
            CameraMove::Right,
            deltaTime
        );
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.processKeyboard(
            CameraMove::Up,
            deltaTime
        );
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        camera.processKeyboard(
            CameraMove::Down,
            deltaTime
        );
    }
}

void Application::mainLoop()
{
    float previousTime = currentTimeSeconds();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const float now = currentTimeSeconds();

        float deltaTime = now - previousTime;
        previousTime = now;

        // Avoid huge camera jumps after a breakpoint/window stall.
        if (deltaTime > 0.1f) {
            deltaTime = 0.1f;
        }

        processInput(deltaTime);

        drawFrame();
    }

    vkDeviceWaitIdle(vulkan.device);
}

void Application::drawFrame()
{
    static size_t currentFrame = 0;

    VkFence inFlightFence =
        vulkan.inFlightFences[currentFrame];

    if (vkWaitForFences(
            vulkan.device,
            1,
            &inFlightFence,
            VK_TRUE,
            UINT64_MAX) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed waiting for in-flight fence"
        );
    }

    uint32_t imageIndex = 0;

    VkResult result = vkAcquireNextImageKHR(
        vulkan.device,
        vulkan.swapchain,
        UINT64_MAX,
        vulkan.imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        vulkan.recreateSwapchain();
        return;
    }

    if (result != VK_SUCCESS &&
        result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error(
            "failed to acquire swapchain image"
        );
    }

    if (vulkan.inFlightFences[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(
            vulkan.device,
            1,
            &vulkan.inFlightFences[imageIndex],
            VK_TRUE,
            UINT64_MAX
        );
    }

    vulkan.inFlightFences[imageIndex] =
        inFlightFence;

    vkResetFences(
        vulkan.device,
        1,
        &inFlightFence
    );

    VkCommandBuffer commandBuffer =
        vulkan.commandBuffers[currentFrame];

    vkResetCommandBuffer(
        commandBuffer,
        0
    );

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(
            commandBuffer,
            &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to begin command buffer"
        );
    }

    VkRenderPassBeginInfo renderPassInfo{};

    renderPassInfo.sType =
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

    renderPassInfo.renderPass =
        vulkan.renderPass;

    renderPassInfo.framebuffer =
        vulkan.framebuffers[imageIndex];

    renderPassInfo.renderArea.offset = {
        0,
        0
    };

    renderPassInfo.renderArea.extent =
        vulkan.swapchainExtent;

    VkClearValue clearValues[2]{};

    clearValues[0].color = {
        {0.025f, 0.025f, 0.035f, 1.0f}
    };

    clearValues[1].depthStencil = {
        1.0f,
        0
    };

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(
        commandBuffer,
        &renderPassInfo,
        VK_SUBPASS_CONTENTS_INLINE
    );

    // The UI object is currently created below in mainLoop only
    // after this function has been expanded. For this initial
    // renderer pass, Pipeline is responsible for the actual scene.
    //
    // No model is loaded yet, so there is intentionally no geometry
    // draw here.

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to record command buffer"
        );
    }

    VkSemaphore waitSemaphores[] = {
        vulkan.imageAvailableSemaphores[currentFrame]
    };

    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphore signalSemaphores[] = {
        vulkan.renderFinishedSemaphores[currentFrame]
    };

    VkSubmitInfo submitInfo{};

    submitInfo.sType =
        VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores =
        waitSemaphores;

    submitInfo.pWaitDstStageMask =
        waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers =
        &commandBuffer;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores =
        signalSemaphores;

    if (vkQueueSubmit(
            vulkan.graphicsQueue,
            1,
            &submitInfo,
            inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to submit draw command buffer"
        );
    }

    VkSwapchainKHR swapchains[] = {
        vulkan.swapchain
    };

    VkPresentInfoKHR presentInfo{};

    presentInfo.sType =
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores =
        signalSemaphores;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains =
        swapchains;

    presentInfo.pImageIndices =
        &imageIndex;

    result = vkQueuePresentKHR(
        vulkan.presentQueue,
        &presentInfo
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR ||
        result == VK_SUBOPTIMAL_KHR ||
        vulkan.framebufferResized) {

        vulkan.framebufferResized = false;
        vulkan.recreateSwapchain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to present swapchain image"
        );
    }

    currentFrame =
        (currentFrame + 1) %
        VulkanBase::MAX_FRAMES_IN_FLIGHT;
}

void Application::cleanup()
{
    if (vulkan.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(vulkan.device);
    }

    vulkan.cleanup();

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
}

void Application::framebufferResizeCallback(
    GLFWwindow* window,
    int width,
    int height)
{
    (void)width;
    (void)height;

    auto* application =
        static_cast<Application*>(
            glfwGetWindowUserPointer(window)
        );

    if (application) {
        application->vulkan.framebufferResized = true;
    }
}

void Application::mouseButtonCallback(
    GLFWwindow* window,
    int button,
    int action,
    int mods)
{
    (void)mods;

    auto* application =
        static_cast<Application*>(
            glfwGetWindowUserPointer(window)
        );

    if (!application) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            application->rightMouseDown = true;
            application->firstMouse = true;

            glfwSetInputMode(
                window,
                GLFW_CURSOR,
                GLFW_CURSOR_DISABLED
            );
        }
        else if (action == GLFW_RELEASE) {
            application->rightMouseDown = false;

            glfwSetInputMode(
                window,
                GLFW_CURSOR,
                GLFW_CURSOR_NORMAL
            );
        }
    }
}

void Application::cursorPositionCallback(
    GLFWwindow* window,
    double xpos,
    double ypos)
{
    auto* application =
        static_cast<Application*>(
            glfwGetWindowUserPointer(window)
        );

    if (!application ||
        !application->rightMouseDown) {
        return;
    }

    if (application->firstMouse) {
        application->lastMouseX = xpos;
        application->lastMouseY = ypos;
        application->firstMouse = false;
        return;
    }

    const float dx =
        static_cast<float>(
            xpos - application->lastMouseX
        );

    const float dy =
        static_cast<float>(
            application->lastMouseY - ypos
        );

    application->lastMouseX = xpos;
    application->lastMouseY = ypos;

    application->camera.processMouseDelta(
        dx,
        dy
    );
}

void Application::run()
{
    try {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }
    catch (const std::exception& e) {
        std::cerr
            << "Fatal error: "
            << e.what()
            << '\n';

        cleanup();

        throw;
    }
}
