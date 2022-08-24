#include <cstdlib>
#include <cstdio>
#include <config.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <iostream>
#include <windows.h>

#include "gl_env.h"

#include <glm/gtc/matrix_transform.hpp>

#include "utils_camera.h"
#include "utils_shader_program.h"
#include "utils_mesh.h"
#include "utils_model.h"
#include "utils_light.h"

#include "renderer_cube_quad.h"
#include "renderer_off.h"
#include "renderer_ssao.h"
#include "renderer_ssdo.h"
#include "renderer_both.h"
#include "renderer_image.h"

#ifndef MY_PI
#define MY_PI (3.1415926535897932)
#endif

// callback functions
static void error_callback(int error, const char *description) {
    fprintf(stderr, "Error: %s\n", description);
}
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// camera & info
Camera camera;
bool firstMouse = true;
float cameraLastX   =  0.0f;
float cameraLastY   =  0.0f;
int cameraFree = 1;
int showInfo   = 1;
int plainModel = 0;

// keyboard input
struct InputData
{
    int state_1, state_2, state_3, state_4;
};
void processInput(GLFWwindow *window, InputData &inputData);

// timing
float inputDeltaTime = 0.0f;
float inputLastTime = 0.0f;

int main(int argc, char *argv[])
{
    // create and init the window
    GLFWwindow *window;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

#ifdef __APPLE__ // for macos
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(800, 800, "OpenGL output", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (glewInit() != GLEW_OK)
        exit(EXIT_FAILURE);

    // set mouse callbacks and modes
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // keyboard input data
    InputData inputData;
    int renderMode = 1;

    // enable depth test
    glEnable(GL_DEPTH_TEST);

    // load the models
    Model my3DModel(DATA_DIR"/Luminaris/FBX/Luminaris.fbx");

    // lights
    const unsigned int NR_LIGHTS = 8;
    std::vector<Light> lights;
    srand(114514);
    for (unsigned int i = 0; i < NR_LIGHTS; i++)
    {
        // calculate slightly random offsets
        float xPos = static_cast<float>(((rand() % 100) / 100.0) * 4.0 - 2.0);
        float yPos = static_cast<float>(((rand() % 100) / 100.0) * 4.0 - 2.0);
        float zPos = static_cast<float>(((rand() % 100) / 100.0) * 8.0 - 4.0);
        // also calculate random color
        float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
        float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
        float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
        lights.push_back(Light(glm::vec3(xPos, yPos, zPos), glm::vec3(rColor, gColor, bColor)));
    }

    // renderers
    RendererOFF  rendererOFF;
    RendererSSAO rendererSSAO;
    RendererSSDO rendererSSDO;
    RendererBoth rendererBoth;
    RendererImage rendererImage;

    // the main loop
    float passed_time;
    while (!glfwWindowShouldClose(window)) {
        passed_time = (float) glfwGetTime();

        // timing and process the input
        inputDeltaTime = passed_time - inputLastTime;
        inputLastTime = passed_time;
        processInput(window, inputData);
        if (inputData.state_1 == GLFW_PRESS)
            renderMode = 1;
        else if (inputData.state_2 == GLFW_PRESS)
            renderMode = 2;
        else if (inputData.state_3 == GLFW_PRESS)
            renderMode = 3;
        else if (inputData.state_4 == GLFW_PRESS)
            renderMode = 4;

        // get width & height
        float ratio;
        int width, height;

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        // clear
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render
        if (renderMode == 1)
            rendererOFF.render(my3DModel, camera, lights, width, height, plainModel);
        else if (renderMode == 2)
            rendererSSAO.render(my3DModel, camera, lights, width, height, plainModel);
        else if (renderMode == 3)
            rendererSSDO.render(my3DModel, camera, lights, width, height, plainModel);
        else if (renderMode == 4)
            rendererBoth.render(my3DModel, camera, lights, width, height, plainModel);

        if (showInfo == 1)
            rendererImage.draw(renderMode, cameraFree);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}

void processInput(GLFWwindow *window, InputData &inputData)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

    // get options
    inputData.state_1 = glfwGetKey(window, GLFW_KEY_1);
    inputData.state_2 = glfwGetKey(window, GLFW_KEY_2);
    inputData.state_3 = glfwGetKey(window, GLFW_KEY_3);
    inputData.state_4 = glfwGetKey(window, GLFW_KEY_4);

    // cameraFree & showInfo & plainModel
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
        cameraFree = 1;
    else if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        {cameraFree = 0; firstMouse=1;}

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        showInfo = 1;
    else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        showInfo = 0;

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        plainModel = 1;
    else if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        plainModel = 0;

    // camera change position
    if (!cameraFree) return;
    CameraSpeed cameraSpeed = SPEED_NORMAL;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraSpeed = SPEED_DOUBLE;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.move(MOVE_FORWARD, cameraSpeed, inputDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.move(MOVE_BACKWARD, cameraSpeed, inputDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.move(MOVE_LEFT, cameraSpeed, inputDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.move(MOVE_RIGHT, cameraSpeed, inputDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.move(MOVE_UP, cameraSpeed, inputDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.move(MOVE_DOWN, cameraSpeed, inputDeltaTime);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (!cameraFree) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        cameraLastX = xpos;
        cameraLastY = ypos;
        firstMouse = false;
    }

    float xoffset = cameraLastX - xpos;
    float yoffset = cameraLastY - ypos;
    camera.rotate(xoffset, yoffset);

    cameraLastX = xpos;
    cameraLastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!cameraFree) return;

    camera.scroll(yoffset);
}