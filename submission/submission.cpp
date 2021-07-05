// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>

#include <vector>
#include <memory>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <chrono>

#include <opencv2/opencv.hpp>


std::ostream& operator << (std::ostream& os, const glm::uvec4 & v){
	os << v.x << " " << v.y << " " << v.z << " " << v.w;
	return os;
}

std::ostream& operator << (std::ostream& os, const glm::vec3 & v){
	os << "{ " << v.x << " " << v.y << " " << v.z << " }";
	return os;
}

std::ostream& operator << (std::ostream& os, const glm::mat4 & v){
	for (int i = 0; i < 4; ++i){
		for (int j = 0; j < 4; ++j){
			os << v[i][j] << " ";
		}
		os << std::endl;
	}
	os << std::endl;
	return os;
}

class Grid
{
	GLuint vao;
	GLuint vbo;
	GLuint ibo;
	GLuint colorbuffer;
	int slices = 10;
	int lenght;

	GLuint programID;
	GLuint MatrixID;

	std::vector<glm::vec3> vertices;
	std::vector<glm::uvec4> indices;

	glm::vec3 color = {0, 1, 0};

public:

	Grid()
	{
		programID = LoadShaders(
				"TransformVertexShader.vertexshader",
				"ColorFragmentShader.fragmentshader"
		);

		// Get a handle for our "MVP" uniform
		MatrixID = glGetUniformLocation(programID, "MVP");

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		for(int j = 0; j <= slices; ++j) {
			for(int i = 0; i <= slices; ++i) {
				float x = (float) i / (float) slices;
				float y = 0;
				float z = (float) j / (float) slices;
				vertices.push_back(glm::vec3(x, y, z));
			}
		}

		// Add the line segments
		for(int j = 0; j < slices; ++j) {
			for(int i = 0; i < slices; ++i) {

				int row1 =  j * (slices + 1);
				int row2 = (j + 1) * (slices + 1);

				indices.push_back(glm::uvec4(
						row1 + i,
						row1 + i + 1,
						row1 + i + 1,
						row2 + i + 1
						));

				indices.push_back(glm::uvec4(
						row2 + i + 1,
						row2 + i,
						row2 + i,
						row1 + i
						));
			}
		}

		std::vector<glm::vec3> colors;

		for (int i = 0; i < vertices.size(); ++i){
			colors.push_back(color);
		}

		glGenBuffers(1, &colorbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), glm::value_ptr(colors[0]), GL_STATIC_DRAW);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		// Take care of the changed type when you create and initialize the data store of the buffer:
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), glm::value_ptr(vertices[0]), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glGenBuffers(1, &ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(glm::uvec4), glm::value_ptr(indices[0]), GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Finally calculate the proper number of indices
		lenght = (GLuint)indices.size() * 4;
	}

	glm::mat4 calc_MVP(const glm::mat4 & ViewMatrix, const glm::mat4 & ProjectionMatrix)
	{
		double scale_factor = 10;

//			First let's store our scale in a 3d vector:
		glm::vec3 scale = glm::vec3(scale_factor, scale_factor, scale_factor);

//			Now we need a basic model matrix with no transformations:
		glm::mat4 ModelMatrix = glm::mat4(1.0);

		glm::mat4 TranslationMatrix = translate(mat4(), glm::vec3(-5.f, 0.0f, -5.f));

		ModelMatrix = TranslationMatrix * ModelMatrix;

//			Now we can apply the scale to our model matrix:
		ModelMatrix = glm::scale(ModelMatrix, scale);

		return ProjectionMatrix * ViewMatrix * ModelMatrix;
	}

	void draw(const glm::mat4 & ViewMatrix, const glm::mat4 & ProjectionMatrix)
	{
		glUseProgram(programID);
//
		// Compute the MVP matrix from keyboard and mouse input
		auto MVP = calc_MVP(ViewMatrix, ProjectionMatrix);

		//		// Send our transformation to the currently bound shader,
		//		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
//
//		// render the grid:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_DEPTH_TEST);
		glBindVertexArray(vao);

		// 2nd attribute buffer : colors
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(
				1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
				3,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
		);

		glDrawElements(GL_LINES, lenght, GL_UNSIGNED_INT, NULL);
		glBindVertexArray(0);
		glDisable(GL_DEPTH_TEST);
	}

	void draw(const glm::mat4 & ViewMatrix, const glm::mat4 & ProjectionMatrix, cv::Mat & frame)
	{
		glm::mat4 MVP = calc_MVP(ViewMatrix, ProjectionMatrix);

		int frame_w = frame.size().width;
		int frame_h = frame.size().height;

		std::vector<cv::Point> points;
		for (const glm::vec3 & v3 : vertices){
			glm::vec4 v4 = {v3.x, v3.y, v3.z, 1};
			v4 = MVP * v4;

			cv::Point2i p;
			p.x = v4.x / v4.w * frame_w / 2 + frame_w / 2;
			p.y = frame_h - (v4.y / v4.w * frame_h / 2 + frame_h / 2);
			points.push_back(p);
		}

		cv::Scalar clr{
				255 * color[2],
				255 * color[1],
				255 * color[0]}
		;

		for (const auto & p : points){
			cv::circle(frame, p, 2, clr, 2, 1);
		}

		for (int i = 0; i < indices.size(); i += 1){
			for (int j = 0; j < 3; ++j){
				int indx1 = indices[i][j];
				int indx2 = indices[i][(j + 1) % 4];
				auto p1 = points[indx1];
				auto p2 = points[indx2];
				cv::line(frame, p1, p2, clr,2, 1);
			}
		}
	}

	~Grid()
	{
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ibo);
		glDeleteVertexArrays(1, &vao);
		glDeleteProgram(programID);
	}
};

class Ship
{
	GLuint vao;
	GLuint vertexbuffer;

	GLuint programID;
	GLuint MatrixID;

	const double delta_theta;

	GLuint colorbuffer;
	const glm::vec3 color;

	// 4 triangles
	// 5 unique vertices
	// 4 * 3 indices
	const std::vector<glm::vec3> vertices = {
			{0.f, 0.f, 1.f},
			{-1.f, 0.f, -1.f},
			{1.f, 0.f, -1.f},

			{0.f, 0.f, 1.f},
			{-1.f, 1.f, -1.f},
			{1.f, 1.f, -1.f},

			{0.f, 0.f, 1.f},
			{-1.f, 0.f, -1.f},
			{-1.f, 1.f, -1.f},

			{0.f, 0.f, 1.f},
			{1.f, 0.f, -1.f},
			{1.f, 1.f, -1.f},
	};

public:

	Ship(glm::vec3 color_, double delta_theta_) : color(color_), delta_theta(delta_theta_)
	{
		programID = LoadShaders(
			"TransformVertexShader.vertexshader",
			"ColorFragmentShader.fragmentshader"
		);

		// Get a handle for our "MVP" uniform
		MatrixID = glGetUniformLocation(programID, "MVP");

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		const GLfloat g_color_buffer_data[] = {
				color[0], color[1], color[2],
				color[0], color[1], color[2],
				color[0], color[1], color[2],

				color[0], color[1], color[2],
				color[0], color[1], color[2],
				color[0], color[1], color[2],

				color[0], color[1], color[2],
				color[0], color[1], color[2],
				color[0], color[1], color[2],

				color[0], color[1], color[2],
				color[0], color[1], color[2],
				color[0], color[1], color[2],
		};

// This will identify our vertex buffer
//		GLuint vertexbuffer;
// Generate 1 buffer, put the resulting identifier in vertexbuffer
		glGenBuffers(1, &vertexbuffer);
// The following commands will talk about our 'vertexbuffer' buffer
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
// Give our vertices to OpenGL.
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), glm::value_ptr(vertices[0]), GL_STATIC_DRAW);
		// Finally calculate the proper number of indices

		glGenBuffers(1, &colorbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

		glBindVertexArray(0);
	}

	glm::vec3 calc_position(std::chrono::system_clock::time_point tp)
	{
		double t = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
		t = t / 1000;

		double r = 5; // radius
		double T = 10; // period
		double theta = 2 * M_PI * t / T + delta_theta;// angle

		double x = r * cos(theta);
//		double y = r * sin(theta) / 3;
		double y = 0;
		double z = r * sin(theta);


		// rotate around X
//		double alpha = M_PI_4 / 2;
		double alpha = M_PI_4;

		glm::mat3 rotation = {
				{1, 0, 0},
				{0, cos(alpha), -sin(alpha)},
				{0, sin(alpha), cos(alpha)},
		};

		return rotation * glm::vec3{x, y, z};
	}


	glm::vec3 prev_pos = calc_position(std::chrono::system_clock::now());

	double calc_angle(const glm::vec3 & r1, const glm::vec3 & r2){
		return acos(glm::dot(glm::normalize(r1), glm::normalize(r2)));
	}

	glm::vec3 calc_axis(const glm::vec3 & r1, const glm::vec3 & r2){
		return glm::cross(r1, r2);
	}

	glm::mat4 calcMVP(const glm::mat4 & ViewMatrix, const glm::mat4 & ProjectionMatrix){
		//		double scale_factor = 0.01;
		double scale_factor = 0.5;

		glm::vec3 curr_pos = calc_position(std::chrono::system_clock::now());

//			Now we need a basic model matrix with no transformations:
		glm::mat4 ModelMatrix = glm::mat4(1.0);

		// scale model
//			First let's store our scale in a 3d vector:
		glm::vec3 scale = glm::vec3(scale_factor, scale_factor, scale_factor);
//			Now we can apply the scale to our model matrix:
		ModelMatrix = glm::scale(ModelMatrix, scale);

		// rotate model
		glm::vec3 n = glm::normalize(curr_pos - prev_pos);
		prev_pos = curr_pos;

		glm::vec3 n_xz = glm::vec3{n.x, 0, n.z};

		ModelMatrix = glm::rotate(ModelMatrix, float(calc_angle(n_xz, n)), calc_axis(n_xz, n)); // where x, y, z is axis of rotation (e.g. 0 1 0)
		ModelMatrix = glm::rotate(ModelMatrix, float(calc_angle({0, 0, 1}, n_xz)), calc_axis({0, 0, 1}, n_xz)); // where x, y, z is axis of rotation (e.g. 0 1 0)

		// translate model
		glm::mat4 TranslationMatrix = translate(mat4(), curr_pos);

		ModelMatrix = TranslationMatrix * ModelMatrix;

		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		//		// Send our transformation to the currently bound shader,
		//		// in the "MVP" uniform
		return MVP;
	}



	void draw(const glm::mat4 & ViewMatrix, const glm::mat4 & ProjectionMatrix)
	{
		// Use our shader
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//		glDisable(GL_DEPTH_TEST);
//		glDisable(GL_DEPTH_TEST);
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao);

		glm::mat4 MVP = calcMVP(ViewMatrix, ProjectionMatrix);
		//		// Send our transformation to the currently bound shader,
		//		// in the "MVP" uniform

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
				0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : colors
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(
				1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
				3,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, 4 * 3 * 3); // 12*3 indices starting at 0 -> 12 triangles

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}

	void draw(const glm::mat4 & ViewMatrix, const glm::mat4 & ProjectionMatrix, cv::Mat & frame)
	{
		glm::mat4 MVP = calcMVP(ViewMatrix, ProjectionMatrix);

		int frame_w = frame.size().width;
		int frame_h = frame.size().height;

		std::vector<cv::Point> points;
		for (const glm::vec3 & v3 : vertices){
			glm::vec4 v4 = {v3.x, v3.y, v3.z, 1};
			v4 = MVP * v4;

			cv::Point2i p;
			p.x = v4.x / v4.w * frame_w / 2 + frame_w / 2;
			p.y = frame_h - (v4.y / v4.w * frame_h / 2 + frame_h / 2);
			points.push_back(p);
		}

		cv::Scalar clr{
			255 * color[2],
			255 * color[1],
			255 * color[0]}
			;

		for (const auto & p : points){
			cv::circle(frame, p, 2, clr, 2, 1);
		}

		for (int i = 0; i < points.size(); i += 3){
			for (int j = 0; j < 3; ++j){
				auto p1 = points[i + j];
				auto p2 = points[i + (j + 1) % 3];
				cv::line(frame, p1, p2, clr,2, 1);
			}
		}
	}


	~Ship()
	{
		glDeleteBuffers(1, &vertexbuffer);
		glDeleteVertexArrays(1, &vao);
		glDeleteProgram(programID);
	}
};


mat4 LookAtRH(vec3 eye, vec3 target, vec3 up )
{
	vec3 zaxis = glm::normalize(eye - target);    // The "forward" vector.
	vec3 xaxis = glm::normalize(glm::cross(up, zaxis));// The "right" vector.
	vec3 yaxis = glm::cross(zaxis, xaxis);     // The "up" vector.

	// Create a 4x4 orientation matrix from the right, up, and forward vectors
	// This is transposed which is equivalent to performing an inverse
	// if the matrix is orthonormalized (in this case, it is).
	mat4 orientation = {
			vec4( xaxis.x, yaxis.x, zaxis.x, 0 ),
			vec4( xaxis.y, yaxis.y, zaxis.y, 0 ),
			vec4( xaxis.z, yaxis.z, zaxis.z, 0 ),
			vec4(   0,       0,       0,     1 )
	};

	// Create a 4x4 translation matrix.
	// The eye position is negated which is equivalent
	// to the inverse of the translation matrix.
	// T(v)^-1 == T(-v)
	mat4 translation = {
			vec4(   1,      0,      0,   0 ),
			vec4(   0,      1,      0,   0 ),
			vec4(   0,      0,      1,   0 ),
			vec4(-eye.x, -eye.y, -eye.z, 1 )
	};

	// Combine the orientation and translation to compute
	// the final view matrix. Note that the order of
	// multiplication is reversed because the matrices
	// are already inverted.
	return orientation * translation;
}


int main(void)
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context

	int win_width = 1024;
	int win_height = 768;

	window = glfwCreateWindow( win_width, win_height, "OpenGL", nullptr, nullptr);
	if(window == nullptr){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	// Hide the mouse and enable unlimited mouvement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, 1024 / 2, 768 / 2);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);


	std::unique_ptr<Grid> grid = std::make_unique<Grid>();
	std::unique_ptr<Ship> ship1 = std::make_unique<Ship>(glm::vec3{1, 0, 0}, 0);
	std::unique_ptr<Ship> ship2 = std::make_unique<Ship>(glm::vec3{1, 1, 0}, M_PI / 4);

	glm::vec3 cameraPosition = glm::vec3(5,10,-10);
	glm::vec3 cameraTarget = glm::vec3(0,0,0);
	glm::vec3 cameraUp = glm::vec3(0,1,0);

	glm::mat4 CameraMatrix = glm::lookAt(
			cameraPosition, // the position of your camera, in world space
			cameraTarget,  // where you want to look at, in world space
			cameraUp   // probably glm::vec3(0,1,0), but (0,-1,0) would make you looking upside-down, which can be great too
	);

	glm::mat4 ProjectionMatrix = glm::perspective(
			glm::radians(45.f), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
			(GLfloat)win_width / (GLfloat)win_height, // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
			1.0f, // Near clipping plane. Keep as big as possible, or you'll get precision issues.
			150.0f // Far clipping plane. Keep as little as possible.
	);

	glm::mat4 ViewMatrix = LookAtRH(cameraPosition, cameraTarget, cameraUp);

	bool export_to_opencv = false;

	do{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		computeMatricesFromInputs();

//		glm::mat4 ProjectionMatrix = getProjectionMatrix();

		grid->draw(ViewMatrix, ProjectionMatrix);
		ship1->draw(ViewMatrix, ProjectionMatrix);
		ship2->draw(ViewMatrix, ProjectionMatrix);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

		bool render_in_opencv = true;
		if (render_in_opencv){
			cv::Mat image(win_height, win_width, CV_8UC3, {20, 0, 0});

			grid->draw(ViewMatrix, ProjectionMatrix, image);
			ship1->draw(ViewMatrix, ProjectionMatrix, image);
			ship2->draw(ViewMatrix, ProjectionMatrix, image);

			cv::imshow("OpenCV", image);
			cv::waitKey(5);
		}

		if (export_to_opencv){
			unsigned char* buffer = new unsigned char[win_width * win_height * 3];
			glReadPixels(0, 0, win_width, win_height, GL_RGB, GL_UNSIGNED_BYTE, buffer);

			cv::Mat image(win_height, win_width, CV_8UC3, buffer);
			flip(image, image, 0);
			cvtColor(image, image, CV_RGB2BGR);
			cv::imshow("OpenCV", image);
			cv::waitKey(5);
		}

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
	       glfwWindowShouldClose(window) == 0 );

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}
