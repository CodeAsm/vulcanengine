#include "Application.h"
#include "Buffer.h"
#include "ModelLoader.h"

#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

float currentTimeSeconds()
{
    using clock = std::chrono::steady_clock;

    static const auto start = clock::now();
    const auto now = clock::now();

    return std::chrono::duration<float>(now - start).count();
}

MeshData createTestCube()
{
    MeshData mesh;

    /*
     * A cube has 24 vertices rather than 8 because each face
     * needs its own normal and UV coordinates.
     */

    const glm::vec3 positions[] = {
        // Front
        {-1.0f, -1.0f,  1.0f},
        { 1.0f, -1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},

        // Back
        { 1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f},

        // Left
        {-1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f, -1.0f},

        // Right
        { 1.0f, -1.0f,  1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f,  1.0f},

        // Top
        {-1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},

        // Bottom
        {-1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f,  1.0f}
    };

    const glm::vec3 normals[] = {
        // Front
        { 0.0f,  0.0f,  1.0f},
        { 0.0f,  0.0f,  1.0f},
        { 0.0f,  0.0f,  1.0f},
        { 0.0f,  0.0f,  1.0f},

        // Back
        { 0.0f,  0.0f, -1.0f},
        { 0.0f,  0.0f, -1.0f},
        { 0.0f,  0.0f, -1.0f},
        { 0.0f,  0.0f, -1.0f},

        // Left
        {-1.0f,  0.0f,  0.0f},
        {-1.0f,  0.0f,  0.0f},
        {-1.0f,  0.0f,  0.0f},
        {-1.0f,  0.0f,  0.0f},

        // Right
        { 1.0f,  0.0f,  0.0f},
        { 1.0f,  0.0f,  0.0f},
        { 1.0f,  0.0f,  0.0f},
        { 1.0f,  0.0f,  0.0f},

        // Top
        { 0.0f,  1.0f,  0.0f},
        { 0.0f,  1.0f,  0.0f},
        { 0.0f,  1.0f,  0.0f},
        { 0.0f,  1.0f,  0.0f},

        // Bottom
        { 0.0f, -1.0f,  0.0f},
        { 0.0f, -1.0f,  0.0f},
        { 0.0f, -1.0f,  0.0f},
        { 0.0f, -1.0f,  0.0f}
    };

    const glm::vec2 uvs[] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},

        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},

        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},

        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},

        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},

        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    for (size_t i = 0; i < 24; ++i) {
        mesh.vertices.push_back({
            positions[i],
            normals[i],
            uvs[i]
        });
    }

    const uint32_t indices[] = {
         0,  1,  2,   2,  3,  0, // Front
         4,  5,  6,   6,  7,  4, // Back
         8,  9, 10,  10, 11,  8, // Left
        12, 13, 14,  14, 15, 12, // Right
        16, 17, 18,  18, 19, 16, // Top
        20, 21, 22,  22, 23, 20  // Bottom
    };

    mesh.indices.assign(
        std::begin(indices),
        std::end(indices)
    );

    mesh.materialIndex = 0;

    return mesh;
}

}

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

    glfwSetWindowUserPointer(window, this);

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

    glfwSetScrollCallback(
        window,
        scrollCallback
    );
}

void Application::initVulkan()
{
    vulkan.init(window);

    pipeline.init(vulkan);

    ui.init(window, vulkan);

    ui.setModelSelectedCallback(
        [this](const std::string& path) {
            loadModel(path);
        }
    );

    initTestCube();
}

void Application::loadModel(const std::string& path)
{
    ModelData data;

    try {
        data = ModelLoader::load(path);
    }
    catch (const std::exception& e) {
        statusMessage = std::string("Failed to load model: ") + e.what();
        std::cerr << statusMessage << '\n';
        return;
    }

    // Make sure the GPU is done with whatever it's currently drawing before
    // we destroy the previous model's buffers/textures underneath it.
    vkDeviceWaitIdle(vulkan.device);

    unloadCurrentModel();

    try {
        // ---- one texture (and descriptor set) per material ----
        modelTextures.reserve(data.materials.size());
        modelTextureDescriptorSets.reserve(data.materials.size());

        for (const MaterialData& material : data.materials) {
            auto texture = std::make_unique<Texture>();

            if (!material.diffuseTexturePath.empty()) {
                texture->loadFromFile(vulkan, material.diffuseTexturePath);
            } else {
                const glm::vec3 c = glm::clamp(material.fallbackColor, 0.0f, 1.0f);
                texture->createSolidColor(
                    vulkan,
                    static_cast<unsigned char>(c.r * 255.0f),
                    static_cast<unsigned char>(c.g * 255.0f),
                    static_cast<unsigned char>(c.b * 255.0f)
                );
            }

            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
            pipeline.createTextureDescriptor(vulkan, *texture, descriptorSet);

            modelTextureDescriptorSets.push_back(descriptorSet);
            modelTextures.push_back(std::move(texture));
        }

        // Every mesh should have a material (ModelLoader always assigns
        // one), but if a file somehow has none, add a plain fallback so
        // there's still something valid for meshes to point at.
        if (modelTextureDescriptorSets.empty()) {
            auto texture = std::make_unique<Texture>();
            texture->createSolidColor(vulkan, 220, 220, 220);

            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
            pipeline.createTextureDescriptor(vulkan, *texture, descriptorSet);

            modelTextureDescriptorSets.push_back(descriptorSet);
            modelTextures.push_back(std::move(texture));
        }

        // ---- upload geometry, tracking bounds for auto-fit ----
        glm::vec3 minBounds(std::numeric_limits<float>::max());
        glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

        modelMeshes.reserve(data.meshes.size());
        modelVertexCount = 0;

        for (const MeshData& mesh : data.meshes) {
            for (const Vertex& v : mesh.vertices) {
                minBounds = glm::min(minBounds, v.pos);
                maxBounds = glm::max(maxBounds, v.pos);
            }

            GPUMesh gpuMesh = uploadMesh(vulkan, mesh);

            // Guard against an out-of-range material index so draw() never
            // indexes past the end of modelTextureDescriptorSets.
            if (gpuMesh.materialIndex < 0 ||
                static_cast<size_t>(gpuMesh.materialIndex) >= modelTextureDescriptorSets.size()) {
                gpuMesh.materialIndex = 0;
            }

            modelVertexCount += mesh.vertices.size();
            modelMeshes.push_back(gpuMesh);
        }

        // ---- center the model on the origin and normalize its size ----
        // Downloaded models can be in wildly different units (millimeters,
        // meters, some arbitrary scale), so without this a model can appear
        // as a speck or as a wall filling the whole view.
        const glm::vec3 center = (minBounds + maxBounds) * 0.5f;
        const glm::vec3 extent = maxBounds - minBounds;
        const float maxExtent = std::max({extent.x, extent.y, extent.z});
        const float autoScale = (maxExtent > 0.0001f) ? (2.0f / maxExtent) : 1.0f;

        modelBaseTransform =
            glm::scale(glm::mat4(1.0f), glm::vec3(autoScale)) *
            glm::translate(glm::mat4(1.0f), -center);

        objectTransform = ObjectTransform{};

        modelMeshCount = modelMeshes.size();
        modelLabel = std::filesystem::path(path).filename().string();
        statusMessage =
            "Loaded " + modelLabel +
            " (" + std::to_string(modelMeshCount) + " mesh(es), " +
            std::to_string(modelVertexCount) + " vertices)";
        modelLoaded = true;
    }
    catch (const std::exception& e) {
        // Something failed partway through uploading to the GPU. Tear down
        // whatever we managed to create and fall back to the test cube
        // rather than leaving the app in a half-loaded state.
        unloadCurrentModel();
        statusMessage = std::string("Failed to load model: ") + e.what();
        std::cerr << statusMessage << '\n';
    }
}

void Application::unloadCurrentModel()
{
    for (GPUMesh& mesh : modelMeshes) {
        destroyGPUMesh(vulkan.device, mesh);
    }
    modelMeshes.clear();

    for (auto& texture : modelTextures) {
        texture->destroy(vulkan.device);
    }
    modelTextures.clear();
    modelTextureDescriptorSets.clear();

    modelLoaded = false;
    modelMeshCount = 0;
    modelVertexCount = 0;
}

glm::mat4 Application::buildObjectModelMatrix() const
{
    glm::mat4 t = glm::translate(glm::mat4(1.0f), objectTransform.position);
    t = glm::rotate(t, glm::radians(objectTransform.rotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
    t = glm::rotate(t, glm::radians(objectTransform.rotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
    t = glm::rotate(t, glm::radians(objectTransform.rotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
    t = glm::scale(t, glm::vec3(objectTransform.scale));
    return t * modelBaseTransform;
}

void Application::initTestCube()
{
    MeshData cube = createTestCube();

    cubeMesh = uploadMesh(
        vulkan,
        cube
    );

    cubeTexture.createSolidColor(
        vulkan,
        220,
        220,
        220
    );

    pipeline.createTextureDescriptor(
        vulkan,
        cubeTexture,
        cubeTextureDescriptorSet
    );
}

void Application::processInput(float deltaTime)
{
    if (!window)
        return;

    if (!ui.wantsKeyboard()) {

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.processKeyboard(
                CameraMove::Forward,
                deltaTime
            );

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.processKeyboard(
                CameraMove::Backward,
                deltaTime
            );

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.processKeyboard(
                CameraMove::Left,
                deltaTime
            );

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.processKeyboard(
                CameraMove::Right,
                deltaTime
            );

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.processKeyboard(
                CameraMove::Down,
                deltaTime
            );

        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            camera.processKeyboard(
                CameraMove::Up,
                deltaTime
            );

        // ---- move/rotate/scale the loaded object ----
        // Arrow keys translate in world X/Z, Page Up/Down in Y. Hold Left
        // Shift to rotate instead of translate. +/- scales it. R resets.
        const float moveStep = 1.2f * deltaTime;
        const float rotateStep = 60.0f * deltaTime;
        const bool shiftHeld = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            if (shiftHeld) objectTransform.rotationDegrees.y -= rotateStep;
            else objectTransform.position.x -= moveStep;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            if (shiftHeld) objectTransform.rotationDegrees.y += rotateStep;
            else objectTransform.position.x += moveStep;
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            if (shiftHeld) objectTransform.rotationDegrees.x += rotateStep;
            else objectTransform.position.z -= moveStep;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            if (shiftHeld) objectTransform.rotationDegrees.x -= rotateStep;
            else objectTransform.position.z += moveStep;
        }

        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
            objectTransform.position.y += moveStep;
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
            objectTransform.position.y -= moveStep;

        if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
            objectTransform.scale *= (1.0f + deltaTime);
        if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
            objectTransform.scale = std::max(0.001f, objectTransform.scale * (1.0f - deltaTime));

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            objectTransform.position = glm::vec3(0.0f);
            objectTransform.rotationDegrees = glm::vec3(0.0f);
            objectTransform.scale = 1.0f;
        }
    }
}

void Application::mainLoop()
{
    float previousTime =
        currentTimeSeconds();

    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        const float now =
            currentTimeSeconds();

        float deltaTime =
            now - previousTime;

        previousTime = now;

        if (deltaTime > 0.1f)
            deltaTime = 0.1f;

        ui.newFrame();

        UIState uiState;
        uiState.transform = &objectTransform;
        uiState.cameraMoveSpeed = &camera.moveSpeed;
        uiState.modelLabel = modelLabel;
        uiState.statusMessage = statusMessage;
        uiState.meshCount = modelLoaded ? modelMeshCount : size_t(1);
        uiState.vertexCount = modelLoaded ? modelVertexCount : size_t(24);
        uiState.cameraPosition = camera.position;
        uiState.cameraYaw = camera.yaw;
        uiState.cameraPitch = camera.pitch;

        ui.draw(uiState);

        processInput(deltaTime);

        drawFrame();
    }

    vkDeviceWaitIdle(
        vulkan.device
    );
}

void Application::drawFrame()
{
    VkFence inFlightFence =
        vulkan.inFlightFences[currentFrame];

    if (vkWaitForFences(
            vulkan.device,
            1,
            &inFlightFence,
            VK_TRUE,
            UINT64_MAX
        ) != VK_SUCCESS) {

        throw std::runtime_error(
            "failed waiting for in-flight fence"
        );
    }

    uint32_t imageIndex = 0;

    VkResult result =
        vkAcquireNextImageKHR(
            vulkan.device,
            vulkan.swapchain,
            UINT64_MAX,
            vulkan.imageAvailableSemaphores[
                currentFrame
            ],
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

    if (vulkan.imagesInFlight[imageIndex] !=
        VK_NULL_HANDLE) {

        vkWaitForFences(
            vulkan.device,
            1,
            &vulkan.imagesInFlight[imageIndex],
            VK_TRUE,
            UINT64_MAX
        );
    }

    vulkan.imagesInFlight[imageIndex] =
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
            &beginInfo
        ) != VK_SUCCESS) {

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

    clearValues[0].color = {{
        0.025f,
        0.025f,
        0.035f,
        1.0f
    }};

    clearValues[1].depthStencil = {
        1.0f,
        0
    };

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues =
        clearValues;

    vkCmdBeginRenderPass(
        commandBuffer,
        &renderPassInfo,
        VK_SUBPASS_CONTENTS_INLINE
    );

    /*
     * Update the camera uniform buffer for this frame.
     */
    pipeline.updateScene(
        vulkan,
        static_cast<uint32_t>(currentFrame),
        camera
    );

    /*
     * Draw whatever is currently loaded: the user's model if one was
     * loaded successfully, otherwise the placeholder test cube.
     */
    std::vector<GPUMesh> meshesToDraw;
    std::vector<VkDescriptorSet> textureDescriptorSetsToDraw;
    glm::mat4 modelMatrix(1.0f);

    if (modelLoaded && !modelMeshes.empty()) {
        meshesToDraw = modelMeshes;
        textureDescriptorSetsToDraw = modelTextureDescriptorSets;
        modelMatrix = buildObjectModelMatrix();
    } else {
        meshesToDraw = { cubeMesh };
        textureDescriptorSetsToDraw = { cubeTextureDescriptorSet };
    }

    pipeline.draw(
        vulkan,
        commandBuffer,
        static_cast<uint32_t>(currentFrame),
        meshesToDraw,
        textureDescriptorSetsToDraw,
        modelMatrix
    );

    /*
     * ImGui is rendered after the 3D scene,
     * so it appears on top of the cube.
     */
    ui.render(commandBuffer);

    vkCmdEndRenderPass(
        commandBuffer
    );

    if (vkEndCommandBuffer(
            commandBuffer
        ) != VK_SUCCESS) {

        throw std::runtime_error(
            "failed to record command buffer"
        );
    }

    VkSemaphore waitSemaphores[] = {
        vulkan.imageAvailableSemaphores[
            currentFrame
        ]
    };

    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphore signalSemaphores[] = {
        vulkan.renderFinishedSemaphores[
            currentFrame
        ]
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
            inFlightFence
        ) != VK_SUCCESS) {

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
    if (vulkan.device != VK_NULL_HANDLE)
        vkDeviceWaitIdle(vulkan.device);

    unloadCurrentModel();

    if (cubeMesh.vertexBuffer != VK_NULL_HANDLE ||
        cubeMesh.indexBuffer != VK_NULL_HANDLE) {

        destroyGPUMesh(
            vulkan.device,
            cubeMesh
        );
    }

    cubeTexture.destroy(
        vulkan.device
    );

    ui.cleanup();

    pipeline.cleanup(
        vulkan
    );

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

    if (application)
        application->vulkan.framebufferResized =
            true;
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

    if (!application)
        return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {

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

            // Only re-show the cursor if the object-drag button isn't also
            // still held down.
            if (!application->objectDragDown) {
                glfwSetInputMode(
                    window,
                    GLFW_CURSOR,
                    GLFW_CURSOR_NORMAL
                );
            }
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {

        // Right mouse button manipulates the loaded object instead of the
        // camera: plain drag moves it, drag while holding Left Shift
        // rotates it (see cursorPositionCallback).
        if (application->ui.wantsMouse()) {
            return;
        }

        if (action == GLFW_PRESS) {

            application->objectDragDown = true;
            application->firstObjectDrag = true;
        }
        else if (action == GLFW_RELEASE) {

            application->objectDragDown = false;
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

    if (!application) {
        return;
    }

    // Left mouse button: look around with the camera.
    if (application->rightMouseDown) {

        if (application->firstMouse) {

            application->lastMouseX = xpos;
            application->lastMouseY = ypos;

            application->firstMouse = false;
        }
        else {
            const float dx =
                static_cast<float>(xpos - application->lastMouseX);

            const float dy =
                static_cast<float>(application->lastMouseY - ypos);

            application->lastMouseX = xpos;
            application->lastMouseY = ypos;

            application->camera.processMouseDelta(dx, dy);
        }
    }

    // Right mouse button: move (or, with Left Shift held, rotate) the
    // loaded object.
    if (application->objectDragDown) {

        if (application->firstObjectDrag) {

            application->lastObjectMouseX = xpos;
            application->lastObjectMouseY = ypos;

            application->firstObjectDrag = false;
        }
        else {
            const float dx =
                static_cast<float>(xpos - application->lastObjectMouseX);

            const float dy =
                static_cast<float>(application->lastObjectMouseY - ypos);

            application->lastObjectMouseX = xpos;
            application->lastObjectMouseY = ypos;

            const bool rotateMode =
                glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

            if (rotateMode) {
                const float rotateSpeed = 0.3f;
                application->objectTransform.rotationDegrees.y += dx * rotateSpeed;
                application->objectTransform.rotationDegrees.x += dy * rotateSpeed;
            }
            else {
                const float dragSpeed = 0.0035f;
                const glm::vec3 right = application->camera.right();
                const glm::vec3 up = application->camera.up();

                application->objectTransform.position +=
                    right * (dx * dragSpeed) + up * (dy * dragSpeed);
            }
        }
    }
}

void Application::scrollCallback(
    GLFWwindow* window,
    double xoffset,
    double yoffset)
{
    (void)xoffset;

    auto* application =
        static_cast<Application*>(
            glfwGetWindowUserPointer(window)
        );

    if (!application || application->ui.wantsMouse()) {
        return;
    }

    const bool altHeld =
        glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;

    if (altHeld) {
        // Alt + scroll: scale the loaded object.
        const float factor = 1.0f + static_cast<float>(yoffset) * 0.1f;
        application->objectTransform.scale = std::clamp(
            application->objectTransform.scale * factor,
            0.001f,
            1000.0f
        );
    }
    else {
        // Plain scroll: adjust how fast WASD flies the camera.
        application->camera.moveSpeed = std::clamp(
            application->camera.moveSpeed + static_cast<float>(yoffset) * 0.5f,
            0.1f,
            50.0f
        );
    }
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