#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    Camera();

    void onMouseMove(double xpos, double ypos);
    void onMouseButton(int button, int action, int mods);

    void buildProjectionMatrix(int width, int height);
    glm::mat4x4 getProjectionMatrix() { return m_projection; };

    void buildViewMatrix();
    glm::mat4x4 getViewMatrix() { return m_view; }

    glm::vec3 getPosition();
    glm::vec3 screenToWorld(int x, int y);

private:
    glm::mat4x4 m_projection;
    glm::mat4x4 m_view;

    glm::vec3 m_target;

    float m_pitch;
    float m_yaw;
    float m_distance;

    bool m_firstMove;
    int m_lastX;
    int m_lastY;

    int m_screenWidth;
    int m_screenHeight;
};