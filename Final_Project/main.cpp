#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"
#include <iostream>
#include <cmath>


#ifdef __cplusplus
extern "C" {
#endif

    __declspec(dllexport) int NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#ifdef __cplusplus
}
#endif

// window
gps::Window myWindow;

const unsigned int SHADOW_WIDTH = 10000;
const unsigned int SHADOW_HEIGHT = 10000;

int retina_width, retina_height;
GLFWwindow* glWindow = NULL;
int glWindowWidth = 1920;
int glWindowHeight = 1080;
bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = retina_width / 2.0;
float lastY = retina_height / 2.0;
float fov = 45.0f;

float t = 0;
glm::vec3 controlPoints[5] = { glm::vec3 {-120.0f, 45.0f, 20.0f},
                              glm::vec3 {-30.0f, 25.0f, -10.0f},
                              glm::vec3 {70.0f, 15.0f, 30.0f} };

struct rectangle{
    glm::vec3 bottom_left;
    glm::vec3 bottom_right;
    glm::vec3 top_right;
    glm::vec3 top_left;
} rectangles[5];

struct circle {
    glm::vec3 center;
    float radius;
} circles[3];

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat4 tumbleWeedMatrix = glm::mat4(1.0f);

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

//fog parameters
bool foginit = false;
GLfloat fogDensity = 0.005f;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLuint shadowMapFBO;
GLuint depthMapTexture;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 3.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.2f;

GLboolean pressedKeys[1024];

// models
gps::Model3D teapot;
gps::Model3D bigScene;
gps::Model3D ground;
gps::Model3D tumbleWeed;
gps::Model3D tumbleWeed2;
gps::Model3D tumbleWeed3;
gps::Model3D tumbleWeed4;
gps::Model3D eagleWings;
gps::Model3D eagleBody;
gps::Model3D eagleFeathers;
gps::Model3D eagleTail;
gps::Model3D lamp;
gps::Model3D lamp2;
gps::Model3D lamp3;
gps::Model3D specialWeed;

float bodyAngle;
float feathersAngle;
GLfloat angle;
GLfloat trans = 0.0f;
GLfloat transWeedX = 0.0f;
GLfloat transWeedZ = 0.0f;
GLfloat angleWeedZ = 0.0f;
GLfloat angleWeedX = 0.0f;
glm::vec3 currentWeedPosition;
bool objGen = false;
bool objStop = false;
bool isNight = false;
bool scenePrev = false;
int featherDirection = 0;

std::vector<const GLchar*> faces;
gps::SkyBox mySkyBox;


// shaders
gps::Shader myBasicShader;
gps::Shader skyboxShader;
gps::Shader depthMapShader;
int nr_rectangles = 0;
int nr_circles = 0;
void addRectangle(glm::vec3 b_l, glm::vec3 b_r, glm::vec3 t_r, glm::vec3 t_l, int index) {
    rectangles[index].bottom_left = b_l;
    rectangles[index].bottom_right = b_r;
    rectangles[index].top_right = t_r;
    rectangles[index].top_left = t_l;
}

void addCircle(glm::vec3 c, float r, int index) {
    circles[index].center = c;
    circles[index].radius = r;
}

bool checkInRectangle(glm::vec3 currentPos, rectangle r[]) {
    for (int i = 0; i < nr_rectangles; i++) {
        if (currentPos.x >= r[i].bottom_left.x && currentPos.x <= r[i].bottom_right.x
            && currentPos.z <= r[i].bottom_left.z && currentPos.z >= r[i].top_left.z) {
            return true;
        }
    }
    return false;
}

float computeDistance(float x1, float y1, float x2, float y2) {
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

bool checkInCircle(glm::vec3 currentPos, circle c[]) {
    for (int i = 0; i < nr_circles; i++) {
        if (computeDistance(currentPos.x, currentPos.z, c[i].center.x, c[i].center.z) <= c[i].radius)
            return true;
    }
    return false;
}

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                error = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                error = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
    glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

    myBasicShader.useShaderProgram();

    // set projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
    //send matrix data to shader
    GLint projLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


    // set Viewport transform
    glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    //TODO
    if (!scenePrev) {
        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.15f; // change this value to your liking
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        /*glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        //cameraFront = glm::normalize(front);*/
        myCamera.rotate(pitch, yaw);

        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }
}

glm::vec3 takeNextPosition(float transWeedX, float transWeedZ) {
    tumbleWeedMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(transWeedX, 0, 0));
    tumbleWeedMatrix = glm::translate(tumbleWeedMatrix, glm::vec3(0, 0, transWeedZ));
    tumbleWeedMatrix = glm::translate(tumbleWeedMatrix, glm::vec3(-60.0, 0.6f, 2.5f));
    return glm::vec3(tumbleWeedMatrix * glm::vec4(0, 0, 0, 1.0f));
}

void processMovement() {
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		//update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

    if (pressedKeys[GLFW_KEY_X]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (pressedKeys[GLFW_KEY_Z]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (pressedKeys[GLFW_KEY_C]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }

    if (pressedKeys[GLFW_KEY_F]) {

        myBasicShader.useShaderProgram();
        foginit = true;
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "foginit"), foginit);

        skyboxShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(skyboxShader.shaderProgram, "foginit"), foginit);

    }

    if (pressedKeys[GLFW_KEY_G]) {
        myBasicShader.useShaderProgram();
        foginit = false;
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "foginit"), foginit);

        skyboxShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(skyboxShader.shaderProgram, "foginit"), foginit);
    }

    if (pressedKeys[GLFW_KEY_H])
    {
        fogDensity += 0.0002f;
        fogDensity = glm::min(fogDensity, 1.0f);
    }

    if (pressedKeys[GLFW_KEY_J])
    {
        fogDensity -= 0.0002f;
        fogDensity = glm::max(fogDensity, 0.0f);
    }

    // start the wind and generate the objects
    if (pressedKeys[GLFW_KEY_O]) {
        objGen = true;
    }

    // stop the wind and destroy the objects
    if (pressedKeys[GLFW_KEY_P]) {
        objGen = false;
    }

    // it is night and we open the flashlight
    if (pressedKeys[GLFW_KEY_N]) {
        isNight = true;
        myBasicShader.useShaderProgram();
        glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "isNight"), isNight);
        skyboxShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(skyboxShader.shaderProgram, "isNight"), isNight);
    }

    //it is day
    if (pressedKeys[GLFW_KEY_M]) {
        isNight = false;
        myBasicShader.useShaderProgram();
        glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "isNight"), isNight);
        skyboxShader.useShaderProgram();
        glUniform1i(glGetUniformLocation(skyboxShader.shaderProgram, "isNight"), isNight);
    }

    //preview of the scene
    if (pressedKeys[GLFW_KEY_U]) {
        scenePrev = true;
        t = 0;
        myCamera.setPosition(controlPoints[0]);
    }

    //stop the preview
    if (pressedKeys[GLFW_KEY_I]) {
        scenePrev = false;
    }

    
    //instances for moving the tumbleweed
    if (pressedKeys[GLFW_KEY_DOWN]) {
        glm::vec3 nextPosition = takeNextPosition(transWeedX, transWeedZ + 0.5);
        if (!checkInRectangle(nextPosition, rectangles) &&
            !checkInCircle(nextPosition, circles)) {
            transWeedZ += 0.5;
        }
        angleWeedX = abs(angleWeedX);
        angleWeedX += 5;
        if (angleWeedX > 360) {
            angleWeedX -= 360;
        }
    }
    if (pressedKeys[GLFW_KEY_UP]) {
        glm::vec3 nextPosition = takeNextPosition(transWeedX, transWeedZ - 0.5);
        if (!checkInRectangle(nextPosition, rectangles) &&
            !checkInCircle(nextPosition, circles)) {
            transWeedZ -= 0.5;
        }
        angleWeedX = -abs(angleWeedX);
        angleWeedX -= 5;
        if (angleWeedX < -360) {
            angleWeedX += 360;
        }
    }
    if (pressedKeys[GLFW_KEY_LEFT]) {
        glm::vec3 nextPosition = takeNextPosition(transWeedX - 0.5, transWeedZ);
        if (!checkInRectangle(nextPosition, rectangles) &&
            !checkInCircle(nextPosition, circles)) {
            transWeedX -= 0.5;
        }
        angleWeedZ = abs(angleWeedZ);
        angleWeedZ += 5;
        if (angleWeedZ > 360) {
            angleWeedZ -= 360;
        }
    }
    if (pressedKeys[GLFW_KEY_RIGHT]) {
        glm::vec3 nextPosition = takeNextPosition(transWeedX + 0.5, transWeedZ);
        if (!checkInRectangle(nextPosition, rectangles) &&
            !checkInCircle(nextPosition, circles)) {
            transWeedX += 0.5;
        }
        angleWeedZ = -abs(angleWeedZ);
        angleWeedZ -= 5;
        if (angleWeedZ < -360) {
            angleWeedZ += 360;
        }
    }
}

void initOpenGLWindow() {
    myWindow.Create(1920, 1080, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {
    teapot.LoadModel("models/teapot/teapot20segUT.obj");
    bigScene.LoadModel("models/scene/KB3D_Gaea-Native.obj");
    tumbleWeed.LoadModel("models/tumbleweed/tumbleweed.obj");
    tumbleWeed2.LoadModel("models/tumbleweed2/tumbleweed2.obj");
    tumbleWeed3.LoadModel("models/tumbleweed3/tumbleweed3.obj");
    tumbleWeed4.LoadModel("models/tumbleweed4/tumbleweed4.obj");
    specialWeed.LoadModel("models/specialWeed/specialWeed.obj");
    eagleBody.LoadModel("models/eagle/body.obj");
    eagleFeathers.LoadModel("models/eagle/feathers.obj");
    eagleWings.LoadModel("models/eagle/wings.obj");
    eagleTail.LoadModel("models/eagle/tail.obj");
    lamp.LoadModel("models/lamp/lamp.obj");
    lamp2.LoadModel("models/lamp2/lamp2.obj");
    lamp3.LoadModel("models/lamp3/lamp3.obj");
    ground.LoadModel("models/ground/untitled.obj");
    faces.push_back("skybox/right.tga");
    faces.push_back("skybox/left.tga");
    faces.push_back("skybox/top.tga");
    faces.push_back("skybox/bottom.tga");
    faces.push_back("skybox/back.tga");
    faces.push_back("skybox/front.tga");
    mySkyBox.Load(faces);
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
    skyboxShader.loadShader(
        "shaders/skyboxShader.vert", 
        "shaders/skyboxShader.frag");
    depthMapShader.loadShader(
        "shaders/depthMapShader.vert",
        "shaders/depthMapShader.frag");
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix for teapot
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(-80.0f, 125.0f, 50.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    //spot light
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "cutoff"), glm::cos(glm::radians(12.5f)));
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "outerCutoff"), glm::cos(glm::radians(17.5f)));
    glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "isNight"), isNight);

    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLightDir"), 1, glm::value_ptr(myCamera.getCameraDirection()));
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLightPos"), 1, glm::value_ptr(myCamera.getCameraPosition()));

    //point light
    glm::vec3 pointLightPos = glm::vec3(9.0f, 3.5f, 0);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightLocation"), 1, glm::value_ptr(pointLightPos));

    glm::vec3 pointLightPos2 = glm::vec3(-3.0f, 3.5f, 0);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightLocation2"), 1, glm::value_ptr(pointLightPos2));

    glm::vec3 pointLightPos3 = glm::vec3(-14.0f, 3.5f, 0);
    glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "pointLightLocation3"), 1, glm::value_ptr(pointLightPos3));

}

void initFBO() {
    //TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
    //generate FBO ID
    glGenFramebuffers(1, &shadowMapFBO);

    //create depth texture for FBO
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    //attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


glm::mat4 computeLightSpaceTrMatrix() {
    //TODO - Return the light-space transformation matrix
    glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat near_plane = 0.1f, far_plane = 400.0f;
    glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, near_plane, far_plane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

    return lightSpaceTrMatrix;
}

void renderTeapot(gps::Shader shader, bool depthPass) {
    // select active shader program
    shader.useShaderProgram();
    //send teapot model matrix data to shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    //send teapot normal matrix data to shader
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    // draw teapot
    teapot.Draw(shader);
}

void renderAllObjects(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    bigScene.Draw(shader);
}

void renderSkyBox(gps::Shader shader) {
    shader.useShaderProgram();
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "view"), 1, GL_FALSE,
        glm::value_ptr(view));

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "projection"), 1, GL_FALSE,
        glm::value_ptr(projection));
    mySkyBox.Draw(shader, view, projection);
}

void renderGround(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    ground.Draw(shader);
}

void renderTumbleWeed(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    if (!objStop) {
        angle += 1.2f;
        trans += 0.03f;
    }
    if (trans > 76) {
        objStop = true;
    }

    tumbleWeedMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(trans, 0, 0));
    tumbleWeedMatrix = glm::translate(tumbleWeedMatrix, glm::vec3(-40.0, 0.6f, 2.5f));
    tumbleWeedMatrix = glm::rotate(tumbleWeedMatrix, glm::radians(-angle), glm::vec3(0, 0, 1));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(tumbleWeedMatrix));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * tumbleWeedMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    tumbleWeed.Draw(shader);

    tumbleWeedMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(trans * 1.12, 0, 0));
    tumbleWeedMatrix = glm::translate(tumbleWeedMatrix, glm::vec3(-45.0, 0.3f, 4.25f));
    tumbleWeedMatrix = glm::rotate(tumbleWeedMatrix, glm::radians(-(angle * 1.2f)), glm::vec3(0, 0, 1));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(tumbleWeedMatrix));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * tumbleWeedMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    tumbleWeed2.Draw(shader);

    tumbleWeedMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(trans * 1.05, 0, 0));
    tumbleWeedMatrix = glm::translate(tumbleWeedMatrix, glm::vec3(-42.0, 0.4f, 6.5f));
    tumbleWeedMatrix = glm::rotate(tumbleWeedMatrix, glm::radians(-(angle * 1.07f)), glm::vec3(0, 0, 1));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(tumbleWeedMatrix));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * tumbleWeedMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    tumbleWeed3.Draw(shader);

    tumbleWeedMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(trans * 0.9, 0, 0));
    tumbleWeedMatrix = glm::translate(tumbleWeedMatrix, glm::vec3(-35.0, 0.7f, 8.5f));
    tumbleWeedMatrix = glm::rotate(tumbleWeedMatrix, glm::radians(-(angle * 0.95f)), glm::vec3(0, 0, 1));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(tumbleWeedMatrix));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * tumbleWeedMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    tumbleWeed4.Draw(shader);
}

void renderSpecialWeed(gps::Shader shader, bool depthPass) {
    tumbleWeedMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(transWeedX, 0, 0));
    tumbleWeedMatrix = glm::translate(tumbleWeedMatrix, glm::vec3(0, 0, transWeedZ));
    tumbleWeedMatrix = glm::translate(tumbleWeedMatrix, glm::vec3(-60.0, 0.6f, 2.5f));
    tumbleWeedMatrix = glm::rotate(tumbleWeedMatrix, glm::radians(angleWeedZ), glm::vec3(0, 0, 1));
    tumbleWeedMatrix = glm::rotate(tumbleWeedMatrix, glm::radians(angleWeedX), glm::vec3(1, 0, 0));
    currentWeedPosition = glm::vec3(tumbleWeedMatrix * glm::vec4(0, 0, 0, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(tumbleWeedMatrix));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * tumbleWeedMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    specialWeed.Draw(shader);
}

void renderEagleBody(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    glm::mat4 eagleBodyMatrix;
    eagleBodyMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(bodyAngle), glm::vec3(0, 1, 0));
    eagleBodyMatrix = glm::translate(eagleBodyMatrix, glm::vec3(-10.0f, 12.0f, 6.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(eagleBodyMatrix));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * eagleBodyMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    eagleBody.Draw(shader);
}

void renderEagleWings(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    glm::mat4 eagleWingsMatrix;
    eagleWingsMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(bodyAngle), glm::vec3(0, 1, 0));
    eagleWingsMatrix = glm::translate(eagleWingsMatrix, glm::vec3(-10.0f, 12.0f, 6.0f));
    eagleWingsMatrix = glm::rotate(eagleWingsMatrix, glm::radians(feathersAngle), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(eagleWingsMatrix));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * eagleWingsMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    eagleWings.Draw(shader);
}

void renderEagleFeathers(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    glm::mat4 eagleFeathersMatrix;
    eagleFeathersMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(bodyAngle), glm::vec3(0, 1, 0));
    eagleFeathersMatrix = glm::translate(eagleFeathersMatrix, glm::vec3(-10.0f, 12.0f, 6.0f));
    eagleFeathersMatrix = glm::rotate(eagleFeathersMatrix, glm::radians(1.5f * feathersAngle), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(eagleFeathersMatrix));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * eagleFeathersMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    eagleFeathers.Draw(shader);
}

void renderEagleTail(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    glm::mat4 eagleTailMatrix;
    eagleTailMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(bodyAngle), glm::vec3(0, 1, 0));
    eagleTailMatrix = glm::translate(eagleTailMatrix, glm::vec3(-10.0f, 12.0f, 6.0f));
    eagleTailMatrix = glm::rotate(eagleTailMatrix, glm::radians(0.2f * feathersAngle), glm::vec3(1, 0, 0));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(eagleTailMatrix));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * eagleTailMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    eagleTail.Draw(shader);
}

void renderLamp(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    glm::mat4 lampMatrix = glm::mat4(1.0f);
    lampMatrix = glm::translate(lampMatrix, glm::vec3(9.0f, 0, 0));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(lampMatrix));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * lampMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    lamp.Draw(shader);
}

void renderLamp2(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    glm::mat4 lampMatrix = glm::mat4(1.0f);
    lampMatrix = glm::translate(lampMatrix, glm::vec3(-3.0f, 0, 0));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(lampMatrix));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * lampMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    lamp2.Draw(shader);
}

void renderLamp3(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    glm::mat4 lampMatrix = glm::mat4(1.0f);
    lampMatrix = glm::translate(lampMatrix, glm::vec3(-14.0f, 0, 0));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(lampMatrix));
    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * lampMatrix));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    lamp3.Draw(shader);
}

glm::vec3 bezierCurve(glm::vec3 a[], float t) {
    return (1 - t) * ((1 - t) * a[0] + t * a[1]) +
        t * ((1 - t) * a[1] + t * a[2]);
}

void scenePreview() {
    t += 0.003f;
    myCamera.setPosition(bezierCurve(controlPoints, t));
    myCamera.setTarget(bezierCurve(controlPoints, t + 0.003f));
    myCamera.setFrontDirection(glm::normalize(myCamera.getCameraTarget() - myCamera.getCameraPosition()));
    myCamera.setUpDirection(glm::vec3(0, 1, 0));

    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    if (t >= 1)
        scenePrev = false;
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!isNight) {
        depthMapShader.useShaderProgram();
        glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(computeLightSpaceTrMatrix()));
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        renderTeapot(depthMapShader, true);
        renderGround(depthMapShader, true);
        renderLamp(depthMapShader, true);
        renderLamp2(depthMapShader, true);
        renderLamp3(depthMapShader, true);
        if (objGen) {
            renderTumbleWeed(depthMapShader, true);
        }
        else {
            angle = trans = 0;
            objStop = false;
        }
        renderSpecialWeed(depthMapShader, true);
        renderAllObjects(depthMapShader, true);
        renderEagleBody(depthMapShader, true);
        renderEagleFeathers(depthMapShader, true);
        renderEagleWings(depthMapShader, true);
        renderEagleTail(depthMapShader, true);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
        view = myCamera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));


        glViewport(0, 0, glWindowWidth, glWindowHeight);
        myBasicShader.useShaderProgram();

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

        glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity"), fogDensity);
    }
    else {
        myBasicShader.useShaderProgram();
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLightDir"), 1, glm::value_ptr(myCamera.getCameraDirection()));
        glUniform3fv(glGetUniformLocation(myBasicShader.shaderProgram, "spotLightPos"), 1, glm::value_ptr(myCamera.getCameraPosition()));
    }

	// render the teapot
    renderTeapot(myBasicShader, false);

    //render all objects
    renderAllObjects(myBasicShader, false);

    //render tumbleweed
    if (objGen) {
        renderTumbleWeed(myBasicShader, false);
    }
    else {
        angle = trans = 0;
        objStop = false;
    }

    //render eagle
    renderEagleBody(myBasicShader, false);
    renderEagleFeathers(myBasicShader, false);
    renderEagleWings(myBasicShader, false);
    renderEagleTail(myBasicShader, false);
    bodyAngle += 0.6f;
    if (feathersAngle > 10) featherDirection = 1;
    if (feathersAngle < -10) featherDirection = 0;
    if (featherDirection)
        feathersAngle -= 0.45f;
    else
        feathersAngle += 0.45f;
    if (bodyAngle > 360) {
        bodyAngle -= 360;
    }

    //if eagle POV is set, change the camera accordingly
    if (scenePrev) {
        scenePreview();
    }
    renderSpecialWeed(myBasicShader, false);
    renderLamp(myBasicShader, false);
    renderLamp2(myBasicShader, false);
    renderLamp3(myBasicShader, false);

    //render ground
    renderGround(myBasicShader, false);

    //render skybox
    renderSkyBox(skyboxShader);

}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}


void generateBoundingBoxes() {
    addRectangle(glm::vec3(-58.3f, 0, -4.5f), glm::vec3(-27.4f, 0, -4.5f),
        glm::vec3(-27.4f, 0, -10.0f), glm::vec3(-58.3f, 0, -10.0f), 0);
    nr_rectangles++;
    addRectangle(glm::vec3(43.4f, 0, 7.12f), glm::vec3(45.52f, 0, 8.73f),
        glm::vec3(45.9f, 0, 7.4f), glm::vec3(44.5f, 0, 6.3f), 1);
    nr_rectangles++;
    addRectangle(glm::vec3(49.0f, 0, 9.15f), glm::vec3(51.3f, 0, 8.5f),
        glm::vec3(51.0f, 0, 7.4f), glm::vec3(49.0f, 0, 8.15f), 2);
    nr_rectangles++;
    addRectangle(glm::vec3(-20.1f, 0, 21.7f), glm::vec3(-6.9f, 0, 21.6f),
        glm::vec3(-6.8, 0, 18.5f), glm::vec3(-20.5, 0, 18.7f), 3);
    nr_rectangles++;

    addCircle(glm::vec3(-42.7, 0, -7.7f), 6.6f, 0);
    nr_circles++;
    addCircle(glm::vec3(48.24f, 0, 2.4f), 6.35, 1);
    nr_circles++;
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
    initFBO();
	initShaders();
	initUniforms();
    setWindowCallbacks();
    generateBoundingBoxes();


	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
	    renderScene();
        
        //std::cout << myCamera.getCameraPosition().x << " " << myCamera.getCameraPosition().y << " " << myCamera.getCameraPosition().z << "\n";

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
