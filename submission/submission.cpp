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


std::ostream& operator << (std::ostream& os, const glm::uvec4 & v){
	os << v.x << " " << v.y << " " << v.z << " " << v.w;
	return os;
}

class Grid
{
	GLuint vao;
	GLuint vbo;
	GLuint ibo;
	int slices = 10;
	int lenght;

	GLuint programID;
	GLuint MatrixID;

	const bool horizontal;

public:

	Grid(bool horizontal_ = true) : horizontal(horizontal_)
	{
		if (horizontal){
			programID = LoadShaders(
					"TransformVertexShader.vertexshader",
					"SimpleFragmentShader.fragmentshader"
			);
		} else {
			programID = LoadShaders(
					"TransformVertexShader.vertexshader",
					"SimpleFragmentShaderRed.fragmentshader"
			);
		}

		// Get a handle for our "MVP" uniform
		MatrixID = glGetUniformLocation(programID, "MVP");

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

//		GLuint programID = LoadShaders(
//				"TransformVertexShader.vertexshader",
//				"SimpleFragmentShader.fragmentshader"
//				);

		std::vector<glm::vec3> vertices;
		std::vector<glm::uvec4> indices;

		if (horizontal){
			for(int j = 0; j <= slices; ++j) {
				for(int i = 0; i <= slices; ++i) {
					float x = (float) i / (float) slices;
					float y = 0;
					float z = (float) j / (float) slices;
					vertices.push_back(glm::vec3(x, y, z));
				}
			}
		} else {
			for(int j = 0; j <= slices; ++j) {
				for(int i = 0; i <= slices; ++i) {
					float x = (float) i / (float) slices;
					float y = (float) j / (float) slices;
					float z = 0;
					vertices.push_back(glm::vec3(x, y, z));
				}
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

//				std::cout << indices.back() << std::endl;

				indices.push_back(glm::uvec4(
						row2 + i + 1,
						row2 + i,
						row2 + i,
						row1 + i
						));

//				std::cout << indices.back() << std::endl;
//				std::cout << std::endl;
			}
		}

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

	void draw()
	{
		glm::mat4 CameraMatrix = glm::lookAt(
				glm::vec3(0,1,-1), // the position of your camera, in world space
				glm::vec3(0,0,0),   // where you want to look at, in world space
				glm::vec3(0,1,0)        // probably glm::vec3(0,1,0), but (0,-1,0) would make you looking upside-down, which can be great too
		);

//		std::cout << "programID " << programID << std::endl;
		glUseProgram(programID);
//
		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();

		bool locked_view = false;

		double scale_factor = 10;

		if (locked_view){
			glm::mat4 ProjectionMatrix = getProjectionMatrix();
			glm::mat4 ViewMatrix = getViewMatrix();
			glm::mat4 ModelMatrix = glm::mat4(1.0);
			glm::mat4 MVP = ProjectionMatrix * CameraMatrix * ModelMatrix;
			//		// Send our transformation to the currently bound shader,
			//		// in the "MVP" uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		} else {
			glm::mat4 ProjectionMatrix = getProjectionMatrix();
			glm::mat4 ViewMatrix = getViewMatrix();

//			First let's store our scale in a 3d vector:
			glm::vec3 scale = glm::vec3(scale_factor, scale_factor, scale_factor);

//			Now we need a basic model matrix with no transformations:
			glm::mat4 ModelMatrix = glm::mat4(1.0);

			glm::mat4 TranslationMatrix = translate(mat4(), glm::vec3(-5.f, 0.0f, -5.f));

			ModelMatrix = TranslationMatrix * ModelMatrix;

//			Now we can apply the scale to our model matrix:
			ModelMatrix = glm::scale(ModelMatrix, scale);


			glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			//		// Send our transformation to the currently bound shader,
			//		// in the "MVP" uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		}
//
//		// render the grid:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//
		glEnable(GL_DEPTH_TEST);
//
		glBindVertexArray(vao);
		glDrawElements(GL_LINES, lenght, GL_UNSIGNED_INT, NULL);
		glBindVertexArray(0);
//////
		glDisable(GL_DEPTH_TEST);
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

public:

	Ship(bool red, double delta_theta_) : delta_theta(delta_theta_)
	{
		if (!red){
			programID = LoadShaders(
					"TransformVertexShader.vertexshader",
					"SimpleFragmentShaderYellow.fragmentshader"
			);
		} else {
			programID = LoadShaders(
					"TransformVertexShader.vertexshader",
					"SimpleFragmentShaderRed.fragmentshader"
			);
		}

		// Get a handle for our "MVP" uniform
		MatrixID = glGetUniformLocation(programID, "MVP");

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

//		GLuint programID = LoadShaders(
//				"TransformVertexShader.vertexshader",
//				"SimpleFragmentShader.fragmentshader"
//				);

		// 4 triangles
		// 5 unique vertices
		// 4 * 3 indices

		static const GLfloat g_vertex_buffer_data[] {
				0.f, 0.f, 0.f,
				-1.f, 0.f, -2.f,
				1.f, 0.f, -2.f,

				0.f, 0.f, 0.f,
				-1.f, 1.f, -2.f,
				1.f, 1.f, -2.f,

				0.f, 0.f, 0.f,
				-1.f, 0.f, -2.f,
				-1.f, 1.f, -2.f,

				0.f, 0.f, 0.f,
				1.f, 0.f, -2.f,
				1.f, 1.f, -2.f,
		};


// This will identify our vertex buffer
		GLuint vertexbuffer;
// Generate 1 buffer, put the resulting identifier in vertexbuffer
		glGenBuffers(1, &vertexbuffer);
// The following commands will talk about our 'vertexbuffer' buffer
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
// Give our vertices to OpenGL.
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
		// Finally calculate the proper number of indices

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
		double y = r * sin(theta) / 3;
//		double y = 0;
		double z = r * sin(theta);

		return glm::vec3{x, y, z};
	}


	glm::vec3 prev_pos = calc_position(std::chrono::system_clock::now());

	double calc_turn_angle(const glm::vec3 & r1, const glm::vec3 & r2){
		auto dr = r2 - r1;
		return atan2(dr.x, dr.z);
	}

	void draw()
	{
		// Use our shader
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_DEPTH_TEST);

		glBindVertexArray(vao);

//		bool locked_view = true;
		bool locked_view = false;

//		double scale_factor = 0.01;
		double scale_factor = 0.5;

		glm::vec3 curr_pos = calc_position(std::chrono::system_clock::now());

		glm::vec3 dr = curr_pos - prev_pos;

		if (locked_view){
			glm::mat4 CameraMatrix = glm::lookAt(
					glm::vec3(0,1,-5), // the position of your camera, in world space
					glm::vec3(0,0,0),   // where you want to look at, in world space
					glm::vec3(0,1,0)        // probably glm::vec3(0,1,0), but (0,-1,0) would make you looking upside-down, which can be great too
			);

			glm::mat4 ProjectionMatrix = getProjectionMatrix();
			glm::mat4 ViewMatrix = getViewMatrix();
			glm::mat4 ModelMatrix = glm::mat4(1.0);
			glm::mat4 MVP = ProjectionMatrix * CameraMatrix * ModelMatrix;
			//		// Send our transformation to the currently bound shader,
			//		// in the "MVP" uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		} else {
			glm::mat4 ProjectionMatrix = getProjectionMatrix();
			glm::mat4 ViewMatrix = getViewMatrix();

//			Now we need a basic model matrix with no transformations:
			glm::mat4 ModelMatrix = glm::mat4(1.0);

			// scale model
//			First let's store our scale in a 3d vector:
			glm::vec3 scale = glm::vec3(scale_factor, scale_factor, scale_factor);
//			Now we can apply the scale to our model matrix:
			ModelMatrix = glm::scale(ModelMatrix, scale);

			// rotate model
			auto angle_in_radians = float(calc_turn_angle(prev_pos, curr_pos));
			ModelMatrix = glm::rotate(ModelMatrix, angle_in_radians, glm::vec3 (0, 1, 0)); // where x, y, z is axis of rotation (e.g. 0 1 0)

			// translate model
			glm::mat4 TranslationMatrix = translate(mat4(), curr_pos);
			ModelMatrix = TranslationMatrix * ModelMatrix;

			// rotate orbit
//			ModelMatrix = glm::rotate(ModelMatrix, M_PI_4f32, glm::vec3 (1, 0, 0)); // where x, y, z is axis of rotation (e.g. 0 1 0)

			// translate orbit
			ModelMatrix = translate(mat4(), glm::vec3(0, 3, 0)) * ModelMatrix;

			// rotate orbit
//			ModelMatrix = glm::rotate(ModelMatrix, M_PI_4f32, glm::vec3 (0, 1, 0)); // where x, y, z is axis of rotation (e.g. 0 1 0)

			glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			//		// Send our transformation to the currently bound shader,
			//		// in the "MVP" uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		}

		prev_pos = curr_pos;

//		glm::mat4 ProjectionMatrix = getProjectionMatrix();
//		glm::mat4 ViewMatrix = getViewMatrix();
//		glm::mat4 ModelMatrix = glm::mat4(1.0);
//		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
//
//		// Send our transformation to the currently bound shader,
//		// in the "MVP" uniform
//		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// Bind our texture in Texture Unit 0
//		glActiveTexture(GL_TEXTURE0);
//		glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
//		glUniform1i(TextureID, 0);

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

		// 2nd attribute buffer : UVs
//		glEnableVertexAttribArray(1);
////		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
//		glVertexAttribPointer(
//				1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
//				2,                                // size : U+V => 2
//				GL_FLOAT,                         // type
//				GL_FALSE,                         // normalized?
//				0,                                // stride
//				(void*)0                          // array buffer offset
//		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, 4 * 3 * 3); // 12*3 indices starting at 0 -> 12 triangles

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}

	~Ship()
	{
		glDeleteBuffers(1, &vertexbuffer);
		glDeleteVertexArrays(1, &vao);
		glDeleteProgram(programID);
	}
};

class NumberCube{
	GLuint VertexArrayID;
	GLuint programID;
	GLuint MatrixID;
	GLuint Texture;
	GLuint TextureID;

	GLuint vertexbuffer;
	GLuint uvbuffer;

public:

	NumberCube(){

		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);

		// Create and compile our GLSL program from the shaders
		programID = LoadShaders(
				"TransformVertexShader.vertexshader",
				"TextureFragmentShader.fragmentshader"
		);

		// Get a handle for our "MVP" uniform
		MatrixID = glGetUniformLocation(programID, "MVP");

		// Load the texture
		Texture = loadDDS("uvtemplate.DDS");

		// Get a handle for our "myTextureSampler" uniform
		TextureID = glGetUniformLocation(programID, "myTextureSampler");

		// Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
		// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
		GLfloat g_vertex_buffer_data[] = {
				-1.0f,-1.0f,-1.0f,
				-1.0f,-1.0f, 1.0f,
				-1.0f, 1.0f, 1.0f,
				1.0f, 1.0f,-1.0f,
				-1.0f,-1.0f,-1.0f,
				-1.0f, 1.0f,-1.0f,
				1.0f,-1.0f, 1.0f,
				-1.0f,-1.0f,-1.0f,
				1.0f,-1.0f,-1.0f,
				1.0f, 1.0f,-1.0f,
				1.0f,-1.0f,-1.0f,
				-1.0f,-1.0f,-1.0f,
				-1.0f,-1.0f,-1.0f,
				-1.0f, 1.0f, 1.0f,
				-1.0f, 1.0f,-1.0f,
				1.0f,-1.0f, 1.0f,
				-1.0f,-1.0f, 1.0f,
				-1.0f,-1.0f,-1.0f,
				-1.0f, 1.0f, 1.0f,
				-1.0f,-1.0f, 1.0f,
				1.0f,-1.0f, 1.0f,
				1.0f, 1.0f, 1.0f,
				1.0f,-1.0f,-1.0f,
				1.0f, 1.0f,-1.0f,
				1.0f,-1.0f,-1.0f,
				1.0f, 1.0f, 1.0f,
				1.0f,-1.0f, 1.0f,
				1.0f, 1.0f, 1.0f,
				1.0f, 1.0f,-1.0f,
				-1.0f, 1.0f,-1.0f,
				1.0f, 1.0f, 1.0f,
				-1.0f, 1.0f,-1.0f,
				-1.0f, 1.0f, 1.0f,
				1.0f, 1.0f, 1.0f,
				-1.0f, 1.0f, 1.0f,
				1.0f,-1.0f, 1.0f
		};

		// Two UV coordinatesfor each vertex. They were created with Blender.
		GLfloat g_uv_buffer_data[] = {
				0.000059f, 0.000004f,
				0.000103f, 0.336048f,
				0.335973f, 0.335903f,
				1.000023f, 0.000013f,
				0.667979f, 0.335851f,
				0.999958f, 0.336064f,
				0.667979f, 0.335851f,
				0.336024f, 0.671877f,
				0.667969f, 0.671889f,
				1.000023f, 0.000013f,
				0.668104f, 0.000013f,
				0.667979f, 0.335851f,
				0.000059f, 0.000004f,
				0.335973f, 0.335903f,
				0.336098f, 0.000071f,
				0.667979f, 0.335851f,
				0.335973f, 0.335903f,
				0.336024f, 0.671877f,
				1.000004f, 0.671847f,
				0.999958f, 0.336064f,
				0.667979f, 0.335851f,
				0.668104f, 0.000013f,
				0.335973f, 0.335903f,
				0.667979f, 0.335851f,
				0.335973f, 0.335903f,
				0.668104f, 0.000013f,
				0.336098f, 0.000071f,
				0.000103f, 0.336048f,
				0.000004f, 0.671870f,
				0.336024f, 0.671877f,
				0.000103f, 0.336048f,
				0.336024f, 0.671877f,
				0.335973f, 0.335903f,
				0.667969f, 0.671889f,
				1.000004f, 0.671847f,
				0.667979f, 0.335851f
		};

		glGenBuffers(1, &vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

		glGenBuffers(1, &uvbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);
	}

	void draw(){
		glBindVertexArray(VertexArrayID);

		// Clear the screen


		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// Use our shader
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader,
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

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

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
				1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
				2,                                // size : U+V => 2
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, 12*3); // 12*3 indices starting at 0 -> 12 triangles

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		glBindVertexArray(0);
	}

	~NumberCube(){

		// Cleanup VBO and shader
		glDeleteBuffers(1, &vertexbuffer);
		glDeleteBuffers(1, &uvbuffer);
		glDeleteProgram(programID);
		glDeleteTextures(1, &TextureID);
		glDeleteVertexArrays(1, &VertexArrayID);
	}
};

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
	window = glfwCreateWindow( 1024, 768, "Tutorial 0 - Keyboard and Mouse", nullptr, nullptr);
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

	// Cull triangles which normal is not towards the camera
//	glEnable(GL_CULL_FACE);
//	glEnable(GL_CULL_FACE);

	std::unique_ptr<Grid> grid = std::make_unique<Grid>();
	std::unique_ptr<Ship> ship1 = std::make_unique<Ship>(true, 0);
	std::unique_ptr<Ship> ship2 = std::make_unique<Ship>(false, M_PI / 4);

	do{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//		cube->draw();
		grid->draw();
		ship1->draw();
		ship2->draw();
//		grid2->draw();
//		glEnable(GL_DEPTH_TEST);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
	       glfwWindowShouldClose(window) == 0 );

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

