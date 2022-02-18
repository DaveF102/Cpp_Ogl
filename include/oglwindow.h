#ifndef OGLWINDOW_H
#define OGLWINDOW_H

#include <memory>
#include <map>
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shape.h"
#include "camera.h"
#include "shader.h"

// This class combines the shape from an .stl file as well as material properties and
//   the location/orientation of the object. This class also contains indices for the
//   vertex array and the vertex buffer which are sent to the GPU by the OGLWindow class.
class OGLObject
{
public:
	// Constructor
    OGLObject() :
        vaoIdx { 0 },
        vboIdx { 0 },
        vertexCount { 0 },
        indexCount { 0 },
        pos { glm::vec3(0.0f, 0.0f, 0.0f) },
        rotang { glm::vec3(0.0f, 0.0f, 0.0f) },
        scale { glm::vec3(1.0f, 1.0f, 1.0f) },
        matlIdx { 0 },
        opt { true, false }
        {}

    GLuint vaoIdx;
	GLuint vboIdx;
    long vertexCount;
    long indexCount;
    glm::vec3 pos;
    glm::vec3 rotang;
    glm::vec3 scale;
    unsigned int matlIdx;
    bool opt[2];
};

// This class contains color and transparency properties of an object.  The index (matlIdx)
//   of a vector of Material class objects is a variable in the OGLObject class
class Material
{
public:
	// Constructor
    Material() :
        name { "" },
        diffuse { glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) },
        specular { glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) },
        alpha { 1.0f },
        shininess { 32.0f }
        {}

	std::string name;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float alpha;
	float shininess;
};

// This struct is used in displaying characters in the OpenGL window
struct Character
{
	unsigned int TextureID;
	glm::ivec2 Size;
	glm::ivec2 Bearing;
	unsigned int Advance;
};

// This class controls the overall creation and manipulation of the OpenGL window, including
//   aspects such as windows size, lighting, view characteristics.  It also sends the object
//   vertices to the GPU and controls double buffered OpenGL rendering
class OGLWindow
{
public:
	// Constructor
    OGLWindow() :
        _scrX { 1024U },
        _scrY { 768U },
        _scrTop { 100U },
        _scrLeft { 100U },
        _viewX { -60.0f },
        _viewY { 60.0f },
        _pan { glm::vec3(0.0f, 0.0f, 0.0f) },
	    _zoom { 1.0f },
	    _msIdX { 2 },
	    _msIdY { 1 },
        _msx { 0 },
        _msy { 0 },
        _leftButtonDown { false },
        _leftDownPosX { 0 },
        _leftDownPosY { 0 },
        _keyDown { 0 },
        _GLBackground { glm::vec4(0.1f, 0.25f, 0.4f, 1.0f) },
        _LightAmbient { glm::vec4(0.7f, 0.7f, 0.7f, 1.0f) },
        _LightDiffuse { glm::vec4(0.6f, 0.6f, 0.6f, 1.0f) },
        _LightDirection { glm::vec4(-0.5f, -0.5f, 0.2f, 1.0f) },
        _LightSpecular { glm::vec4(0.4f, 0.4f, 0.4f, 1.0f) },
        _deltaTime { 0.1f },
        _lastFrame { 0.001f },
        _txtVAO { 0 },
        _txtVBO { 0 }
        {}

    // Other functions
    void InitOGLWindowData();
    void InitOGLWindow(std::unique_ptr<std::string> ufPath);
	void SendVerticesToGPU(std::unique_ptr<std::string> ufPath);
    void ProcessView(int thcount);
    void SetProjectionAndView(Shader &shd, bool &withpan, bool &withZoom, glm::vec3 &ProjTran, glm::vec3 &ViewTran);
    void RenderScene();
    void InitShaders();
	void InitVertices(OGLObject &obj, Shape &shp);
    void InitFreeType();
    void RenderText(std::string txt, float x, float y, float scale, glm::vec3 color);
    void RT_TextAndFloat(std::string txt, float val, int prec, int row);
	void ApplyMatlToShader(unsigned int matID, Shader &shd);
    void KeysUp();
    void ToggleMesh();
    void ReturnToOrigin();

private:
    // Private variables
	Camera _cam;
	unsigned int _scrX;
	unsigned int _scrY;
	unsigned int _scrTop;
	unsigned int _scrLeft;
	float _viewX;
	float _viewY;
	glm::vec3 _pan;
	float _zoom;
	int _msIdX;
	int _msIdY;
    int _msx;
    int _msy;
    bool _leftButtonDown;
    int _leftDownPosX;
    int _leftDownPosY;
	int _keyDown;
	glm::vec4 _GLBackground;
	glm::vec4 _LightAmbient;
	glm::vec4 _LightDiffuse;
	glm::vec4 _LightDirection;
	glm::vec4 _LightSpecular;
	float _deltaTime;
	float _lastFrame;
	GLuint _txtVAO;
	GLuint _txtVBO;
    vector<vector<float>> _msMult;
	vector<OGLObject> _ob;
	vector<Material> _matl;
    vector<Shader> _shd;
    GLFWwindow* _window;
	std::map<GLchar, Character> _Characters;

    // Issue:  2.16 - How do I use C++ methods as callbacks? (https://www.glfw.org/faq.html)
    //   You cannot use regular methods as callbacks, as GLFW is a C library and doesn’t know about objects and
    //   this pointers. If you wish to receive callbacks to a C++ object, use static methods or regular functions
    //   as callbacks, store the pointer to the object you wish to call as the user pointer for the window and 
    //   use it to call methods on your object.
    // Idea for callback wrapper in nested class from (Answer #4) in:
    //   https://stackoverflow.com/questions/7676971/pointing-to-a-function-that-is-a-class-member-glfw-setkeycallback

	void ScreenSizeCallback(GLFWwindow* window, int width, int height);
    void MousePositionCallback(GLFWwindow* window, int positionX, int positionY);
	void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void SetCallbackFunctions();

    class GLFWCallbackWrapper
    {
    public:
        GLFWCallbackWrapper() = delete;
        GLFWCallbackWrapper(const GLFWCallbackWrapper&) = delete;
        GLFWCallbackWrapper(GLFWCallbackWrapper&&) = delete;
        ~GLFWCallbackWrapper() = delete;

        static void ScreenSizeCallback(GLFWwindow* window, int width, int height);
        static void MousePositionCallback(GLFWwindow* window, double xpos, double ypos);
        static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void SetApplication(OGLWindow *view);
    private:
        static OGLWindow* s_oglwindow;
    };
};

#endif
