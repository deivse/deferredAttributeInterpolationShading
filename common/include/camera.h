//-----------------------------------------------------------------------------
//  [PGR2] Camera
//  19/02/2014
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#pragma once
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include <utility>

// INTERNAL VARIABLES DEFINITIONS______________________________________________
namespace Tools {
class Camera
{
public:
    Camera()
      : Position(glm::vec3(0.0f)), Center(glm::vec3(0.0f, 0.0f, -1.0f)),
        Up(glm::vec3(0.0f, 1.0f, 0.0f)) {}

    void setPosition(const glm::vec3& position, const glm::vec3& center,
                     const glm::vec3& up) {
        Position = position;
        Center = center;
        Up = up;
    }

    void rotateAroundPosition(float dx, float dy) {
        const float angleZ = dy * PositionRotationSpeed;
        const float nextRotationX = glm::clamp(
          CurrentRotationX - angleZ, RotationRange.x, RotationRange.y);
        if (nextRotationX != CurrentRotationX) {
            CurrentRotationX = nextRotationX;

            const float angleY = dx * PositionRotationSpeed;
            const glm::vec3 axis
              = glm::normalize(glm::cross(Center - Position, Up));
            rotateView(angleZ, axis);
            rotateView(angleY, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }

    void rotateAroundCenter(float dx, float dy) {
        std::swap(Position, Center);
        rotateAroundPosition(-dx * CenterRotationSpeed,
                             dy * CenterRotationSpeed);
        std::swap(Position, Center);
    }

    void rotateView(float angle, const glm::vec3& axis) {
        const glm::vec3 currentView = Center - Position;
        const float cosTheta = glm::cos(glm::radians(angle));
        const float sinTheta = glm::sin(glm::radians(angle));

        glm::vec3 newView;
        // x-coordinate of newView
        newView.x
          = (cosTheta + (1.0f - cosTheta) * axis.x * axis.x) * currentView.x;
        newView.x += ((1.0f - cosTheta) * axis.x * axis.y - axis.z * sinTheta)
                     * currentView.y;
        newView.x += ((1.0f - cosTheta) * axis.x * axis.z + axis.y * sinTheta)
                     * currentView.z;

        // y-coordinate of newView
        newView.y = ((1.0f - cosTheta) * axis.x * axis.y + axis.z * sinTheta)
                    * currentView.x;
        newView.y
          += (cosTheta + (1.0f - cosTheta) * axis.y * axis.y) * currentView.y;
        newView.y += ((1.0f - cosTheta) * axis.y * axis.z - axis.x * sinTheta)
                     * currentView.z;

        // z-coordinate of newView
        newView.z = ((1.0f - cosTheta) * axis.x * axis.z - axis.y * sinTheta)
                    * currentView.x;
        newView.z += ((1.0f - cosTheta) * axis.y * axis.z + axis.x * sinTheta)
                     * currentView.y;
        newView.z
          += (cosTheta + (1.0f - cosTheta) * axis.z * axis.z) * currentView.z;

        // calculate new camera view center point
        Center = Position + newView;
    }

    void moveForward(float speed) {
        const glm::vec3 move = (Center - Position) * speed;
        Position += move;
        Center += move;
    }

    void zoom(float speed) {
        const glm::vec3 move = (Center - Position) * (speed * ZoomSpeed);
        Position += move;
    }

    void moveSide(float speed) {
        static bool s_fix = false;
        const glm::vec3 move
          = glm::normalize(glm::cross(Center - Position, Up)) * speed;
        Position += move;
        Center += move;
    }

    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(Position, Center, Up);
    }

    const glm::vec3& getPosition() { return Position; }

private:
    const glm::vec2 RotationRange = glm::vec2(-0.85f, 2.00f);
    const float ZoomSpeed = 0.01f;
    const float PositionRotationSpeed = 0.001f;
    const float CenterRotationSpeed = 10.0f;

    glm::vec3 Position;
    glm::vec3 Center;
    glm::vec3 Up;
    float CurrentRotationX = 0.0f;
}; // end of Camera
}; // end of namespace Tools
