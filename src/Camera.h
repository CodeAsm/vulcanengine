#pragma once
#include <glm/glm.hpp>
 
enum class CameraMove { Forward, Backward, Left, Right, Up, Down };
 
class Camera {
public:
    glm::vec3 position{0.0f, 1.0f, 4.0f};
    float yaw = -90.0f;   // degrees, facing -Z
    float pitch = 0.0f;
    float fovDegrees = 60.0f;
    float moveSpeed = 3.0f;
    float mouseSensitivity = 0.12f;
 
    glm::vec3 front() const;
    glm::vec3 right() const;
    glm::vec3 up() const;
 
    void processKeyboard(CameraMove dir, float dt);
    void processMouseDelta(float dx, float dy);
 
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;
};
 
