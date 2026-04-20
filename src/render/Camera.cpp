#include "Camera.h"
#include <iostream>
#include <GLFW/glfw3.h>

Camera::Camera()
{
    // TODO: fix hardcoded values
    m_screenWidth = 1280;
    m_screenHeight = 720;

    m_target = glm::vec3(32.f, 32.f, 32.f);
    m_pitch = 15.f;
    m_yaw = 80.f;
    m_distance = 100.0f;

    m_firstMove = true;

    buildProjectionMatrix(1280, 720);
    buildViewMatrix();
}

void Camera::onMouseMove(double xpos, double ypos)
{
    if (m_firstMove)
    {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMove = false;
        return;
    }

    float dx = xpos - m_lastX;
    float dy = ypos - m_lastY;
    m_lastX = xpos;
    m_lastY = ypos;

    const float sensitivity = 0.1f;
    m_yaw += dx * sensitivity;
    m_pitch += dy * sensitivity;

    m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

    buildViewMatrix();
}

void Camera::onMouseButton(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        m_firstMove = true;
    }
}

void Camera::onMouseScroll(double xoffset, double yoffset)
{
    // TODO: define min and max zoom
    m_distance -= yoffset * 10;
    m_yaw += xoffset * 10;
    buildViewMatrix();
}

void Camera::buildProjectionMatrix(int width, int height)
{
    m_screenWidth = width;
    m_screenHeight = height;

    m_projection = glm::perspective(
        glm::radians(60.0f),
        (float)width / (float)height,
        0.1f,
        1000.0f);
}

void Camera::buildViewMatrix()
{
    m_view = glm::lookAt(getPosition(), m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 Camera::getPosition()
{
    glm::vec3 pos;

    pos.x = m_target.x + m_distance * cos(glm::radians(m_pitch)) * cos(glm::radians(m_yaw));
    pos.y = m_target.y + m_distance * sin(glm::radians(m_pitch));
    pos.z = m_target.z + m_distance * cos(glm::radians(m_pitch)) * sin(glm::radians(m_yaw));

    return pos;
}

glm::vec3 Camera::screenToWorld(int screenX, int screenY)
{
    float ndcX = (2.0f * screenX) / m_screenWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY) / m_screenHeight;

    glm::mat4 invVP = glm::inverse(m_projection * m_view);

    glm::vec4 rayNear = invVP * glm::vec4(ndcX, ndcY, 0.0f, 1.0f); // near plane
    glm::vec4 rayFar = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);  // far plane

    rayNear /= rayNear.w;
    rayFar /= rayFar.w;

    glm::vec3 rayOrigin = glm::vec3(rayNear);
    glm::vec3 rayDir = glm::normalize(glm::vec3(rayFar) - rayOrigin);

    if (glm::abs(rayDir.z) < 1e-6f)
        return glm::vec3(0);

    float t = -rayOrigin.z / rayDir.z;
    glm::vec3 worldPos = rayOrigin + t * rayDir;

    return worldPos;
}