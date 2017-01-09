// tag::C++11check[]
#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if __cplusplus < 201103L
#pragma message("WARNING: the compiler may not be C++11 compliant. __cplusplus version is : " STRING(__cplusplus))
#endif
// end::C++11check[]

// tag::includes[]
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <cassert>

#include <GL/glew.h>
#include <SDL2/SDL.h>

#define GLM_FORCE_RADIANS // suppress a warning in GLM 0.9.5
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
// end::includes[]

// tag::using[]
// see https://isocpp.org/wiki/faq/Coding-standards#using-namespace-std
// don't use the whole namespace, either use the specific ones you want, or just type std::
using std::cout;
using std::cerr;
using std::endl;
using std::max;
using std::string;
// end::using[]


// tag::globalVariables[]
std::string exeName;
SDL_Window *win; //pointer to the SDL_Window
SDL_GLContext context; //the SDL_GLContext
int frameCount = 0;
std::string frameLine = "";
// end::globalVariables[]

// tag::loadShader[]
std::string loadShader(const string filePath) {
	std::ifstream fileStream(filePath, std::ios::in | std::ios::binary);
	if (fileStream)
	{
		string fileData((std::istreambuf_iterator<char>(fileStream)),
			(std::istreambuf_iterator<char>()));

		cout << "Shader Loaded from " << filePath << endl;
		return fileData;
	}
	else
	{
		cerr << "Shader could not be loaded - cannot read file " << filePath << ". File does not exist." << endl;
		return "";
	}
}
// end::loadShader[]

//Function to return all the data in a log file in the form of a float array
std::vector<GLfloat> loadLog(const string filePath, int scale, std::vector<GLfloat> RGB)
{
	//Create Array for Log
	std::vector<GLfloat> LogData;
	//Load File
	std::ifstream fileStream(filePath, std::ios::in | std::ios::binary);
	if (fileStream)
	{
		//parse Log
		std::string line;
		while (std::getline(fileStream, line))
		{ 
			std::size_t prev = 0, pos;
			//Parse out info not needed
			int count = 0;
			while ((pos = line.find_first_of("X=Y=Z=; ", prev)) != std::string::npos)
			{
				if (pos > prev)
				{
					if (scale == 0)
						LogData.push_back(std::stof(line.substr(prev, pos - prev)));
					else
					{
						//add data to array							//scale down for window size
						LogData.push_back((std::stof(line.substr(prev, pos - prev))) / scale);
						count++;
						
					}
				}
				prev = pos + 1;
				if (count == 3)
				{
					count = 0;
					LogData.push_back(RGB[0]); LogData.push_back(RGB[1]); LogData.push_back(RGB[2]); 
				}
			}
			//stop loop when reaching the end
			if (prev < line.length())
			{
				//push back last 6 bits of data
				break;
			}
		}
		cout << "LOG Loaded from " << filePath << endl;
		//Save to a single string
		/*string fileData((std::istreambuf_iterator<char>(fileStream)),(std::istreambuf_iterator<char>()));*/
		//return data
		return LogData;
	}
	else
	{
		cerr << "LOG could not be loaded - cannot read file " << filePath << ". File does not exist." << endl;
		return LogData;
	}
}

//Function to return all the data in a log file in the form of a float array
std::vector<GLfloat> loadHeatMapData(const string filePath)
{
	//Create Array for Log
	std::vector<GLfloat> LogData;
	//Load File
	std::ifstream fileStream(filePath, std::ios::in | std::ios::binary);
	if (fileStream)
	{
		//parse Log
		std::string line;
		while (std::getline(fileStream, line))
		{
			std::size_t prev = 0, pos;
			//Parse out info not needed
			while ((pos = line.find_first_of("X=Y=Z=; ", prev)) != std::string::npos)
			{
				if (pos > prev)
				{
					LogData.push_back(std::stof(line.substr(prev, pos - prev)));
				}
				prev = pos + 1;
			}
			//stop loop when reaching the end
			if (prev < line.length())
			{
				break;
			}
		}
		//return data
		return LogData;
	}
	else
	{
		return LogData;
	}
}

//our variables
bool done = false;

// tag::gameState[]
//the translation vector we'll pass to our GLSL program
glm::vec3 position1 = { 0.5f, 0.0f, 0.0f };
glm::vec3 position2 = {-0.5f, 0.0f, 0.0f };
glm::vec3 Centre =    { 0.0f, 0.0f, 0.0f };

//Camera Start values
//Location of the camera
glm::vec3 Cam1 = { 0.0f, 0.0f, 5.0f };
//What the camera is looking at
glm::vec3 Cam2 = { 0.0f, 0.0f, 0.0f };
//Rotation of the camera
glm::vec3 Cam3 = { 0.0f, 0.000001f, 0.0f };

// end::gameState[]

// tag::GLVariables[]
//our GL and GLSL variables
//programIDs
GLuint theProgram; //GLuint that we'll fill in to refer to the GLSL program (only have 1 at this point)
GLint positionLocation; //GLuint that we'll fill in with the location of the `position` attribute in the GLSL
GLint vertexColorLocation; //GLuint that we'll fill in with the location of the `vertexColor` attribute in the GLSL
GLint textureLocation;
GLint colorLocation;

//uniform location
GLint modelMatrixLocation;
GLint viewMatrixLocation;
GLint projectionMatrixLocation;

GLuint vertexDataBufferObject[11];
GLuint vertexArrayObject[11];
std::vector<GLfloat> PlayerPosition[11];
std::vector<GLfloat> Count[5];
std::vector<GLfloat> Colours;

glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;
glm::mat4 modelMatrix;

// end::GLVariables[]

// end Global Variables
/////////////////////////

// tag::initialise[]
void initialise()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
		exit(1);
	}
	cout << "SDL initialised OK!\n";
}
// end::initialise[]

// tag::createWindow[]
void createWindow()
{
	//get executable name, and use as window title
	int beginIdxWindows = exeName.rfind("\\"); //find last occurrence of a backslash
	int beginIdxLinux = exeName.rfind("/"); //find last occurrence of a forward slash
	int beginIdx = max(beginIdxWindows, beginIdxLinux);
	std::string exeNameEnd = exeName.substr(beginIdx + 1);
	const char *exeNameCStr = exeNameEnd.c_str();

	//create window
	win = SDL_CreateWindow(exeNameCStr, 100, 100, 900, 800, SDL_WINDOW_OPENGL); //same height and width makes the window square ...

																				//error handling
	if (win == nullptr)
	{
		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}
	cout << "SDL CreatedWindow OK!\n";
}
// end::createWindow[]

// tag::setGLAttributes[]
void setGLAttributes()
{
	int major = 3;
	int minor = 3;
	cout << "Built for OpenGL Version " << major << "." << minor << endl; //ahttps://en.wikipedia.org/wiki/OpenGL_Shading_Language#Versions
																		  // set the opengl context version
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //core profile
	cout << "Set OpenGL context to versicreate remote branchon " << major << "." << minor << " OK!\n";
}
// tag::setGLAttributes[]

// tag::createContext[]
void createContext()
{
	setGLAttributes();

	context = SDL_GL_CreateContext(win);
	if (context == nullptr) {
		SDL_DestroyWindow(win);
		std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}
	cout << "Created OpenGL context OK!\n";
}
// end::createContext[]

// tag::initGlew[]
void initGlew()
{
	GLenum rev;
	glewExperimental = GL_TRUE; //GLEW isn't perfect - see https://www.opengl.org/wiki/OpenGL_Loading_Library#GLEW
	rev = glewInit();
	if (GLEW_OK != rev) {
		std::cerr << "GLEW Error: " << glewGetErrorString(rev) << std::endl;
		SDL_Quit();
		exit(1);
	}
	else {
		cout << "GLEW Init OK!\n";
	}
}
// end::initGlew[]

// tag::createShader[]
GLuint createShader(GLenum eShaderType, const std::string &strShaderFile)
{
	GLuint shader = glCreateShader(eShaderType);
	//error check
	const char *strFileData = strShaderFile.c_str();
	glShaderSource(shader, 1, &strFileData, NULL);

	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

		const char *strShaderType = NULL;
		switch (eShaderType)
		{
		case GL_VERTEX_SHADER: strShaderType = "vertex"; break;
		case GL_GEOMETRY_SHADER: strShaderType = "geometry"; break;
		case GL_FRAGMENT_SHADER: strShaderType = "fragment"; break;
		}

		fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete[] strInfoLog;
	}

	return shader;
}
// end::createShader[]

// tag::createProgram[]
GLuint createProgram(const std::vector<GLuint> &shaderList)
{
	GLuint program = glCreateProgram();

	for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glAttachShader(program, shaderList[iLoop]);

	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
		fprintf(stderr, "Linker failure: %s\n", strInfoLog);
		delete[] strInfoLog;
	}

	for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glDetachShader(program, shaderList[iLoop]);

	return program;
}
// end::createProgram[]

// tag::initializeProgram[]
void initializeProgram()
{
	std::vector<GLuint> shaderList;

	shaderList.push_back(createShader(GL_VERTEX_SHADER, loadShader("../vertexShader.glsl")));
	shaderList.push_back(createShader(GL_FRAGMENT_SHADER, loadShader("../fragmentShader.glsl")));

	theProgram = createProgram(shaderList);
	if (theProgram == 0)
	{
		cerr << "GLSL program creation error." << std::endl;
		SDL_Quit();
		exit(1);
	}
	else {
		cout << "GLSL program creation OK! GLUint is: " << theProgram << std::endl;
	}

	// tag::glGetAttribLocation[]
	positionLocation = glGetAttribLocation(theProgram, "position");
	vertexColorLocation = glGetAttribLocation(theProgram, "vertexColor");
	// end::glGetAttribLocation[]

	// tag::glGetUniformLocation[]
	modelMatrixLocation = glGetUniformLocation(theProgram, "modelMatrix");
	viewMatrixLocation = glGetUniformLocation(theProgram, "viewMatrix");
	projectionMatrixLocation = glGetUniformLocation(theProgram, "projectionMatrix");

	//only generates runtime code in debug mode
	SDL_assert_release(modelMatrixLocation != -1);
	SDL_assert_release(viewMatrixLocation != -1);
	SDL_assert_release(projectionMatrixLocation != -1);
	// end::glGetUniformLocation[]

	//clean up shaders (we don't need them anymore as they are no in theProgram
	for_each(shaderList.begin(), shaderList.end(), glDeleteShader);
}
// end::initializeProgram[]

// tag::initializeVertexArrayObject[]
//setup a GL object (a VertexArrayObject) that stores how to access data and from where
void initializeVertexArrayObject(int index)
{
	glGenVertexArrays(1, &vertexArrayObject[index]); //create a Vertex Array Object
	cout << "Vertex Array Object created OK! GLUint is: " << vertexArrayObject << std::endl;
	glBindVertexArray(vertexArrayObject[index]); //make the just created vertexArrayObject the active one
	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObject[index]); //bind vertexDataBufferObject

	glEnableVertexAttribArray(positionLocation); //enable attribute at index positionLocation
	glEnableVertexAttribArray(vertexColorLocation); //enable attribute at index vertexColorLocation

	// tag::glVertexAttribPointer[]
	glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, (6 * sizeof(GL_FLOAT)), (GLvoid *)(0 * sizeof(GLfloat))); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation
	glVertexAttribPointer(vertexColorLocation, 3, GL_FLOAT, GL_FALSE, (6 * sizeof(GL_FLOAT)), (GLvoid *)(3 * sizeof(GLfloat))); //specify that position data contains four floats per vertex, and goes into attribute index vertexColorLocation
	//glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0); //(4 * sizeof(GL_FLOAT)), (GLvoid *)(0 * sizeof(GLfloat))); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation
	
	// end::glVertexAttribPointer[]

	glBindVertexArray(0); //unbind the vertexArrayObject so we can't change it
						 
	glDisableVertexAttribArray(positionLocation); //disable vertex attribute at index positionLocation
	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind array buffer

}
// end::initializeVertexArrayObject[]

// tag::initializeVertexBuffer[]
void initializeVertexBuffer(int index)
{
	glGenBuffers(1, &vertexDataBufferObject[index]);
	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObject[index]);
	glBufferData(GL_ARRAY_BUFFER, PlayerPosition[index].size() *sizeof(GLfloat), &PlayerPosition[index][0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cout << "vertexDataBufferObject created OK! GLUint is: " << vertexDataBufferObject << std::endl;
	
	initializeVertexArrayObject(index);
}
// end::initializeVertexBuffer[]

int const binsize = 45;
float rangeMax = 2400;
float rangeMin = -2400;
float length = 0;
int DroppedIndex;
// X= 2200 Y = 2000

std::vector<GLfloat> DivideRange(int min, int max, int size)
{
	std::vector<GLfloat> Bins;

	length = (max - min + 1) / size;

	for (unsigned int i = 0; i < size; i++)
	{
		//Bins[i] = min + length*i;
		float temp = min + (length * i);
		Bins.push_back(temp);
	}
	Bins.push_back(max);

	return Bins;
}

std::vector<GLfloat> HeatMapColours(int Value)
{
	std::vector<GLfloat> RGBValue;

	if (Value == Colours[0]) { RGBValue.push_back(0.0); RGBValue.push_back(0.0); RGBValue.push_back(0.0); }

	if (Value > Colours[0] && Value < Colours[1]) { RGBValue.push_back(1.0); RGBValue.push_back(1.0); RGBValue.push_back(0.0); }
	
	if (Value >= Colours[1] && Value < Colours[2]) { RGBValue.push_back(1.0); RGBValue.push_back(0.5); RGBValue.push_back(0.0); }

	if (Value >= Colours[2] && Value < Colours[3]) { RGBValue.push_back(0.5); RGBValue.push_back(0.0); RGBValue.push_back(0.0); }

	if (Value >= Colours[3] && Value < Colours[4]) { RGBValue.push_back(1.0); RGBValue.push_back(0.0); RGBValue.push_back(0.0); }

	if (Value >= Colours[4]) { RGBValue.push_back(1.0); RGBValue.push_back(1.0); RGBValue.push_back(1.0); }

	return RGBValue;
}

std::vector<GLfloat> TrajectoryColours(int Value)
{
	std::vector<GLfloat> RGBValue;

	if (Value == 0) { RGBValue.push_back(0.0); RGBValue.push_back(0.0); RGBValue.push_back(0.0); }
	if (Value == 1) { RGBValue.push_back(1.0); RGBValue.push_back(0.0); RGBValue.push_back(0.0); }
	if (Value == 2) { RGBValue.push_back(0.0); RGBValue.push_back(2.0); RGBValue.push_back(0.0); }
	if (Value == 3) { RGBValue.push_back(0.0); RGBValue.push_back(0.0); RGBValue.push_back(3.0); }
	if (Value == 4) { RGBValue.push_back(0.0); RGBValue.push_back(0.0); RGBValue.push_back(0.0); }
	if (Value == 5) { RGBValue.push_back(0.0); RGBValue.push_back(0.0); RGBValue.push_back(0.0); }

	return RGBValue;
}


std::vector<GLfloat> CalculateCount(const string filePath)
{
	std::vector<GLfloat> PlayerData = loadHeatMapData(filePath);
	std::vector<GLfloat> FinalCount;

	std::vector<GLfloat> RangesY = DivideRange(2400, -2400, binsize);
	std::vector<GLfloat> Ranges = DivideRange(rangeMin, rangeMax, binsize);

	float Count[binsize * binsize];
	for (unsigned int i = 0; i < (binsize * binsize); i++)
	{
		Count[i] = 0;
	}

	//read through data and increase count if found in correct bin
	for (unsigned int j = 0; j < PlayerData.size(); j++)
	{
		int X_Cell = 0;
		int Y_Cell = 0;
		//loop Bins along the X axis
		for (unsigned int i = 0; i < binsize; i++)
		{
			//X data within Bin
			if (PlayerData[j] >= Ranges[i] && PlayerData[j] < Ranges[i + 1])
			{
				//the cell the number was found in along X
				X_Cell = i;
				//loop through Bins on Y axis
				for (unsigned int i = 0; i < binsize; i++)
				{
					//Y data within Bin
					if (PlayerData[j + 1] <= RangesY[i] && PlayerData[j + 1] > RangesY[i + 1])
					{
						//The cell the number was found along in the Y
						Y_Cell = i;
						int cell = (binsize * Y_Cell) + X_Cell;
						Count[cell] += 1;
					}
				}
			}
		}
		//skip over Y and Z values and move onto next position data
		j = j + 2;
	}
	for (unsigned int i = 0; i < (binsize * binsize); i++)
	{
		FinalCount.push_back(Count[i]);
	}
	return FinalCount;
}

std::vector<GLfloat> CreateHeatmap(std::vector<GLfloat> Count)
{
	std::vector<GLfloat> GridData;

	for (unsigned int Rows = 0; Rows < binsize; Rows++)
	{
		for (unsigned int Collums = 0; Collums < binsize; Collums++)
		{
			//find Cell
			int Cell = (binsize * Rows) + Collums;
			//Set Z value
			float Z = -0.25;
			//Generate Colours based on the count
			std::vector<GLfloat> RGB = HeatMapColours(Count[Cell]);

			//First Triangle
			//First Point    X	Y	Z
			GridData.push_back((rangeMin + (Collums * length)) / 1000);			GridData.push_back((rangeMax - (Rows * length)) / 1000);		GridData.push_back(Z);
			GridData.push_back(RGB[0]); GridData.push_back(RGB[1]); GridData.push_back(RGB[2]);
			//Second Point	X	Y	Z
			GridData.push_back((rangeMin + ((Collums + 1) * length)) / 1000);	GridData.push_back((rangeMax - (Rows * length)) / 1000);		GridData.push_back(Z);
			GridData.push_back(RGB[0]); GridData.push_back(RGB[1]); GridData.push_back(RGB[2]);
			//Third Point	X	Y	Z
			GridData.push_back((rangeMin + (Collums * length)) / 1000);			GridData.push_back((rangeMax - ((Rows + 1) * length)) / 1000);	GridData.push_back(Z);
			GridData.push_back(RGB[0]); GridData.push_back(RGB[1]); GridData.push_back(RGB[2]);

			//Second Triangle
			//First Point    X	Y	Z
			GridData.push_back((rangeMin + ((Collums + 1) * length)) / 1000);	GridData.push_back((rangeMax - (Rows * length)) / 1000);		GridData.push_back(Z);
			GridData.push_back(RGB[0]); GridData.push_back(RGB[1]); GridData.push_back(RGB[2]);
			//Second Point	X	Y	Z
			GridData.push_back((rangeMin + (Collums * length)) / 1000);			GridData.push_back((rangeMax - ((Rows + 1) * length)) / 1000);	GridData.push_back(Z);
			GridData.push_back(RGB[0]); GridData.push_back(RGB[1]); GridData.push_back(RGB[2]);
			//Third Point	X	Y	Z
			GridData.push_back((rangeMin + ((Collums + 1) * length)) / 1000);	GridData.push_back((rangeMax - ((Rows + 1) * length)) / 1000);	GridData.push_back(Z);
			GridData.push_back(RGB[0]); GridData.push_back(RGB[1]); GridData.push_back(RGB[2]);
		}
	}
	return GridData;

	// Workings for creating a Plane
	//				X						Y						Z
	//	(rangeMin + (collum * Length)) , rangeMax - (Row * Length), Z )			(rangeMin + ((collum + 1) * Length)) , rangeMax - (Row * Length), Z )
	//
	//	(rangeMin + (collum * Length)) , rangeMax - ((Row + 1) * Length), Z )

	//																			(rangeMin + ((collum + 1) * Length)) , rangeMax - (Row * Length), Z )
	//
	//	(rangeMin + (collum * Length)) , rangeMax - ((Row + 1) * Length), Z )	(rangeMin + ((collum + 1) * Length)) , rangeMax - ((Row + 1) * Length), Z )
	//	 MAKE SURE TO SCALE ALL POSITION VALUES DOWN
}


std::vector<GLfloat> AggregateHeatMap(int count)
{
	//add all the Count Arrays into one then use that the draw a heatmap
	std::vector<GLfloat> CombinedCount;
	std::vector<GLfloat> One = Count[0];
	std::vector<GLfloat> Two = Count[1];
	std::vector<GLfloat> Three = Count[2];
	std::vector<GLfloat> Four = Count[3];
	std::vector<GLfloat> Five = Count[4];

	//if size == 0 dont use

	for (unsigned int i = 0; i < (binsize * binsize); i++)
	{
		GLfloat temp;
		switch (count)
		{
			case 1: temp = (One[i]);break;
			case 2: temp = (One[i] + Two[i]); break;
			case 3: temp = (One[i] + Two[i] + Three[i]); break;
			case 4: temp = (One[i] + Two[i] + Three[i] + Four[i]); break;
			case 5: temp = (One[i] + Two[i] + Three[i] + Four[i] + Five[i]); break;
		}
		CombinedCount.push_back(temp);
	}

	std::vector<GLfloat> Heatmap;
	for (unsigned int Rows = 0; Rows < binsize; Rows++)
	{
		for (unsigned int Collums = 0; Collums < binsize; Collums++)
		{
			//find Cell
			int Cell = (binsize * Rows) + Collums;
			//Set Z value
			float Z = -0.25;
			//Generate Colours based on the count
			std::vector<GLfloat> RGB = HeatMapColours(CombinedCount[Cell]);

			//First Triangle
			//First Point    X	Y	Z
			Heatmap.push_back((rangeMin + (Collums * length)) / 1000);			Heatmap.push_back((rangeMax - (Rows * length)) / 1000);		Heatmap.push_back(Z);
			Heatmap.push_back(RGB[0]); Heatmap.push_back(RGB[1]); Heatmap.push_back(RGB[2]);
			//Second Point	X	Y	Z
			Heatmap.push_back((rangeMin + ((Collums + 1) * length)) / 1000);	Heatmap.push_back((rangeMax - (Rows * length)) / 1000);		Heatmap.push_back(Z);
			Heatmap.push_back(RGB[0]); Heatmap.push_back(RGB[1]); Heatmap.push_back(RGB[2]);
			//Third Point	X	Y	Z
			Heatmap.push_back((rangeMin + (Collums * length)) / 1000);			Heatmap.push_back((rangeMax - ((Rows + 1) * length)) / 1000);	Heatmap.push_back(Z);
			Heatmap.push_back(RGB[0]); Heatmap.push_back(RGB[1]); Heatmap.push_back(RGB[2]);

			//Second Triangle
			//First Point    X	Y	Z
			Heatmap.push_back((rangeMin + ((Collums + 1) * length)) / 1000);	Heatmap.push_back((rangeMax - (Rows * length)) / 1000);		Heatmap.push_back(Z);
			Heatmap.push_back(RGB[0]); Heatmap.push_back(RGB[1]); Heatmap.push_back(RGB[2]);
			//Second Point	X	Y	Z
			Heatmap.push_back((rangeMin + (Collums * length)) / 1000);			Heatmap.push_back((rangeMax - ((Rows + 1) * length)) / 1000);	Heatmap.push_back(Z);
			Heatmap.push_back(RGB[0]); Heatmap.push_back(RGB[1]); Heatmap.push_back(RGB[2]);
			//Third Point	X	Y	Z
			Heatmap.push_back((rangeMin + ((Collums + 1) * length)) / 1000);	Heatmap.push_back((rangeMax - ((Rows + 1) * length)) / 1000);	Heatmap.push_back(Z);
			Heatmap.push_back(RGB[0]); Heatmap.push_back(RGB[1]); Heatmap.push_back(RGB[2]);
		}
	}
	return Heatmap;
}

// tag::loadAssets[]
void loadAssets()
{
	initializeProgram(); //create GLSL Shaders, link into a GLSL program, and get IDs of attributes and variables
	Colours = DivideRange(0, 15, 5);
	cout << "Loaded Assets OK!\n";
}
// end::loadAssets[]

bool W, A, S, D, Q, E, UP, DOWN, F, Eight, Two, Six, Four = false;
bool Togglefile[10];
string getFileExt(const string& s) {

	size_t i = s.rfind('.', s.length());
	if (i != string::npos) {
		return(s.substr(i + 1, s.length() - i));
	}
	return("");
}
int HeatMapAgg = 0;
int const filesHeld = 5;
// tag::handleInput[]
void handleInput()
{
	//Event-based input handling
	//The underlying OS is event-based, so **each** key-up or key-down (for example)
	//generates an event.
	//  - https://wiki.libsdl.org/SDL_PollEvent
	//In some scenarios we want to catch **ALL** the events, not just to present state
	//  - for instance, if taking keyboard input the user might key-down two keys during a frame
	//    - we want to catch based, and know the order
	//  - or the user might key-down and key-up the same within a frame, and we still want something to happen (e.g. jump)
	//  - the alternative is to Poll the current state with SDL_GetKeyboardState

	SDL_Event event; //somewhere to store an event
	char* dropped_filedir;
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
					 //NOTE: there may be multiple events per frame
	while (SDL_PollEvent(&event)) //loop until SDL_PollEvent returns 0 (meaning no more events)
	{
		switch (event.type)
		{
		case SDL_QUIT:
			done = true; //set donecreate remote branch flag if SDL wants to quit (i.e. if the OS has triggered a close event,
						 //  - such as window close, or SIGINT
			break;

		case (SDL_DROPFILE) : {      // In case if dropped file
			dropped_filedir = event.drop.file;
			//string of file name
			std::string file(dropped_filedir);
			//check of file extenstion
			string exstension = getFileExt(dropped_filedir);
			string pos = "Position";
			//if file type text
			if (exstension == "txt")
			{
				// Shows directory of dropped file
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"Suitable Text dropped on window",dropped_filedir,win);

				//checks to find if the text contains the right title
				//if ((file.find(pos) != std::string::npos) && playerFiles == 0)
				if (file.find(pos) != std::string::npos)
				{
					if (DroppedIndex >= filesHeld)
					{
						DroppedIndex = 0;
					}

					//trajectory
					std::vector<GLfloat> RGB = TrajectoryColours(DroppedIndex);
					PlayerPosition[DroppedIndex] = loadLog(file,1000, RGB);
					Count[DroppedIndex] = CalculateCount(file);
					PlayerPosition[DroppedIndex + 5] = CreateHeatmap(Count[DroppedIndex]);
				
					initializeVertexBuffer(DroppedIndex); //load data into a vertex buffer
					initializeVertexBuffer(DroppedIndex + 5); //load data into a vertex buffer
					for (int i = 0; i < 10; i++)
					{
						Togglefile[i] = false;
					}
					HeatMapAgg = 0;
					Togglefile[DroppedIndex] = true;
					Togglefile[DroppedIndex + 5] = true;
					++DroppedIndex;				
				}
			}
			else
			{
				// Shows directory of dropped file
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Un-suitable file type dropped on window", dropped_filedir, win);
			}
			SDL_free(dropped_filedir);    // Free dropped_filedir memory
			break;
		}

			//keydown handling - we should to the opposite on key-up for direction controls (generally)
		case SDL_KEYDOWN:
			//Keydown can fire repeatable if key-repeat is on.
			//  - the repeat flag is set on the keyboard event, if this is a repeat event
			//  - in our case, we're going to ignore repeat events
			//  - https://wiki.libsdl.org/SDL_KeyboardEvent
			if (!event.key.repeat)
				switch (event.key.keysym.sym)
				{
					//hit escape to exit
				case SDLK_ESCAPE: done = true;

				//Camera Controls
				//Back to default
				case SDLK_SPACE:
					Cam1 = { 0.0f, 0.0f, 5.0f };
					Cam2 = { 0.0f, 0.0f, 0.0f };
					Cam3 = { 0.0f, 0.000001f, 0.0f };
					break;
					//Decrease X axis
				case SDLK_a:	A = true;	break;
					//Increase X axis
				case SDLK_d:	D = true;	break;
					//Decrease Y axis
				case SDLK_e:	E = true;	break;
					//Increase Y axis
				case SDLK_q:	Q = true;	break;
					//Decrease Z axis
				case SDLK_w:	W = true;	break;
					//Increase Z axis
				case SDLK_s:	S = true;	break;

				//Trajectorys
				case SDLK_1: Togglefile[0] = !Togglefile[0];	break;
				case SDLK_2: Togglefile[1] = !Togglefile[1];	break;
				case SDLK_3: Togglefile[2] = !Togglefile[2];	break;
				case SDLK_4: Togglefile[3] = !Togglefile[3];	break;
				case SDLK_5: Togglefile[4] = !Togglefile[4];	break;
				//Heatmaps
				case SDLK_6: Togglefile[5] = !Togglefile[5];	break;
				case SDLK_7: Togglefile[6] = !Togglefile[6];	break;
				case SDLK_8: Togglefile[7] = !Togglefile[7];	break;
				case SDLK_9: Togglefile[8] = !Togglefile[8];	break;
				case SDLK_0: Togglefile[9] = !Togglefile[9];	break;

				case SDLK_MINUS: 
					if (HeatMapAgg > 0)
					{
						HeatMapAgg--;

						PlayerPosition[10] = AggregateHeatMap(HeatMapAgg);
						initializeVertexBuffer(10); //load data into a vertex buffer
					}
					break;
				case SDLK_EQUALS:
					if (HeatMapAgg < filesHeld && HeatMapAgg <= DroppedIndex)
					{
						HeatMapAgg++;

						PlayerPosition[10] = AggregateHeatMap(HeatMapAgg);
						initializeVertexBuffer(10); //load data into a vertex buffer
					}
					
					break;

				case SDLK_KP_8:	Eight = true;	break;
				case SDLK_KP_6:	Six = true;		break;
				case SDLK_KP_4:	Four = true;	break;
				case SDLK_KP_2:	Two = true;		break;

				}	
			break;
		case SDL_KEYUP:
			switch (event.key.keysym.sym)
			{
				case SDLK_w:	W = false;		break;
				case SDLK_s:	S = false;		break;
				case SDLK_a:	A = false;		break;
				case SDLK_d:	D = false;		break;
				case SDLK_q:	Q = false;		break;
				case SDLK_e:	E = false;		break;
				case SDLK_KP_8:	Eight = false;	break;
				case SDLK_KP_6:	Six = false;	break;
				case SDLK_KP_4:	Four = false;	break;
				case SDLK_KP_2:	Two = false;	break;
			}
			break;


		default: //good practice to always have a default case
			break;
		}
	}
}
// end::handleInput[]

// tag::updateSimulation[]
void updateSimulation(double simLength = 0.02) //update simulation with an amount of time to simulate for (in seconds)
{
	//WARNING - we should calculate an appropriate amount of time to simulate - not always use a constant amount of time
	// see, for example, http://headerphile.blogspot.co.uk/2014/07/part-9-no-more-delays.html
	//Moving camera
	if (W == true)
		Cam1[2] -= 0.05;
	if (S == true)
		Cam1[2] += 0.05;
	if (A == true)
		Cam1[0] -= 0.05;
	if (D == true)
		Cam1[0] += 0.05;
	if (Q == true)
		Cam1[1] -= 0.05;
	if (E == true)
		Cam1[1] += 0.05;
	if (Eight == true && Cam2[1] < 1)
		Cam2[1] += 0.05;
	if (Two == true && Cam2[1] > -1)
		Cam2[1] -= 0.05;
	if (Four == true && Cam2[0] > -1)
		Cam2[0] -= 0.05;
	if (Six == true && Cam2[0] < 1)
		Cam2[0] += 0.05;
}
// end::updateSimulation[]

// tag::preRender[]
void preRender()
{
	glViewport(0, 0, 900, 800); //set viewpoint
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); //set bg colour
	glClear(GL_COLOR_BUFFER_BIT); //clear the window (technical the scissor box bounds)
}
// end::preRender[]

// tag::render[]
void render()
{
	glUseProgram(theProgram); //installs the program object specified by program as part of current rendering state

	//set projectionMatrix - how we go from 2D to 3D.............................................
	glUniformMatrix4fv(projectionMatrixLocation, 1, false, glm::value_ptr(projectionMatrix));
	//set viewMatrix - how we control the view (viewpoint, view direction, etc)
	glUniformMatrix4fv(viewMatrixLocation, 1, false, glm::value_ptr(viewMatrix));

	//Depth testing
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Do 3d Drawing
	//-------------------------------
	//change the line width from 1 to...
	glLineWidth(2);
	//drawing for all data

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_CONSTANT_ALPHA);
	//drawing induvidial heatmaps
	for (int i = 5; i < 10; i++)
	{
		if (Togglefile[i] == true)
		{
			glBindVertexArray(vertexArrayObject[i]);
			glUniformMatrix4fv(modelMatrixLocation, 1, false, glm::value_ptr(modelMatrix));
			glDrawArrays(GL_TRIANGLES, 0, PlayerPosition[i].size());
		}
	}

	//Draw Aggrigated heatmap here
	if (HeatMapAgg != 0)
	{
		glBindVertexArray(vertexArrayObject[10]);
		glUniformMatrix4fv(modelMatrixLocation, 1, false, glm::value_ptr(modelMatrix));
		glDrawArrays(GL_TRIANGLES, 0, PlayerPosition[10].size());
	}

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	for (int i = 0; i < 5; i++)
	{
		if (Togglefile[i] == true)
		{
			glBindVertexArray(vertexArrayObject[i]);
			glUniformMatrix4fv(modelMatrixLocation, 1, false, glm::value_ptr(modelMatrix));
			glDrawArrays(GL_LINES, 0, PlayerPosition[i].size());
		}
	}


	//set projectionMatrix - how we go from 3D to 2D
	glUniformMatrix4fv(projectionMatrixLocation, 1, false, glm::value_ptr(glm::mat4(1.0)));
	//set viewMatrix - how we control the view (viewpoint, view direction, etc)
	glUniformMatrix4fv(viewMatrixLocation, 1, false, glm::value_ptr(glm::mat4(1.0f)));

	//Do 2D Drawing
	//---------------------------------------------

	glBindVertexArray(0);//unbind the vertex data to be neat
	glUseProgram(0); //clean  up
}
// end::render[]

// tag::postRender[]
void postRender()
{
	SDL_GL_SwapWindow(win);; //present the frame buffer to the display (swapBuffers)
	frameLine += "Frame: " + std::to_string(frameCount++);
	cout << "\r" << frameLine << std::flush;
	frameLine = "";
}
// end::postRender[]

// tag::cleanUp[]
void cleanUp()
{
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);
	cout << "Cleaning up OK!\n";
}
// end::cleanUp[]

void camera()
{
	viewMatrix = glm::lookAt(Cam1, Cam2, Cam3);

	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));
	projectionMatrix = glm::perspective(glm::radians(45.0f), 900.0f / 800.0f, 0.1f, 150.0f);
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

}

// tag::main[]
int main(int argc, char* args[])
{
	exeName = args[0];
	//setup
	//- do just once
	initialise();
	createWindow();

	createContext();

	initGlew();

	glViewport(0, 0, 900, 800); //should check what the actual window res is?
	
	SDL_GL_SwapWindow(win); //force a swap, to make the trace clearer

	loadAssets();

	while (!done) //loop until done flag is set)
	{
		handleInput(); // this should ONLY SET VARIABLES

		updateSimulation(); // this should ONLY SET VARIABLES according to simulation

		preRender();

		//Setup camera
		camera();

		render(); // this should render the world state according to VARIABLES -

		postRender();

	}

	//cleanup and exit
	cleanUp();
	SDL_Quit();

	return 0;
}
// end::main[]
