#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    Camera();

    glm::mat4x4 getViewMatrix();
    glm::vec3 getPosition();

private:
    glm::vec3 target;
    float distance;
    float yaw;
    float pitch;
};