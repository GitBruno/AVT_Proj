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
#include <iomanip>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../HeaderFiles/vector.h"
#include "../HeaderFiles/matrix.h"
#include "../HeaderFiles/mxfactory.h"

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
}

void setupErrorCallback()
{
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(error, 0);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
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

///////////////////////////////////////////////////////////////////// CALLBACKS

void window_close_callback(GLFWwindow* win)
{
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
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(win, GLFW_TRUE);
		window_close_callback(win);
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
	glfwSetCursorPosCallback(win, mouse_callback);
	glfwSetMouseButtonCallback(win, mouse_button_callback);
	glfwSetScrollCallback(win, scroll_callback);
	glfwSetJoystickCallback(joystick_callback);

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
	updateFPS(win, elapsed_sec);
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

// ====================== Rodrigues Rotation ===============================

const Vector3D RodrRot(Vector3D& v, Vector3D& k, float angle) {
	float r_angle = angle * M_PI / 180; // convert the angle in degrees to radians
	k.normalize();

	Vector3D v1 = v * cos(r_angle);
	Vector3D v2 = (k % v) * sin(r_angle);
	Vector3D v3 = k * (k * v) * (1 - cos(r_angle));

	return (v1 + v2 + v3);
}

// =========================================================================

////////////////////////////////////////////////////////////////////////// MAIN

int main(int argc, char* argv[])
{
	/*
	std::cout << std::boolalpha;
	MxFactory m;

	Matrix2 m1 = m.identity2();

	std::cout << m1 << std::endl;
	*/

	/*
	Matrix2 mat;

	std::cout << mat << std::endl;

	mat = Matrix2(1.234567, 0, 0, 0);
	std::cout << mat << std::endl;

	std::istringstream iss("1 3 2 4");
	iss >> mat;

	std::cout << mat << std::endl;

	Matrix2 m1 = mat * mat;

	std::cout << m1 << std::endl;

	mat += mat;

	std::cout << mat << std::endl;

	m1 = mat - mat;

	std::cout << m1 << std::endl;

	mat /= 2;

	m1 = mat.inverse();

	std::cout << m1 << std::endl;
	*/

	/*
	Matrix3 mat;

	std::cout << mat << std::endl;

	Matrix3 m3(1, 4, 7, 2, 5, 8, 3, 6, 9);

	std::cout << m3 << std::endl;

	std::cout << "Row 1, Col 2: " << m3(1,2) << std::endl;
	std::cout << "Row 2, Col 1: " << m3(2, 1) << std::endl;

	Matrix3 mult = m3 * m3;

	std::cout << mult << std::endl;

	m3 = Matrix3(1,0,5,2,1,6,3,4,0);

	Matrix3 inv = m3.inverse();

	std::cout << m3 << std::endl;
	std::cout << inv << std::endl;

	m3 = Matrix3(3, 2, 0, 0, 0, 1, 2, -2, 1);

	inv = m3.inverse();

	std::cout << m3 << std::endl;
	std::cout << inv << std::endl;
	*/

	Matrix4 mat;

	std::cout << mat << std::endl;

	mat = Matrix4(1, 0, 0, 0, 1, 2, 0, 0, 1, 2, 3, 0, 1, 2, 3, 4);
	std::cout << mat << std::endl;

	std::istringstream iss("1 0 0 0 1 2 0 0 1 2 3 0 1 2 3 4");
	iss >> mat;

	std::cout << mat << std::endl;

	float a22 = mat(2, 2);
	std::cout << "a22 -> " << a22 << "\n" <<  std::endl;

	Matrix4 m1 = mat * mat;

	std::cout << m1 << std::endl;

	mat += mat;

	std::cout << mat << std::endl;

	m1 = mat - mat;

	std::cout << m1 << std::endl;

	mat /= 2;

	m1 = mat.transpose();

	std::cout << m1 << std::endl;
	

	exit(EXIT_SUCCESS); 
}

/////////////////////////////////////////////////////////////////////////// END