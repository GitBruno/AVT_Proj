///////////////////////////////////////////////////////////////////////////////
//
// Assignment 1 consists in the following:
//
// - Update your graphics drivers to their latest versions.
// - Download the appropriate libraries GLEW and GLFW for your system.
// - Create a project to compile, link and run the code provided in this 
//   section in your favourite programming environment 
//   (course will use VS2019 Community Edition).
// - Verify what OpenGL contexts your computer can support, a minimum of 
//   OpenGL 3.3 support is required for this course.
//
// Further suggestions to verify your understanding of the concepts explored:
// - Create an abstract class for an OpenGL application.
// - Change the program so display is called at 30 FPS.
//
// (c)2013-20 by Carlos Martinho
//
///////////////////////////////////////////////////////////////////////////////

#define _USE_MATH_DEFINES

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <time.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../HeaderFiles/vector.h"
#include "../HeaderFiles/matrix.h"
#include "../HeaderFiles/mxfactory.h"
#include "../HeaderFiles/object.h"
#include "../HeaderFiles/camera.h"
#include "../HeaderFiles/quaternion.h"

double sprint_factor = 1;
double speed = 10;

////////////////////////////////////////////////// ERROR CALLBACK (OpenGL 4.3+)

static const std::string errorSource(GLenum source)
{
	switch (source) {
	case GL_DEBUG_SOURCE_API:				return "API";
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		return "window system";
	case GL_DEBUG_SOURCE_SHADER_COMPILER:	return "shader compiler";
	case GL_DEBUG_SOURCE_THIRD_PARTY:		return "third party";
	case GL_DEBUG_SOURCE_APPLICATION:		return "application";
	case GL_DEBUG_SOURCE_OTHER:				return "other";
	default:								exit(EXIT_FAILURE);
	}
}

static const std::string errorType(GLenum type)
{
	switch (type) {
	case GL_DEBUG_TYPE_ERROR:				return "error";
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:	return "deprecated behavior";
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:	return "undefined behavior";
	case GL_DEBUG_TYPE_PORTABILITY:			return "portability issue";
	case GL_DEBUG_TYPE_PERFORMANCE:			return "performance issue";
	case GL_DEBUG_TYPE_MARKER:				return "stream annotation";
	case GL_DEBUG_TYPE_PUSH_GROUP:			return "push group";
	case GL_DEBUG_TYPE_POP_GROUP:			return "pop group";
	case GL_DEBUG_TYPE_OTHER_ARB:			return "other";
	default:								exit(EXIT_FAILURE);
	}
}

static const std::string errorSeverity(GLenum severity)
{
	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:			return "high";
	case GL_DEBUG_SEVERITY_MEDIUM:			return "medium";
	case GL_DEBUG_SEVERITY_LOW:				return "low";
	case GL_DEBUG_SEVERITY_NOTIFICATION:	return "notification";
	default:								exit(EXIT_FAILURE);
	}
}

static void error(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
	const GLchar* message, const void* userParam)
{
	std::cerr << "GL ERROR:" << std::endl;
	std::cerr << "  source:     " << errorSource(source) << std::endl;
	std::cerr << "  type:       " << errorType(type) << std::endl;
	std::cerr << "  severity:   " << errorSeverity(severity) << std::endl;
	std::cerr << "  debug call: " << std::endl << message << std::endl << std::endl;
	std::cerr << "Press <return>.";
	std::cin.ignore();
}

void setupErrorCallback()
{
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(error, 0);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);
	// params: source, type, severity, count, ids, enabled
}

///////////////////////////////////////////////// ERROR HANDLING (All versions)

static const std::string errorString(GLenum error)
{
	switch(error) {
	case GL_NO_ERROR:
		return "No error has been recorded.";
	case GL_INVALID_ENUM:
		return "An unacceptable value is specified for an enumerated argument.";
	case GL_INVALID_VALUE:
		return "A numeric argument is out of range.";
	case GL_INVALID_OPERATION:
		return "The specified operation is not allowed in the current state.";
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return "The framebuffer object is not complete.";
	case GL_OUT_OF_MEMORY:
		return "There is not enough memory left to execute the command.";
	case GL_STACK_UNDERFLOW:
		return "An attempt has been made to perform an operation that would cause an internal stack to underflow.";
	case GL_STACK_OVERFLOW:
		return "An attempt has been made to perform an operation that would cause an internal stack to overflow.";
	default: exit(EXIT_FAILURE);
	}
}

static bool isOpenGLError() 
{
	bool isError = false;
	GLenum errCode;
	while ((errCode = glGetError()) != GL_NO_ERROR) {
		isError = true;
		std::cerr << "OpenGL ERROR [" << errorString(errCode) << "]." << std::endl;
	}
	return isError;
}

static void checkOpenGLError(std::string error)
{
	if (isOpenGLError()) {
		std::cerr << error << std::endl;
		exit(EXIT_FAILURE);
	}
}

/////////////////////////////////////////////////////////////////////// SHADERs

struct ShaderSource {
	std::string VertexSource;
	std::string FragmentSource;

};

GLuint VaoId, VboId[2], UboId;
GLuint VertexShaderId, FragmentShaderId, ProgramId;
GLint UniformId;
Camera cam;

static ShaderSource ParseShader(const std::string& filepath) {
	std::ifstream stream(filepath);

	enum class ShaderType {
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	std::string line;
	std::stringstream ss[2];
	ShaderType type = ShaderType::NONE;

	while (getline(stream, line))
	{
		if (line.find("#shader") != std::string::npos) 
		{
			if (line.find("vertex") != std::string::npos)
				type = ShaderType::VERTEX;
			else if (line.find("fragment") != std::string::npos)
				type = ShaderType::FRAGMENT;
		}
		else {
			ss[(int) type] << line << '\n';
		}
	}

	return { ss[0].str(), ss[1].str() };
}

static GLuint CompileShader(GLuint type, const std::string& source) {
	GLuint id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	GLint isCompiled = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(id, maxLength, &maxLength, &errorLog[0]);

		// Provide the infolog in whatever manor you deem best.
		std::cerr << errorLog.data() << std::endl;

		// Exit with failure.
		glDeleteShader(id); // Don't leak the shader.
		return -1;
	}


	return id;
}

void createShaderProgram()
{
	ShaderSource sources = ParseShader("res/shaders/ThreeD.shader");

	std::string VertexShader = sources.VertexSource, 
		FragmentShader = sources.FragmentSource;

	VertexShaderId = CompileShader(GL_VERTEX_SHADER, VertexShader);

	FragmentShaderId = CompileShader(GL_FRAGMENT_SHADER, FragmentShader);

	ProgramId = glCreateProgram();
	glAttachShader(ProgramId, VertexShaderId);
	glAttachShader(ProgramId, FragmentShaderId);

	glBindAttribLocation(ProgramId, POSITIONS, "in_Position");
	glBindAttribLocation(ProgramId, COLORS, "in_Color");

	glLinkProgram(ProgramId);
	UniformId = glGetUniformLocation(ProgramId, "Matrix");
	UboId = glGetUniformBlockIndex(ProgramId, "SharedMatrices");
	glUniformBlockBinding(ProgramId, UboId, UBO_BP);

	glDetachShader(ProgramId, VertexShaderId);
	glDeleteShader(VertexShaderId);
	glDetachShader(ProgramId, FragmentShaderId);
	glDeleteShader(FragmentShaderId);

#ifndef ERROR_CALLBACK
	checkOpenGLError("ERROR: Could not create shaders.");
#endif
}

void destroyShaderProgram()
{
	glUseProgram(0);
	glDeleteProgram(ProgramId);

#ifndef ERROR_CALLBACK
	checkOpenGLError("ERROR: Could not destroy shaders.");
#endif
}

/////////////////////////////////////////////////////////////////////// VAOs & VBOs

std::vector<Object*> scene;

void createBufferObjects()
{
	cam.setupCamera(ProgramId);

	for (Object* obj_ptr : scene) {
		obj_ptr->initObject();
	}
#ifndef ERROR_CALLBACK
	checkOpenGLError("ERROR: Could not create VAOs and VBOs.");
#endif
}

void destroyBufferObjects()
{
	for (Object* obj_ptr : scene) {
		obj_ptr->deleteObject();
	}

#ifndef ERROR_CALLBACK
	checkOpenGLError("ERROR: Could not destroy VAOs and VBOs.");
#endif
}

/////////////////////////////////////////////////////////////////////// SCENE



void walk(GLFWwindow* win, double elapsed) {
	int r = (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS ), 
		l = (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS),
		d = (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS),
		u = (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);

	double xaxis = (double) r - l, 
		yaxis = (double) d - u;

	if (xaxis != 0 || yaxis != 0) {
		Vector3D dir = Vector3D(xaxis, 0, yaxis);
		dir.normalize();

		cam.move(dir, sprint_factor * speed * elapsed);
	}
}

double old_x, old_y;
double angle_x = M_PI_2 / 50, angle_y = M_PI / 50;

void look(GLFWwindow* win, double elapsed) {
	double x, y;
	glfwGetCursorPos(win, &x, &y);

	int w, h;
	glfwGetWindowSize(win, &w, &h);

	double move_x = (x - old_x);
	double move_y = (y - old_y);

	if (move_x != 0 || move_y != 0) 
		cam.look(angle_x * move_x * elapsed, angle_y * move_y * elapsed);

	old_x = x;
	old_y = y;
}

enum class AnimationType { EULER, QUATERNION };
string AnimationTypes[] = { "Euler Animation", "Quaternion Animation" };
AnimationType animation = AnimationType::EULER;
bool animating = false;
double animationTime = 2.0f;
double t = 0.0f;

// Set rotation matrixes to initial position
void resetAnimation() {

	t = 0;
	Matrix4 resetRotation = MxFactory::rotation4(Vector3D(0, 0, 1), 240);

	for (Object* obj_ptr : scene) {
				
		obj_ptr->resetTransform();
	}
	return;
}

// Interpolate towards some euler rotation matrix target
void eulerAnimation() {
	
	std::cout << "Playing Euler Animation" << std::endl;

	Matrix4 currentTransf;
	double percent = t / animationTime;
	Vector3D currentRot = Vector3D(90, 90, 90) * percent;

	if (t > animationTime) return;

	for (Object* obj_ptr : scene) {

		currentTransf = MxFactory::rotation4(Vector3D(0, 0, 1), currentRot.z) * 
						MxFactory::rotation4(Vector3D(1, 0, 0), currentRot.x) * 
						MxFactory::rotation4(Vector3D(0, 1, 0), currentRot.y);

		obj_ptr->setTransform(currentTransf * obj_ptr->initTransformations);
	}
	return;
}

// Interpolate towards some quaternion target
void quaternionAnimation() {
	
	std::cout << "Playing Quaternion Animation" << std::endl;

	for (Object* obj_ptr : scene) {

	}
	return;
}

void animate(GLFWwindow* win, double elapsed) {

	//Toggle Animation Type
	static bool toggle_pressed = false;
	if (!toggle_pressed && glfwGetKey(win, GLFW_KEY_G) == GLFW_PRESS)
	{
		animation = (AnimationType)(((int) animation + 1) % 2);
		animating = false;
		resetAnimation();

		std::cout << "Animation Type: " << AnimationTypes[(int) animation] << std::endl;
		toggle_pressed = true;
	}
	else if (glfwGetKey(win, GLFW_KEY_G) == GLFW_RELEASE) toggle_pressed = false;
	

	// Start Animation
	static bool start_pressed = false;
	if (!start_pressed && glfwGetKey(win, GLFW_KEY_N) == GLFW_PRESS)
	{
		animating = true;
		resetAnimation();

		std::cout << "Start Animation!" << std::endl;
		start_pressed = true;
	}
	else if (glfwGetKey(win, GLFW_KEY_N) == GLFW_RELEASE) start_pressed = false;

	// Actually Animate
	if (animating) {
		t += elapsed;
		switch (animation) {

		case AnimationType::EULER:
			eulerAnimation();
			break;

		case AnimationType::QUATERNION:
			quaternionAnimation();
			break;
		}
	}
}

void processInput(GLFWwindow* win, double elapsed) {
	walk(win, elapsed);
	look(win, elapsed);
	animate(win, elapsed);
}

void drawScene(GLFWwindow* win, double elapsed)
{
	processInput(win, elapsed);

	cam.drawCamera(ProgramId);

	for (Object* obj_ptr : scene) {
		obj_ptr->drawObject(ProgramId);
	}

#ifndef ERROR_CALLBACK
	checkOpenGLError("ERROR: Could not draw scene.");
#endif
}


///////////////////////////////////////////////////////////////////// CALLBACKS

void window_close_callback(GLFWwindow* win)
{
	destroyShaderProgram();
	destroyBufferObjects();
	std::cout << "closing..." << std::endl;
}

void window_size_callback(GLFWwindow* win, int winx, int winy)
{
	std::cout << "size: " << winx << " " << winy << std::endl;
	glViewport(0, 0, winx, winy);
}

void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
	std::cout << "key: " << key << " " << scancode << " " << action << " " << mods << std::endl;
	if (action == GLFW_RELEASE) {
		switch (key) {
		case GLFW_KEY_P:
			if (cam.projType == CameraProj::Parallel) {
				cam.perspectiveProjection(60, 4.0f / 3.0f, 1, 50);
			}
			else {
				cam.parallelProjection(-2,2,-2,2,1,50);
			}
			break;
		case GLFW_KEY_ESCAPE:
			if (cam.state == Working::On) {
				glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
			else {
				glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			cam.toggle();
			break;
		case GLFW_KEY_LEFT_SHIFT:
			sprint_factor = 1;
			break;
		}
	}
	else if (action == GLFW_PRESS) {
		switch (key)
		{
		case GLFW_KEY_LEFT_SHIFT:
			sprint_factor = 3;
			break;
		}
	}
}

void mouse_callback(GLFWwindow* win, double xpos, double ypos)
{
	std::cout << "mouse: " << xpos << " " << ypos << std::endl;
}

void mouse_button_callback(GLFWwindow* win, int button, int action, int mods)
{
	std::cout << "button: " << button << " " << action << " " << mods << std::endl;
}

void scroll_callback(GLFWwindow* win, double xoffset, double yoffset)
{
	std::cout << "scroll: " << xoffset << " " << yoffset << std::endl;
}

void joystick_callback(int jid, int event)
{
	std::cout << "joystick: " << jid << " " << event << std::endl;
}

///////////////////////////////////////////////////////////////////////// SETUP

void glfw_error_callback(int error, const char* description)
{
	std::cerr << "GLFW Error: " << description << std::endl;
}

GLFWwindow* setupWindow(int winx, int winy, const char* title, 
	int is_fullscreen, int is_vsync)
{
	GLFWmonitor* monitor = is_fullscreen ? glfwGetPrimaryMonitor() : 0;
	GLFWwindow* win = glfwCreateWindow(winx, winy, title, monitor, 0);
	if (!win)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(win);
	glfwSwapInterval(is_vsync);
	return win;
}

void setupCallbacks(GLFWwindow* win)
{
	glfwSetKeyCallback(win, key_callback);
	/*
	glfwSetCursorPosCallback(win, mouse_callback);
	glfwSetMouseButtonCallback(win, mouse_button_callback);
	glfwSetScrollCallback(win, scroll_callback);
	glfwSetJoystickCallback(joystick_callback);
	*/
	glfwSetWindowCloseCallback(win, window_close_callback);
	glfwSetWindowSizeCallback(win, window_size_callback);
}

GLFWwindow* setupGLFW(int gl_major, int gl_minor, 
	int winx, int winy, const char* title, int is_fullscreen, int is_vsync)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gl_major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gl_minor);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

	GLFWwindow* win = setupWindow(winx, winy, title, is_fullscreen, is_vsync);
	setupCallbacks(win);

#if _DEBUG
	std::cout << "GLFW " << glfwGetVersionString() << std::endl;
#endif
	return win;
}

void setupGLEW()
{
	glewExperimental = GL_TRUE;
	// Allow extension entry points to be loaded even if the extension isn't 
	// present in the driver's extensions string.
	GLenum result = glewInit();
	if (result != GLEW_OK) 
	{
		std::cerr << "ERROR glewInit: " << glewGetString(result) << std::endl;
		exit(EXIT_FAILURE);
	}
	GLenum err_code = glGetError();
	// You might get GL_INVALID_ENUM when loading GLEW.
}

void checkOpenGLInfo()
{
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* vendor = glGetString(GL_VENDOR);
	const GLubyte* version = glGetString(GL_VERSION);
	const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
	std::cerr << "OpenGL Renderer: " << renderer << " (" << vendor << ")" << std::endl;
	std::cerr << "OpenGL version " << version << std::endl;
	std::cerr << "GLSL version " << glslVersion << std::endl;
}

void setupOpenGL(int winx, int winy)
{
#if _DEBUG
	checkOpenGLInfo();
#endif
	glClearColor(0.1f, 0.1f, 0.3f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glDepthRange(0.0, 1.0);
	glClearDepth(1.0);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glViewport(0, 0, winx, winy);
}

GLFWwindow* setup(int major, int minor, 
	int winx, int winy, const char* title, int is_fullscreen, int is_vsync)
{
	GLFWwindow* win = 
		setupGLFW(major, minor, winx, winy, title, is_fullscreen, is_vsync);
	setupGLEW();
	setupOpenGL(winx, winy);
	setupErrorCallback();
	createShaderProgram();
	createBufferObjects();
	return win;
}

////////////////////////////////////////////////////////////////////////// RUN

void updateFPS(GLFWwindow* win, double elapsed_sec)
{	
	static unsigned int acc_frames = 0;
	static double acc_time = 0.0;
	const double UPDATE_TIME = 1.0;

	++acc_frames;
	acc_time += elapsed_sec;
	if (acc_time > UPDATE_TIME)
	{
		std::ostringstream oss;
		double fps = acc_frames / acc_time;
		oss << std::fixed << std::setw(5) << std::setprecision(1) << fps << " FPS";
		glfwSetWindowTitle(win, oss.str().c_str());

		acc_frames = 0;
		acc_time = 0.0;
	}
}

void display_callback(GLFWwindow* win, double elapsed_sec)
{
	//updateFPS(win, elapsed_sec);
	drawScene(win, elapsed_sec);
}

void run(GLFWwindow* win)
{
	double last_time = glfwGetTime();
	
	while (!glfwWindowShouldClose(win))
	{
		double time = glfwGetTime();
		double elapsed_time = time - last_time;
		last_time = time;


		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		display_callback(win, elapsed_time);
		glfwSwapBuffers(win);
		glfwPollEvents();

		//checkOpenGLError("ERROR: MAIN/RUN");
	}
	glfwDestroyWindow(win);
	glfwTerminate();
}

////////////////////////////////////////////////////////////////////////// MAIN

void populateScene() {
	// Camera init
	cam = Camera(Vector3D(0, 1, 3), Vector3D(0, 0, 0), Vector3D(0, 1, 0));
	cam.parallelProjection(-2,2,-2,2,1,10);

	// Create and initialize objs here
	Object* obj;

	// Front face
	// Part 1
	obj = new Object();

	obj->addVertex(-0.9f, -0.7268f, 0.0f, 0.3f, 0.0f, 0.0f, 1.0f);
	obj->addVertex(-0.8f, -0.9f, 0.0f, 0.3f, 0.0f, 0.0f, 1.0f);
	obj->addVertex(0.0f, 0.8312f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	obj->addVertex(0.0f, 0.4856f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	obj->addVertex(0.8f, -0.5536f, 0.0f, 0.3f, 0.0f, 0.0f, 1.0f);
	obj->addVertex(0.6f, -0.5536f, 0.0f, 0.3f, 0.0f, 0.0f, 1.0f);

	obj->addTriangle(0, 1, 2);
	obj->addTriangle(3, 2, 1);
	obj->addTriangle(2, 3, 5);
	obj->addTriangle(5, 4, 2);

	scene.push_back(obj);

	// Part 2
	obj = new Object();

	obj->addVertex(-0.9f, -0.7268f, 0.0f, 0.0f, 0.3f, 0.0f, 1.0f);
	obj->addVertex(-0.8f, -0.9f, 0.0f, 0.0f, 0.3f, 0.0f, 1.0f);
	obj->addVertex(0.0f, 0.8312f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	obj->addVertex(0.0f, 0.4856f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	obj->addVertex(0.8f, -0.5536f, 0.0f, 0.0f, 0.3f, 0.0f, 1.0f);
	obj->addVertex(0.6f, -0.5536f, 0.0f, 0.0f, 0.3f, 0.0f, 1.0f);

	obj->addTriangle(0, 1, 2);
	obj->addTriangle(3, 2, 1);
	obj->addTriangle(2, 3, 5);
	obj->addTriangle(5, 4, 2);

	obj->rotateAroundAxis(Vector3D(0.0f, 0.0f, 1.0f), 240);
	obj->translate(Vector3D(0.3795f, -0.3108f, 0));
	obj->saveInitTransform();

	scene.push_back(obj);

	// Part 3
	obj = new Object();

	obj->addVertex(-0.9f, -0.7268f, 0.0f, 0.0f, 0.0f, 0.3f, 1.0f);
	obj->addVertex(-0.8f, -0.9f, 0.0f, 0.0f, 0.0f, 0.3f, 1.0f);
	obj->addVertex(0.0f, 0.8312f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	obj->addVertex(0.0f, 0.4856f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	obj->addVertex(0.8f, -0.5536f, 0.0f, 0.0f, 0.0f, 0.3f, 1.0f);
	obj->addVertex(0.6f, -0.5536f, 0.0f, 0.0f, 0.0f, 0.3f, 1.0f);

	obj->addTriangle(0, 1, 2);
	obj->addTriangle(3, 2, 1);
	obj->addTriangle(2, 3, 5);
	obj->addTriangle(5, 4, 2);

	obj->rotateAroundAxis(Vector3D(0.0f, 0.0f, 1.0f), 120);
	obj->translate(Vector3D(-0.08016f, -0.4844f, 0.0f));
	obj->saveInitTransform();

	scene.push_back(obj);

	// Back face
	// Part 1
	obj = new Object();

	obj->addVertex(-0.9f, -0.7268f, 0.0f, 0.1f, 0.0f, 0.0f, 1.0f);
	obj->addVertex(-0.8f, -0.9f, 0.0f, 0.1f, 0.0f, 0.0f, 1.0f);
	obj->addVertex(0.0f, 0.8312f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f);
	obj->addVertex(0.0f, 0.4856f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f);
	obj->addVertex(0.8f, -0.5536f, 0.0f, 0.1f, 0.0f, 0.0f, 1.0f);
	obj->addVertex(0.6f, -0.5536f, 0.0f, 0.1f, 0.0f, 0.0f, 1.0f);

	obj->addTriangle(0, 2, 1);
	obj->addTriangle(3, 1, 2);
	obj->addTriangle(2, 5, 3);
	obj->addTriangle(5, 2, 4);

	scene.push_back(obj);

	// Part 2
	obj = new Object();

	obj->addVertex(-0.9f, -0.7268f, 0.0f, 0.0f, 0.1f, 0.0f, 1.0f);
	obj->addVertex(-0.8f, -0.9f, 0.0f, 0.0f, 0.1f, 0.0f, 1.0f);
	obj->addVertex(0.0f, 0.8312f, 0.0f, 0.0f, 0.5f, 0.0f, 1.0f);
	obj->addVertex(0.0f, 0.4856f, 0.0f, 0.0f, 0.5f, 0.0f, 1.0f);
	obj->addVertex(0.8f, -0.5536f, 0.0f, 0.0f, 0.1f, 0.0f, 1.0f);
	obj->addVertex(0.6f, -0.5536f, 0.0f, 0.0f, 0.1f, 0.0f, 1.0f);

	obj->addTriangle(0, 2, 1);
	obj->addTriangle(3, 1, 2);
	obj->addTriangle(2, 5, 3);
	obj->addTriangle(5, 2, 4);

	obj->rotateAroundAxis(Vector3D(0.0f, 0.0f, 1.0f), 240);
	obj->translate(Vector3D(0.3795f, -0.3108f, 0));
	obj->saveInitTransform();

	scene.push_back(obj);

	// Part 3
	obj = new Object();

	obj->addVertex(-0.9f, -0.7268f, 0.0f, 0.0f, 0.0f, 0.1f, 1.0f);
	obj->addVertex(-0.8f, -0.9f, 0.0f, 0.0f, 0.0f, 0.1f, 1.0f);
	obj->addVertex(0.0f, 0.8312f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f);
	obj->addVertex(0.0f, 0.4856f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f);
	obj->addVertex(0.8f, -0.5536f, 0.0f, 0.0f, 0.0f, 0.1f, 1.0f);
	obj->addVertex(0.6f, -0.5536f, 0.0f, 0.0f, 0.0f, 0.1f, 1.0f);

	obj->addTriangle(0, 2, 1);
	obj->addTriangle(3, 1, 2);
	obj->addTriangle(2, 5, 3);
	obj->addTriangle(5, 2, 4);

	obj->rotateAroundAxis(Vector3D(0.0f, 0.0f, 1.0f), 120);
	obj->translate(Vector3D(-0.08016f, -0.4844f, 0.0f));
	obj->saveInitTransform();

	scene.push_back(obj);

}


void test1() {
	std::cout << "\nTest 1: quaternion rot" << std::endl;

	Vector4D axis(0, 1, 0, 1);
	Quaternion q(90.0f, axis);
	std::cout << "q: " << q << std::endl;

	Quaternion qi(0.0f, 7.0f, 0.0f, 0.0f);
	std::cout << "qi: " << qi << std::endl;

	Quaternion qe(0.0f, 0.0f, 0.0f, -7.0f);
	std::cout << "qe: " << qe << std::endl;

	Quaternion qinv = q.inverse();
	std::cout << "qinv: " << qinv << std::endl;

	Quaternion qf1 = (q * qi) * qinv;
	std::cout << "qf1: " << qf1 << std::endl;

	std::cout << "qf1 == qe? " << (qf1 == qe) << std::endl;

	Quaternion qconj = q.conjugate();
	std::cout << "qconj: " << qconj << std::endl;

	Quaternion qf2 = (q * qi) * qconj;
	std::cout << "qf2: " << qf2 << std::endl;

	std::cout << "qf2 == qe? " << (qf1 == qe) << std::endl;
}

void test2() {
	std::cout << "\nTest 2: matrix" << std::endl;

	Vector4D axis(0, 1, 0, 1);
	Quaternion q(90.0f, axis);
	std::cout << "q: " << q << std::endl;

	Vector4D vi(7.0f, 0.0f, 0.0f, 1.0f);
	std::cout << "vi: " << vi << std::endl;

	Vector4D ve(0.0f, 0.0f, -7.0f, 1.0f);
	std::cout << "ve: " << ve << std::endl;

	Matrix4 m = q.rotMat();
	std::cout << "M: \n" << m << std::endl;

	Vector4D vf = m * vi;
	std::cout << "vf: " << vf << std::endl;

	std::cout << "vf == ve? " << (vf == ve) << std::endl;
}

void test3() {
	std::cout << "\nTest 3: yaw 900 = roll 180 + pitch 180" << std::endl;

	Vector4D axis_x(1.0f, 0.0f, 0.0f, 1.0f);
	Vector4D axis_y(0.0f, 1.0f, 0.0f, 1.0f);
	Vector4D axis_z(0.0f, 0.0f, 1.0f, 1.0f);

	Quaternion qyaw900(900.0f, axis_y);
	std::cout << "qyaw900: " << qyaw900 << std::endl;
	Quaternion qroll180(180.0f, axis_x);
	std::cout << "qroll180: " << qroll180 << std::endl;
	Quaternion qpitch180(180.0f, axis_z);
	std::cout << "qpitch180: " << qpitch180 << std::endl;

	Quaternion qrp = qpitch180 * qroll180;
	std::cout << "qrp: " << qrp << std::endl;
	Quaternion qpr = qroll180 * qpitch180;
	std::cout << "qpr: " << qpr << std::endl;

	Quaternion qi(0.0f,1.0f,0.0f,0.0f);
	std::cout << "qi: " << qi << std::endl;
	Quaternion qe(0.0f, -1.0f, 0.0f, 0.0f);
	std::cout << "qe: " << qe << std::endl;

	Quaternion qfy = (qyaw900 * qi) * qyaw900.inverse();
	std::cout << "qfy: " << qfy << std::endl;
	std::cout << "qfy == qe? " << (qfy == qe) << std::endl;

	Quaternion qfrp = (qrp * qi) * qrp.inverse();
	std::cout << "qfrp: " << qfrp << std::endl;
	std::cout << "qfrp == qe? " << (qfrp == qe) << std::endl;

	Quaternion qfpr = (qpr * qi) * qpr.inverse();
	std::cout << "qfpr: " << qfpr << std::endl;
	std::cout << "qfpr == qe? " << (qfpr == qe) << std::endl;
}

void test4() {
	std::cout << "\nTest 4: Q <-> (angle, axis)" << std::endl;

	double theta1 = 45.0f;
	Vector4D axis_i(0, 1, 0, 1);
	Quaternion q(theta1, axis_i);
	std::cout << "theta1: " << theta1 << std::endl;
	std::cout << "axis_i: " << axis_i << std::endl;

	double thetaf;
	Vector4D axis_f = q.toAngleAxis(thetaf);
	std::cout << "thetaf: " << thetaf << std::endl;
	std::cout << "axis_f: " << axis_f << std::endl;

	std::cout << "the angles are equal? " << (abs(thetaf - theta1) < EQERR) << std::endl;
	std::cout << "axis_i == axis_f? " << (axis_i == axis_f) << std::endl;

}

void test5() {
	std::cout << "\nTest 5: LERP" << std::endl;

	Vector4D axis(0, 1, 0, 1);
	Quaternion q0(0.0f, axis);
	std::cout << "q0: " << q0 << std::endl;
	Quaternion q1(90.0f, axis);
	std::cout << "q1: " << q1 << std::endl;
	Quaternion qe(30.0f, axis);
	std::cout << "qe: " << qe << std::endl;

	Quaternion qLerp0 = q0.Lerp(q1, 0.0f);
	std::cout << "lerp(0): " << qLerp0 << std::endl;
	std::cout << "q0 == qLerp0? " << (q0 == qLerp0) << std::endl;

	Quaternion qLerp1 = q0.Lerp(q1, 1.0f);
	std::cout << "lerp(1): " << qLerp1 << std::endl;
	std::cout << "q1 == qLerp1? " << (q1 == qLerp1) << std::endl;

	Quaternion qLerp = q0.Lerp(q1, 1/3.0f);
	std::cout << "lerp(1/3): " << qLerp << std::endl;
	std::cout << "qe == qLerp? " << (qe == qLerp) << std::endl;
}

void test6() {
	std::cout << "\nTest 5: SLERP" << std::endl;

	Vector4D axis(0, 1, 0, 1);
	Quaternion q0(0.0f, axis);
	std::cout << "q0: " << q0 << std::endl;
	Quaternion q1(90.0f, axis);
	std::cout << "q1: " << q1 << std::endl;
	Quaternion qe(30.0f, axis);
	std::cout << "qe: " << qe << std::endl;

	Quaternion qSlerp0 = q0.Slerp(q1, 0.0f);
	std::cout << "slerp(0): " << qSlerp0 << std::endl;
	std::cout << "q0 == qSlerp0? " << (q0 == qSlerp0) << std::endl;

	Quaternion qSlerp1 = q0.Slerp(q1, 1.0f);
	std::cout << "slerp(1): " << qSlerp1 << std::endl;
	std::cout << "q1 == qSlerp1? " << (q1 == qSlerp1) << std::endl;

	Quaternion qSlerp = q0.Slerp(q1, 1 / 3.0f);
	std::cout << "slerp(1/3): " << qSlerp << std::endl;
	std::cout << "qe == qSlerp? " << (qe == qSlerp) << std::endl;
}


int main(int argc, char* argv[])
{
	
	populateScene();

	Vector4D ref(0.8f, -0.5536f, 0.0f, 1);
	Vector4D v1(0.0f, 0.4856f, 0.0f, 1), v2;

	v2 = MxFactory::rotation4(Vector3D(0,0,1), 240) * v1;

	v2 = ref - v2;

	cout << v2 << endl;

	int gl_major = 4, gl_minor = 3;
	int is_fullscreen = 0;
	int is_vsync = 1;
	GLFWwindow* win = setup(gl_major, gl_minor,
		640, 480, "Hello Modern 2D World", is_fullscreen, is_vsync);

	run(win);
	

	std::cout << boolalpha << setprecision(4);

	test1();
	test2();
	test3();
	test4();
	test5();
	test6();

	exit(EXIT_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////// END