#define GLM_FORCE_SWIZZLE
#include "camera.h"

void Camera::SetDistance(float d)
{
    _distance = d;
}

void Camera::UpdateCameraVectors(float &vwX, float &vwY, float &panX, float &panY)
{
	// Initialize transformation matrix and position, up and right vectors
	glm::mat4 matTrans = glm::mat4(1.0f);
	glm::vec3 initPosition = glm::vec3(0.0f, 0.0f, _distance);
	glm::vec3 initUp = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 initRight = glm::vec3(1.0f, 0.0f, 0.0f);

	// Perform transformations for view angles
	matTrans = glm::rotate(matTrans, glm::radians(vwX), glm::vec3(0.0f, 0.0f, 1.0f));
	matTrans = glm::rotate(matTrans, glm::radians(vwY),glm::vec3(1.0f, 0.0f, 0.0f));

	// Calculate camera position without panning (for shapes that don't pan, like coordinate triad)
	_positionNoPan = (matTrans * glm::vec4(initPosition, 1.0f)).xyz();
	// Calculate Camera orientation vectors
	_up = (matTrans * glm::vec4(initUp, 1.0f)).xyz();
	_right = (matTrans * glm::vec4(initRight, 1.0f)).xyz();
	// Calculate Camera position and target
	_target = panX * _right + panY * _up;
	_position = _positionNoPan + _target;
}