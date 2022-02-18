#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera
{
public:
    // Constructor
    Camera() :
        _position { glm::vec3(0.0f, 0.0f, 0.0f) },
        _positionNoPan { glm::vec3(0.0f, 0.0f, 0.0f) },
        _right { glm::vec3(1.0f, 0.0f, 0.0f) },
        _target { glm::vec3(0.0f, 0.0f, 0.0f) },
        _targetNoPan { glm::vec3(0.0f, 0.0f, 0.0f) },
        _up { glm::vec3(0.0f, 1.0f, 0.0f) },
        _distance { 1.0f }
        {}
    
    // Getters and setters
    void SetDistance(float d);
    glm::vec3 GetPosition() { return _position; }
    glm::vec3 GetPositionNoPan() { return _positionNoPan; }
    glm::vec3 GetRight() { return _right; }
    glm::vec3 GetTarget() { return _target; }
    glm::vec3 GetTargetNoPan() { return _targetNoPan; }
    glm::vec3 GetUp() { return _up; }

    // Other functions
    void UpdateCameraVectors(float &vwX, float &vwY, float &panX, float &panY);
    
private:
    // Private variables
    glm::vec3 _position;
    glm::vec3 _positionNoPan;
    glm::vec3 _right;
    glm::vec3 _target;
    glm::vec3 _targetNoPan;
    glm::vec3 _up;
    float _distance;
};

#endif