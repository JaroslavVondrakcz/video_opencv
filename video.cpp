// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgproc.hpp>
#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
//#include <opencv2\opencv.hpp>

// OpenCV 
#include <opencv2\opencv.hpp>

// OpenGL Extension Wrangler
#include <GL/glew.h> 
#include <GL/wglew.h> //WGLEW = Windows GL Extension Wrangler (change for different platform) 

// GLFW toolkit
#include <GLFW/glfw3.h>

// OpenGL math
#include <glm/glm.hpp>

// OpenGL textures
#include <gli/gli.hpp>


struct s_globals {
	cv::VideoCapture capture;
	GLFWwindow* window;
	int height;
	int width;
	double app_start_time;
};


s_globals globals;
int frames_processed;
bool running = false;
cv::Point center;

//=====================================================================================================
static void error_callback(int error, const char* description);
static void finalize(int value);
static void init_glfw();
static void init_glew();
static void gl_print_info();
static void init_video();
cv::Point process_frame_gl(cv::Mat& frame);
void process_video_gl(cv::VideoCapture& capture);
cv::Point find_center(cv::Mat scene_threshold, cv::Mat scene_hsv);
void draw_cross(cv::Mat& img, int x, int y, int size);
//=====================================================================================================

static void init_opengl(void)
{

	init_glfw();
	init_glew();
	gl_print_info();

}

int main(int argc, char * argv[])
{
	std::thread thread;

	bool change = false;
	int old_frame = 0;

	////////// NEW CODE ///////
	init_opengl();
	init_video();

	auto start = std::chrono::system_clock::now();
	//frames_processed = process_video_gl(globals.capture);
	std::cout << "starting thread" << std::endl;
	thread = std::thread(process_video_gl, std::ref(globals.capture));
	running = true;

	while (running) {
		if (old_frame < frames_processed) {
			old_frame = frames_processed;
			std::cout << center;
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}


	auto end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsed_seconds = end - start;
	std::cout << "elapsed time: " << elapsed_seconds.count() << "sec" << std::endl;
	std::cout << "fps: " << frames_processed / elapsed_seconds.count() << std::endl;

	thread.join();
}

cv::Point process_frame_gl(cv::Mat& frame)
{
	double h_low = 128.0;
	double s_low = 128.0;
	double v_low = 128.0;
	double h_hi = 255.0;
	double s_hi = 255.0;
	double v_hi = 255.0;
	cv::Point result = { 100,1000 };

	// analyze the image

	//cv::resize(frame, frame, cv::Size(690, 540));

	cv::imshow("grabbed", frame);

	cv::Mat scene_hsv, scene_threshold, imgThreshold;

	cv::cvtColor(frame, scene_hsv, cv::COLOR_BGR2HSV);
	cv::inRange(scene_hsv, cv::Scalar(100, 50, 50, 0), cv::Scalar(130, 255, 255, 0), imgThreshold);

	result = find_center(imgThreshold, scene_hsv);
	draw_cross(imgThreshold, result.x, result.y, 8);

	cv::imshow("threshold", imgThreshold);


	//std::this_thread::sleep_for(std::chrono::milliseconds(20));

	//cv::resize(frame, frame, cv::Size(1920, 1080));

	// DO NOT DISPLAY!
	// DO NOT POLL EVENTS!
	////show image
	//cv::imshow("grabbed", frame);
	//cv::imshow("threshold", scene_threshold);
	////process events
	//cv::waitKey(1);

	return result;
}

void process_video_gl(cv::VideoCapture& capture)
{
	cv::Mat frame;
	int frame_count = 0;

	// Run until exit is requested.
	while (!glfwWindowShouldClose(globals.window))
	{
		if (globals.capture.read(frame))
		{

			center = process_frame_gl(frame);

			frames_processed++;
		}
		else
			glfwSetWindowShouldClose(globals.window, true);

		// Clear color buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Use ModelView matrix for following trasformations (translate,rotate,scale)
		glMatrixMode(GL_MODELVIEW);
		// Clear all tranformations
		glLoadIdentity();

		// Draw something
		glRasterPos2i(0, 0);
		glDrawPixels(frame.cols, frame.rows, GL_BGR, GL_UNSIGNED_BYTE, frame.data);

		// Swap front and back buffers 
		// Calls glFlush() inside
		glfwSwapBuffers(globals.window);

		// Poll for and process events
		glfwPollEvents();
	}
	running = false;

}


void draw_cross(cv::Mat& img, int x, int y, int size)
{
	cv::Point p1 = cv::Point(x - size / 2, y);
	cv::Point p2 = cv::Point(x + size / 2, y);
	cv::Point p3 = cv::Point(x, y - size / 2);
	cv::Point p4 = cv::Point(x, y + size / 2);

	cv::line(img, p1, p2, CV_RGB(255, 0, 0), 3, 8, 0);
	cv::line(img, p3, p4, CV_RGB(255, 0, 0), 3, 8, 0);
}

cv::Point find_center(cv::Mat scene_threshold, cv::Mat scene_hsv) {
	std::vector<cv::Mat> components;
	cv::split(scene_hsv, components);

	int sumX = 0, sumY = 0, total = 0;

	for (int y = 0; y < scene_threshold.rows; y++)
	{
		for (int x = 0; x < scene_threshold.cols; x++)
		{

			if (components[0].at<uchar>(y, x) < 15) {
				sumX += x;
				sumY += y;
				total++;
			}
		}
	}
	return { sumX / total, sumY / total };
}

static void init_video() {
	globals.capture = cv::VideoCapture("video3.mkv");

	if (!globals.capture.isOpened())
	{
		std::cerr << "no camera" << std::endl;
		exit(EXIT_FAILURE);
	}
	else
	{
		std::cout << "Camera " <<
			": width=" << globals.capture.get(cv::CAP_PROP_FRAME_WIDTH) <<
			", height=" << globals.capture.get(cv::CAP_PROP_FRAME_HEIGHT) <<
			std::endl;
	}
}

static void init_glfw()
{
	//
// GLFW init.
//

	// set error callback first
	glfwSetErrorCallback(error_callback);

	//initialize GLFW library
	int glfw_ret = glfwInit();
	if (!glfw_ret)
	{
		std::cerr << "GLFW init failed." << std::endl;
		finalize(EXIT_FAILURE);
	}

	// Shader based, modern OpenGL (3.3 and higher)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // only new functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); // only old functions (for old tutorials etc.)

	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	globals.window = glfwCreateWindow(800, 600, "OpenGL context", NULL, NULL);
	if (!globals.window)
	{
		std::cerr << "GLFW window creation error." << std::endl;
		finalize(EXIT_FAILURE);
	}

	// Get some GLFW info.
	{
		int major, minor, revision;

		glfwGetVersion(&major, &minor, &revision);
		std::cout << "Running GLFW " << major << '.' << minor << '.' << revision << std::endl;
		std::cout << "Compiled against GLFW " << GLFW_VERSION_MAJOR << '.' << GLFW_VERSION_MINOR << '.' << GLFW_VERSION_REVISION << std::endl;
	}

	glfwMakeContextCurrent(globals.window);										// Set current window.
	glfwGetFramebufferSize(globals.window, &globals.width, &globals.height);	// Get window size.
	//glfwSwapInterval(0);														// Set V-Sync OFF.
	glfwSwapInterval(1);														// Set V-Sync ON.


	globals.app_start_time = glfwGetTime();										// Get start time.
}

static void init_glew()
{
	//
// Initialize all valid GL extensions with GLEW.
// Usable AFTER creating GL context!
//
	{
		GLenum glew_ret;
		glew_ret = glewInit();
		if (glew_ret != GLEW_OK)
		{
			std::cerr << "WGLEW failed with error: " << glewGetErrorString(glew_ret) << std::endl;
			finalize(EXIT_FAILURE);
		}
		else
		{
			std::cout << "GLEW successfully initialized to version: " << glewGetString(GLEW_VERSION) << std::endl;
		}

		// Platform specific. (Change to GLXEW or ELGEW if necessary.)
		glew_ret = wglewInit();
		if (glew_ret != GLEW_OK)
		{
			std::cerr << "WGLEW failed with error: " << glewGetErrorString(glew_ret) << std::endl;
			finalize(EXIT_FAILURE);
		}
		else
		{
			std::cout << "WGLEW successfully initialized platform specific functions." << std::endl;
		}
	}
}

static void gl_print_info()
{
	// Get OpenGL driver info
	{
		const char* vendor_s = (const char*)glGetString(GL_VENDOR);
		const char* renderer_s = (const char*)glGetString(GL_RENDERER);
		const char* version_s = (const char*)glGetString(GL_VERSION);
		const char* glsl_s = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

		std::cout << "OpenGL driver vendor: " << (vendor_s == nullptr ? "<UNKNOWN>" : vendor_s) << ", renderer: " << (renderer_s == nullptr ? "<UNKNOWN>" : renderer_s) << ", version: " << (version_s == nullptr ? "<UNKNOWN>" : version_s) << std::endl;

		GLint profile_flags;
		glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile_flags);
		std::cout << "Current profile: ";
		if (profile_flags & GL_CONTEXT_CORE_PROFILE_BIT)
			std::cout << "CORE";
		else
			std::cout << "COMPATIBILITY";
		std::cout << std::endl;

		GLint context_flags;
		glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
		std::cout << "Active context flags: ";
		if (context_flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
			std::cout << "GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT ";
		if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT)
			std::cout << "GL_CONTEXT_FLAG_DEBUG_BIT ";
		if (context_flags & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT)
			std::cout << "GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT ";
		if (context_flags & GL_CONTEXT_FLAG_NO_ERROR_BIT)
			std::cout << "GL_CONTEXT_FLAG_NO_ERROR_BIT";
		std::cout << std::endl;

		std::cout << "Primary GLSL shading language version: " << (glsl_s == nullptr ? "<UNKNOWN>" : glsl_s) << std::endl;
	}

	//
	// GLM & GLI library
	//

	std::cout << "GLM version: " << GLM_VERSION_MAJOR << '.' << GLM_VERSION_MINOR << '.' << GLM_VERSION_PATCH << "rev" << GLM_VERSION_REVISION << std::endl;
	std::cout << "GLI version: " << GLI_VERSION_MAJOR << '.' << GLI_VERSION_MINOR << '.' << GLI_VERSION_PATCH << "rev" << GLI_VERSION_REVISION << std::endl;
}

static void finalize(int code)
{
	// ...

	// Close OpenGL window if opened and terminate GLFW  
	if (globals.window)
		glfwDestroyWindow(globals.window);
	glfwTerminate();

	// ...
}


//=====================================================================================================
static void error_callback(int error, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}
//=====================================================================================================
