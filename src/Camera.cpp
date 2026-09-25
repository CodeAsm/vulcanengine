#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
 
glm::vec3 Camera::front() const {
    glm::vec3 f;
    f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    f.y = sin(glm::radians(pitch));
    f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    return glm::normalize(f);
}
 
glm::vec3 Camera::right() const {
    return glm::normalize(glm::cross(front(), glm::vec3(0, 1, 0)));
}
 
glm::vec3 Camera::up() const {
    return glm::normalize(glm::cross(right(), front()));
}
 
void Camera::processKeyboard(CameraMove dir, float dt) {
    float velocity = moveSpeed * dt;
    switch (dir) {
        case CameraMove::Forward:  position += front() * velocity; break;
        case CameraMove::Backward: position -= front() * velocity; break;
        case CameraMove::Left:     position -= right() * velocity; break;
        case CameraMove::Right:    position += right() * velocity; break;
        case CameraMove::Up:       position += glm::vec3(0, 1, 0) * velocity; break;
        case CameraMove::Down:     position -= glm::vec3(0, 1, 0) * velocity; break;
    }
}
 
void Camera::processMouseDelta(float dx, float dy) {
    yaw += dx * mouseSensitivity;
	
	if (yaw > 180.0f)
    	yaw -= 360.0f;

	if (yaw < -180.0f)
    	yaw += 360.0f;

    pitch -= dy * mouseSensitivity;
    pitch = std::clamp(pitch, -89.0f, 89.0f);
}
 
glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position, position + front(), glm::vec3(0, 1, 0));
}
 
glm::mat4 Camera::projectionMatrix(float aspect) const {
    glm::mat4 proj = glm::perspective(glm::radians(fovDegrees), aspect, 0.05f, 500.0f);
    proj[1][1] *= -1.0f; // Vulkan's clip space has inverted Y compared to OpenGL
    return proj;
}
