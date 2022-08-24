#pragma once

#include "gl_env.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// defines several possible options for camera movement
enum CameraMovement
{
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN
};

// defines several possible options for camera speed
enum CameraSpeed
{
    SPEED_NORMAL,
    SPEED_DOUBLE
};

// default camera parameters
const glm::vec3 POS   = glm::vec3(0.0f, 0.0f, 10.0f);
const glm::vec3 FRONT = glm::vec3(0.0f, 0.0f,  -1.0f);
const glm::vec3 UP    = glm::vec3(0.0f, 1.0f,  0.0f);
const float HEADING =  0.0f;
const float MAX_HEADING_RATE = 5.0f;
const float PITCH   =  0.0f;
const float MAX_PITCH_RATE = 5.0f;
const float FOV     =  45.0f;
const float SPEED = 5.0f;

// Camera system based on Quaternion
class Camera
{
public:
    glm::vec3 cameraPos;
private:
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    float heading;
    float maxHeadingRate;
    float pitch;
    float maxPitchRate;
    float fov;
    float speed;

    // change the pitch of the camera
    void ChangePitch(float degrees)
    {
        // Check bounds with the max pitch rate so that we aren't moving too fast
        if (degrees < -maxPitchRate)
            degrees = -maxPitchRate;
        else if (degrees > maxPitchRate)
            degrees = maxPitchRate;
        pitch += degrees;

        // Check bounds for the camera pitch
        if (pitch > 360.0f)
            pitch -= 360.0f;
        else if (pitch < -360.0f)
            pitch += 360.0f;
    }

    // change the heading of the camera
    void ChangeHeading(float degrees)
    {
        // Check bounds with the max heading rate so that we aren't moving too fast
        if (degrees < -maxHeadingRate)
            degrees = -maxHeadingRate;
        else if (degrees > maxHeadingRate)
            degrees = maxHeadingRate;

        // This controls how the heading is changed if the camera is pointed straight up or down
        // The heading delta direction changes
        if (pitch > 90 && pitch < 270 || (pitch < -90 && pitch > -270))
            heading -= degrees;
        else
            heading += degrees;

        // Check bounds for the camera heading
        if (heading > 360.0f)
            heading -= 360.0f;
        else if (heading < -360.0f)
            heading += 360.0f;
    }

public:
    Camera(glm::vec3 _pos = POS,
        glm::vec3 _front = FRONT,
        glm::vec3 _up = UP,
        float _heading = HEADING,
        float _maxHeadingRate = MAX_HEADING_RATE,
        float _pitch = PITCH,
        float _maxPitchRate = MAX_PITCH_RATE,
        float _fov = FOV,
        float _speed = SPEED)
    {
        cameraPos = _pos;
        cameraFront = _front;
        cameraUp = _up;
        heading = _heading;
        maxHeadingRate = _maxHeadingRate;
        pitch = _pitch;
        maxPitchRate = _maxPitchRate;
        fov = _fov;
        speed = _speed;
    }

    // returns the Field Of Vision
    float getFov()
    {
        return fov;
    }

    // returns the view matrix
    glm::mat4 getView()
    {
        return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    }

    // processes input received from a keyboard to move the camera
    void move(CameraMovement cameraMovement, CameraSpeed cameraSpeed, float deltaTime)
    {
        float distance = speed * deltaTime;
        if(cameraSpeed == SPEED_DOUBLE)
            distance *= 2.0;

        if (cameraMovement == MOVE_FORWARD)
            cameraPos += distance * cameraFront;
        if (cameraMovement == MOVE_BACKWARD)
            cameraPos -= distance * cameraFront;
        if (cameraMovement == MOVE_LEFT)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * distance;
        if (cameraMovement == MOVE_RIGHT)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * distance;
        if (cameraMovement == MOVE_UP)
            cameraPos += distance * cameraUp;
        if (cameraMovement == MOVE_DOWN)
            cameraPos -= distance * cameraUp;
    }

    // processes input received from a mouse to rotate the camera
    void rotate(float xoffset, float yoffset)
    {
        float sensitivity = 0.0005f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        ChangeHeading(xoffset);
        ChangePitch(yoffset);

        glm::vec3 axis = glm::cross(cameraFront, cameraUp);
        glm::quat pitch_quat = glm::angleAxis(pitch, axis);
        glm::quat heading_quat = glm::angleAxis(heading, cameraUp);
        glm::quat temp = glm::cross(pitch_quat, heading_quat);
        temp = glm::normalize(temp);

        cameraFront = glm::rotate(temp, cameraFront);

        heading *= .5;
        pitch *= .5;
    }

    // processes input received from a mouse to scroll the camera
    void scroll(float yoffset)
    {
        fov -= (float)yoffset;
        if (fov < 1.0f)
            fov = 1.0f;
        if (fov > 90.0f)
            fov = 90.0f;
    }
};