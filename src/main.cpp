
/* Compilation on Linux: 
 g++ -std=c++17 ./src/*.cpp -o prog -I ./include/ -I./../common/thirdparty/ -lSDL2 -ldl
*/

// Third Party Libraries
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp> 

// C++ Standard Template Library (STL)
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <algorithm>
#include <ctime>
#include <cstdlib>

// vvvvvvvvvvvvvvvvvvvvvvvvvv Globals vvvvvvvvvvvvvvvvvvvvvvvvvv
// Globals generally are prefixed with 'g' in this application.

// Screen Dimensions
int gScreenWidth 						= 1080;
int gScreenHeight 						= 1080;
SDL_Window* gGraphicsApplicationWindow 	= nullptr;
SDL_GLContext gOpenGLContext			= nullptr;

Uint64 gLastTime{ 0 };

// Main loop flag
bool gQuit = false; // If this is quit = 'true' then the program terminates.

GLuint gGraphicsPipelineShaderProgram	= 0;

GLuint gVertexArrayObject					= 0;

GLuint 	gVertexBufferObject					= 0;

GLuint 	gIndexBufferObject                  = 0;


float gCenterX{ 0.0f };
float gCenterY{ 0.0f };
float gZoom{ 1.0 };
float gConstantReal{ 0.1f };
float gConstantImag{ 0.5f };

std::vector<float> gPixelData(gScreenWidth * gScreenHeight, 0.0f);
glm::vec4 gRanges = glm::vec4(0.0001f, 0.33333f, 0.66667f, 1.00f);

int gMaxIterations = 15;
bool gPause = false;
bool gReverse = false;
bool gMandelbrot = true;

// ^^^^^^^^^^^^^^^^^^^^^^^^ Globals ^^^^^^^^^^^^^^^^^^^^^^^^^^^



// vvvvvvvvvvvvvvvvvvv Error Handling Routines vvvvvvvvvvvvvvv
static void GLClearAllErrors(){
    while(glGetError() != GL_NO_ERROR){
    }
}

// Returns true if we have an error
static bool GLCheckErrorStatus(const char* function, int line){
    while(GLenum error = glGetError()){
        std::cout << "OpenGL Error:" << error 
                  << "\tLine: " << line 
                  << "\tfunction: " << function << std::endl;
        return true;
    }
    return false;
}

#define GLCheck(x) GLClearAllErrors(); x; GLCheckErrorStatus(#x,__LINE__);
// ^^^^^^^^^^^^^^^^^^^ Error Handling Routines ^^^^^^^^^^^^^^^




std::string LoadShaderAsString(const std::string& filename){
    // Resulting shader program loaded as a single string
    std::string result = "";

    std::string line = "";
    std::ifstream myFile(filename.c_str());

    if(myFile.is_open()){
        while(std::getline(myFile, line)){
            result += line + '\n';
        }
        myFile.close();

    }

    return result;
}


GLuint CompileShader(GLuint type, const std::string& source){
	// Compile our shaders
	GLuint shaderObject;

	if(type == GL_VERTEX_SHADER){
		shaderObject = glCreateShader(GL_VERTEX_SHADER);
	}else if(type == GL_FRAGMENT_SHADER){
		shaderObject = glCreateShader(GL_FRAGMENT_SHADER);
	}

	const char* src = source.c_str();
	glShaderSource(shaderObject, 1, &src, nullptr);
	
	glCompileShader(shaderObject);

	int result;
	
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &result);

	if(result == GL_FALSE){
		int length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		char* errorMessages = new char[length]; 
		glGetShaderInfoLog(shaderObject, length, &length, errorMessages);

		if(type == GL_VERTEX_SHADER){
			std::cout << "ERROR: GL_VERTEX_SHADER compilation failed!\n" << errorMessages << "\n";
		}else if(type == GL_FRAGMENT_SHADER){
			std::cout << "ERROR: GL_FRAGMENT_SHADER compilation failed!\n" << errorMessages << "\n";
		}
		delete[] errorMessages;

		glDeleteShader(shaderObject);

		return 0;
	}

  return shaderObject;
}



GLuint CreateShaderProgram(const std::string& vertexShaderSource, const std::string& fragmentShaderSource){

    GLuint programObject = glCreateProgram();

    GLuint myVertexShader   = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint myFragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    glAttachShader(programObject,myVertexShader);
    glAttachShader(programObject,myFragmentShader);
    glLinkProgram(programObject);

    glValidateProgram(programObject);

    glDetachShader(programObject,myVertexShader);
    glDetachShader(programObject,myFragmentShader);

	glDeleteShader(myVertexShader);
    glDeleteShader(myFragmentShader);

    return programObject;
}


void CreateGraphicsPipeline(){

    std::string vertexShaderSource      = LoadShaderAsString("./shaders/vert.glsl");
	std::string fragmentShaderSource    = LoadShaderAsString("./shaders/frag.glsl");

	gGraphicsPipelineShaderProgram = CreateShaderProgram(vertexShaderSource,fragmentShaderSource);
}


void InitializeProgram(){
	// Initialize SDL
	if(SDL_Init(SDL_INIT_VIDEO)< 0){
		std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << "\n";
		exit(1);
	}
	

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);


	gGraphicsApplicationWindow = SDL_CreateWindow( "Mandelbrot Visualizer",
													SDL_WINDOWPOS_UNDEFINED,
													SDL_WINDOWPOS_UNDEFINED,
													gScreenWidth,
													gScreenHeight,
													SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );

	if( gGraphicsApplicationWindow == nullptr ){
		std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << "\n";
		exit(1);
	}

	gOpenGLContext = SDL_GL_CreateContext( gGraphicsApplicationWindow );
	if( gOpenGLContext == nullptr){
		std::cout << "OpenGL context could not be created! SDL Error: " << SDL_GetError() << "\n";
		exit(1);
	}

	if(!gladLoadGLLoader(SDL_GL_GetProcAddress)){
		std::cout << "glad did not initialize" << std::endl;
		exit(1);
	}

	gLastTime = SDL_GetTicks();
}

glm::vec4 find_ranges(std::vector<float> &in)
{
    std::sort(in.begin(), in.end());
    int min = 0;
    while (in[min] == 0.0f)
    {
        min++;
    }

    int len = in.size() - min;
	glm::vec4 color_ranges;


	color_ranges = glm::vec4( in[min], in[min + len * 4 / 5 - 1], in[min + len * 9 / 10 - 1], in[in.size() - 1] );


    return color_ranges;
}


void VertexSpecification(){

	const std::vector<GLfloat> vertexData
	{
		-1.0f, -1.0f, -0.0f,
		 1.0f,  1.0f, -0.0f,
		-1.0f,  1.0f, -0.0f,
		 1.0f, -1.0f, -0.0f
	};



	glGenVertexArrays(1, &gVertexArrayObject);

	glBindVertexArray(gVertexArrayObject);


	glGenBuffers(1, &gVertexBufferObject);

	glBindBuffer(GL_ARRAY_BUFFER, gVertexBufferObject);

	glBufferData(GL_ARRAY_BUFFER, 								// Kind of buffer we are working with 
																
				 vertexData.size() * sizeof(GL_FLOAT), 			// Size of data in bytes
				 vertexData.data(), 							// Raw array of data
				 GL_STATIC_DRAW);								// How we intend to use the data


    std::vector<GLuint> indexBufferData = 
	{
		    0, 1, 2,
		    0, 3, 1
	};


	glGenBuffers(1, &gIndexBufferObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 								// Kind of buffer we are working with 
																
				 indexBufferData.size() * sizeof(GLuint), 	// Size of data in bytes
				 indexBufferData.data(), 						// Raw array of data
				 GL_STATIC_DRAW);





	glEnableVertexAttribArray(0);

    glVertexAttribPointer(0,  		// Attribute 0 corresponds to the enabled glEnableVertexAttribArray
							  		
							  		
                          3,  		// The number of components (e.g. x,y,z = 3 components)
                          GL_FLOAT, // Type
                          GL_FALSE, // Is the data normalized
                          sizeof(GL_FLOAT)*3, 		// Stride
                          (void*)0	// Offset
    );


	
	glBindVertexArray(0);
	
	glDisableVertexAttribArray(0);
}


void PreDraw(){
	 glEnable(GL_DEPTH_TEST);
    // Initialize clear color
    // This is the background of the screen.
    glViewport(0, 0, gScreenWidth, gScreenHeight);
    glClearColor( 1.f, 1.f, 0.f, 1.f );

    //Clear color buffer and Depth Buffer
  	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);



   	glUseProgram(gGraphicsPipelineShaderProgram);

	glUniform1f(glGetUniformLocation(gGraphicsPipelineShaderProgram, "zoom"), gZoom);
	glUniform1f(glGetUniformLocation(gGraphicsPipelineShaderProgram, "center_x"), gCenterX);
	glUniform1f(glGetUniformLocation(gGraphicsPipelineShaderProgram, "center_y"), gCenterY);
	glUniform1f(glGetUniformLocation(gGraphicsPipelineShaderProgram, "width"), gScreenWidth);
	glUniform1f(glGetUniformLocation(gGraphicsPipelineShaderProgram, "height"), gScreenHeight);
	glUniform1i(glGetUniformLocation(gGraphicsPipelineShaderProgram, "max_iterations"), gMaxIterations);
	glUniform1i(glGetUniformLocation(gGraphicsPipelineShaderProgram, "mandelbrot"), gMandelbrot);
	glUniform1f(glGetUniformLocation(gGraphicsPipelineShaderProgram, "constant_x"), gConstantReal);
	glUniform1f(glGetUniformLocation(gGraphicsPipelineShaderProgram, "constant_y"), gConstantImag);


	glUniform4f(glGetUniformLocation(gGraphicsPipelineShaderProgram, "color_ranges"), gRanges.x, gRanges.y, gRanges.z, gRanges.w);
}


void Animate()
{
	if(SDL_GetTicks() - gLastTime > 125 && !gMandelbrot)
	{
		gLastTime += 125;
		gConstantImag -= 0.0015;
		gConstantReal -= 0.0015;
		if(gConstantImag <= -0.75)
		{
			gConstantImag = 0.45;
			gConstantReal = 0.55;
		}
		
	}
}



void Draw(){
	glBindVertexArray(gVertexArrayObject);


	glBindBuffer(GL_ARRAY_BUFFER, gVertexBufferObject);


  glDrawElements(GL_TRIANGLES,
                    6,
                    GL_UNSIGNED_INT,
                    0);


  glUseProgram(0);


}


void Input(){
	SDL_Event e;

	while(SDL_PollEvent( &e ) != 0){

		if(e.type == SDL_QUIT){
			gQuit = true;
		}

		if(e.type == SDL_KEYDOWN){
			if(e.key.keysym.sym == SDLK_r)
			{
				gZoom = 1.0f;
				gCenterY = 0.0f;
				gCenterX = 0.0f;
				gMaxIterations = 15;
			}
			if(e.key.keysym.sym == SDLK_p && !gMandelbrot)
			{
				gPause = !gPause;
			}
			if(e.key.keysym.sym == SDLK_j && gMandelbrot)
			{
				gMandelbrot = false;
				gZoom = 0.8f;
				gCenterY = 0.0f;
				gCenterX = 0.0f;
				gMaxIterations = 300;
			}
			if(e.key.keysym.sym == SDLK_m && !gMandelbrot)
			{
				gMandelbrot = true;
				gZoom = 1.0f;
				gCenterY = 0.0f;
				gCenterX = 0.0f;
				gMaxIterations = 15;
			}
		}
	}



    // Retrieve keyboard state
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_UP]) {
		gCenterY = gCenterY + 0.0025f * gZoom;
        if (gCenterY > 1.0f)
        {
            gCenterY = 1.0f;
		}
    }
    if (state[SDL_SCANCODE_DOWN]) {
        gCenterY = gCenterY - 0.0025f * gZoom;
        if (gCenterY < -1.0f)
        {
            gCenterY = -1.0f;
        }
    }
    if (state[SDL_SCANCODE_LEFT]) {
        gCenterX = gCenterX - 0.0025f * gZoom;
        if (gCenterX < -1.0f)
        {
            gCenterX = -1.0f;
        }
    }
    if (state[SDL_SCANCODE_RIGHT]) {
        gCenterX = gCenterX + 0.0025f * gZoom;
        if (gCenterX > 1.0f)
        {
            gCenterX = 1.0f;
        }
    }

    if (state[SDL_SCANCODE_LSHIFT]) {
        gZoom = gZoom * 1.04f;
		if(gMandelbrot)
		{
			gMaxIterations--;
		}
        if (gZoom > 1.0f)
        {
            gZoom = 1.0f;
        }
    	if (gMaxIterations < 15)
        {
			gMaxIterations = 15;
        }
    }
    if (state[SDL_SCANCODE_LCTRL]) {
        gZoom = gZoom * 0.975f;
		if(gMandelbrot)
		{
			gMaxIterations++;
		}
        if (gZoom < 0.000005f)
        {
            gZoom = 0.000005f;
        }
    	if (gMaxIterations > 500)
        {
			gMaxIterations = 500;
        }
    }
}


void MainLoop(){
	std::cout << "Use arrow keys to navigate around" << std::endl
	<< "Use lCtrl to zoom in and lShift to zoom out" << std::endl
	<< "Press R to reset your zoom and position" << std::endl
	<< "Press J to switch to Julia mode, and M to switch to Mandelbrot mode" << std::endl
	<< "When in Julia mode, press P to pause the animation" << std::endl;

	while(!gQuit){

		Input();

		PreDraw();

		if(!gPause)
		{
			Animate();
		}

		Draw();

		SDL_GL_SwapWindow(gGraphicsApplicationWindow);

		glReadPixels(0, 0, gScreenWidth, gScreenHeight, GL_DEPTH_COMPONENT, GL_FLOAT, gPixelData.data());
		gRanges = find_ranges(gPixelData);
	}
}



void CleanUp(){
	SDL_DestroyWindow(gGraphicsApplicationWindow );
	gGraphicsApplicationWindow = nullptr;

    glDeleteBuffers(1, &gVertexBufferObject);
    glDeleteVertexArrays(1, &gVertexArrayObject);

    glDeleteProgram(gGraphicsPipelineShaderProgram);

	SDL_Quit();
}


int main( int argc, char* args[] ){
	// 1. Setup the graphics program
	InitializeProgram();
	
	// 2. Setup our geometry
	VertexSpecification();
	
	// 3. Create our graphics pipeline
	// 	- At a minimum, this means the vertex and fragment shader
	CreateGraphicsPipeline();
	
	// 4. Call the main application loop
	MainLoop();	

	// 5. Call the cleanup function when our program terminates
	CleanUp();

	return 0;
}
