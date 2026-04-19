#include "Camera.h"

Camera::Camera() : target(0.0f), distance(10.f), yaw(0.f), pitch(0.f) {}

glm::mat4x4 Camera::getViewMatrix()
{
    return glm::lookAt(getPosition(), target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Camera::getPosition()
{
    glm::vec3 pos;

    // pos.x = target.x + distance * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    // pos.y = target.y + distance * sin(glm::radians(pitch));
    // pos.z = target.z + distance * cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 150.0f;

    return pos;
}