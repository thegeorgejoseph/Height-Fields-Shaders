/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: georgejo@usc.edu | 8486068836
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <vector>
#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

void displayRendering();
void createModelViewMatrix();
void createProjectionMatrix();
int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not
int flag = 1;
typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = TRANSLATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

typedef enum { POINTS, LINES, WIREFRAME, SMOOTH } MODE_STATE;
MODE_STATE render = POINTS;

int smoothen ;

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;

//initialising VBO, EBO, VAO
unsigned int ElementBufferObject, VertexArrayObject, VertexBufferObject, NeighbourVertexBufferObject;
int vertices, indices;
int c = 0;
int height,width;
OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;


int h_modelViewMatrix, h_projectionMatrix;
unsigned int positionLocation, colorLocation, neighbourLocation;

int globalVertices;

float pixelScale = 256.0f;
float grayscaleColor;

int num_screenshots = 0,ssNum = 0;
// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void displayFunc()
{
  // render some stuff...
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_PRIMITIVE_RESTART);
  glPrimitiveRestartIndex(globalVertices);
  
  pipelineProgram->Bind();
  glBindVertexArray(VertexArrayObject);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBufferObject);
  glBindBuffer(GL_ARRAY_BUFFER, VertexBufferObject);
  
  glEnableVertexAttribArray(positionLocation);
  glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 3* sizeof(float), (void*) 0);
  glEnableVertexAttribArray(colorLocation);
  glVertexAttribPointer(colorLocation, 4, GL_FLOAT, GL_FALSE, 4* sizeof(float), (void*) (3 * vertices * sizeof(float)));
  if (smoothen == 1){
    glBindBuffer(GL_ARRAY_BUFFER, NeighbourVertexBufferObject);
    glEnableVertexAttribArray(neighbourLocation);
    glVertexAttribPointer(neighbourLocation, 4, GL_FLOAT, GL_FALSE, 4 *  sizeof(float), (void*) 0);
  }
  
  createModelViewMatrix();
  createProjectionMatrix();
  
  pipelineProgram->SetSmoothening(smoothen);

  displayRendering();
  
  glBindVertexArray(0);
  glutSwapBuffers();
}

void idleFunc()
{

    if (num_screenshots < 75){
      render = SMOOTH;
      if (num_screenshots < 19){
        landTranslate[0] += 0.01f;
        landScale[0] += 0.01f;
      } else if (num_screenshots < 38){
        landTranslate[1] -= 0.01f;
        landScale[0] -= 0.01f;
      } else if (num_screenshots < 57){
        landTranslate[2] += 0.01f;
        landScale[2] += 0.05f;
      } else{
        landTranslate[2] -= 0.01f;
        landScale[2] -= 0.05f; 
      }
     } else if (num_screenshots < 150){
      render = POINTS;
      if (num_screenshots < 94){
        landScale[1] += 0.03f;
        landTranslate[1] += 0.01f;
      } else if (num_screenshots < 113){
        landTranslate[0] -= 0.01f;
        landScale[1] -= 0.03f;
      } else if (num_screenshots < 132){
        landTranslate[0] += 0.01f;
        landScale[2] += 0.3f;
      } else{
       landTranslate[1] -= 0.03f;
       landScale[2] -= 0.3f;
      } 
    } else if (num_screenshots < 225) {
      render = LINES;
      if (num_screenshots < 169){
        landScale[1] += 0.03f;
        landTranslate[2] += 0.01f;
      } else if (num_screenshots < 188){
        landScale[1] -= 0.03f;
        landTranslate[2] -= 0.01f;  
      } else if (num_screenshots < 207){
        landScale[0] += 0.03f; 
        landTranslate[1] -= 0.01f;
      } else{
        landScale[0] -= 0.03f; 
        landTranslate[1] += 0.01f;
      } 
    } else if(num_screenshots <=300){
      if (num_screenshots < 244){
        landTranslate[0] += 0.01f;
        landScale[2] += 0.02f;
        if (num_screenshots %2 == 0){
          render = WIREFRAME;
        }
        else{
          render = LINES;
        }
      } else if (num_screenshots < 263){
        landTranslate[1] += 0.01f;
        landScale[2] -= 0.02f;
        render = WIREFRAME;
      } else if (num_screenshots < 282){
        landTranslate[0] -= 0.01f;
        landRotate[2] += (1.0 * 360)/19;
        if (num_screenshots %2 == 0){
          render = POINTS;
        }
        else{
          render = WIREFRAME;
        }
      } else{
        landTranslate[1] -= 0.01f;
        landRotate[2] -= (1.0 * 360) / 18;
        if (num_screenshots %2 == 0){
          render = WIREFRAME;
        }
        else{
          render = SMOOTH;
        } 
      } 
    }

    if (num_screenshots == 301){
      landTranslate[0] = 0.0f;
      landTranslate[1] = 0.0f;
      landTranslate[2] = 0.0f;
      landRotate[0] = 0.0f;
      landRotate[1] = 0.0f;
      landRotate[2] = 0.0f;
      landScale[0] = 2.0f;
      landScale[1] = 2.0f;
      landScale[2] = 2.0f;
      render = POINTS;
    }

    if (num_screenshots <= 300){
    char result[10];
    sprintf(result, "%03d", num_screenshots);
    saveScreenshot(("images/" + std::string(result) + ".jpeg").c_str());
    }
    num_screenshots++; 

  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);
  
  // setup perspective matrix...
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  matrix.Perspective(54.0f, (float)w / (float)h, 0.01f, 1000.0f);
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (rightMouseButton)
      {
        // control z translation via the right mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (rightMouseButton)
      {
        // control z rotation via the right mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (rightMouseButton)
      {
        // control z scaling via the right mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    break;
    
    // change render mode
    case 't':
      controlState = TRANSLATE;
      break;
  
    case 's':
      controlState = SCALE;
      break;
  
    case 'r':
      controlState = ROTATE;
      break;

    case '1':
      render = POINTS;
      smoothen = 0;
      break;
      
    case '2':
      render = LINES;
      smoothen = 0;
      break;
      
    case '3':
      render = WIREFRAME;
      smoothen = 0;
      break;
    
    case '4':
      render = SMOOTH;
      smoothen = 1;
      break;
  }

}

void initScene(int argc, char *argv[]) {
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK) {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  // clearning the screen
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  
  // setting up variables for use
  height = heightmapImage->getHeight();
  width = heightmapImage->getWidth();
  vertices = height * width;
  indices = (height + 1) * (width*2);
  //creating vectors to feed the VBOs
  vector<float> positionVector(3*vertices);
  vector<float> colorVector(4*vertices);
  vector<int> elementPoints(indices);
  vector<float> neighbours(4*vertices);
  int idx;
  //getting all pixels from the image
  unsigned char *pixels = heightmapImage->getPixels();
  //getting max height from the pixels from start pointer to end
  float maxHeight = *max_element(pixels, pixels + vertices);
  //scaling factor to scale the height of the heightfield
  float heightScaler = 1 / (5 * maxHeight);

    // creating positionVector by mapping the height of each pixel with z-axis set to 0 scaled to fit the view
    for (int i = 0; i < width; i++){
      idx = 0;
    for (int j = 0; j < height; j++){
        positionVector[(j * width + i) * 3] = ((float) i / width) * 2;
        positionVector[(j * width + i) * 3 + 1] = heightmapImage->getPixel(i, j, 0) * heightScaler * 2;
        positionVector[(j * width + i) * 3 + 2] = ((float) j / height ) * 2;
    } 
    }
    //creating the neighbours vector that stores the up,down,left and right points in vec4 form
    for (int i = 0; i< width; i++){
    for (int j = 0; j < height; j++){
      float left,right,up,down;
      if (i == 0){
        up = positionVector[(j * width + (i+1)) * 3 + 1];
      }
      else{
       up = positionVector[(j * width + (i-1)) * 3 + 1]; 
      }

      if (j == 0){
        left = positionVector[((j+1) * width + (i)) * 3 + 1];
      }
      else{
       left = positionVector[((j-1) * width + (i)) * 3 + 1]; 
      }

      if (i == width - 1){
       down = positionVector[(j * width + (i-1)) * 3 + 1];  
      }
      else{
       down = positionVector[(j * width + (i+1)) * 3 + 1];  
      }

      if (j == height - 1){
        right = positionVector[((j-1) * width + (i)) * 3 + 1];
      }
      else{
       right = positionVector[((j+1) * width + (i)) * 3 + 1]; 
      }

      neighbours[(j * width + i) * 4] = up;
      neighbours[(j * width + i) * 4  + 1] = down;
      neighbours[(j * width + i) * 4  + 2] = left;
      neighbours[(j * width + i) * 4  + 3] = right;
    }
  }  
    // creating the color vector that generates the required coloring for the points
    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height; j++) {
        grayscaleColor = heightmapImage->getPixel(i, j, 0) / pixelScale; 
        colorVector[(j * width + i) * 4] = grayscaleColor ;
        colorVector[(j * width + i) * 4 + 1] = grayscaleColor ;
        colorVector[(j * width + i) * 4 + 2] = grayscaleColor + 255;
        colorVector[(j * width + i) * 4 + 3] = 1;
      }
    }

  // creating a element points vector that is used to populate the element buffer object
  // temp will store all the element buffer points for every two rows
  // it is populated by looking at two rows at a time and placing the first row of the two in odd positions of the temp and the other row in even positions
  int pIdx = 0;
  int temp[width*2+1];
  for (int r = 0; r < height - 1; r++){
    int colIdx = 0;
    for (int c = 0; c < width; c++){
       temp[colIdx] = r * width + c;
       colIdx += 2;
    }
    temp[colIdx] = vertices;
    colIdx = 1;
    for (int c = 0; c < width; c++){
      temp[colIdx] = (r + 1) * width + c;
      colIdx += 2;
    }
    for (int i = 0; i < (width * 2 + 1); i++){
      elementPoints[pIdx] = temp[i];
      pIdx++;
    }
  }

  globalVertices = vertices;
  
  // generating and binding the vertex buffer object, adding the position and color vectors as subdata in the vbo
  glGenBuffers(1, &VertexBufferObject);
  glBindBuffer(GL_ARRAY_BUFFER, VertexBufferObject);
  glBufferData(GL_ARRAY_BUFFER, 3 * vertices * sizeof(float) + 4 * vertices * sizeof(float), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * vertices * sizeof(float), &positionVector[0]);
  glBufferSubData(GL_ARRAY_BUFFER, 3 * vertices * sizeof(float), 4 * vertices * sizeof(float), &colorVector[0]);
  // generating, binding and adding the neighbours vector into another vbo
  glGenBuffers(1, &NeighbourVertexBufferObject);
  glBindBuffer(GL_ARRAY_BUFFER, NeighbourVertexBufferObject);
  glBufferData(GL_ARRAY_BUFFER, 4 * vertices * sizeof(float) , &neighbours[0], GL_STATIC_DRAW);

  // init the program
  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  // generating and binding the VAO
  glGenVertexArrays(1, &VertexArrayObject);
  glBindVertexArray(VertexArrayObject);

  
  positionLocation = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  colorLocation = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  neighbourLocation = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "neighbour"); 

  //binding the EBO
  glGenBuffers(1, &ElementBufferObject);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBufferObject);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,  (height - 1) * (width*2 + 1) * sizeof(int), &elementPoints[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  //passing handles to model view matrix and projection matrix
  h_modelViewMatrix = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "modelViewMatrix");
  h_projectionMatrix = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "projectionMatrix");



  glEnable(GL_DEPTH_TEST);

  std::cout << "GL error: " << glGetError() << std::endl;
  
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}

//function to render the different modes
void displayRendering()
{
  switch (render)
  {
    case LINES:
      glDrawElements(GL_LINE_STRIP, indices, GL_UNSIGNED_INT, 0);
      break;
    case WIREFRAME:
      glDrawElements(GL_TRIANGLE_STRIP, indices, GL_UNSIGNED_INT, 0);
      break;
    case POINTS:
      glDrawElements(GL_POINTS, indices, GL_UNSIGNED_INT, 0);
      break;
    case SMOOTH:
      glDrawElements(GL_TRIANGLE_STRIP, indices, GL_UNSIGNED_INT, 0);
        break; 
    
  }
}

// creating Model View Matrix
void createModelViewMatrix(){
  float m[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  matrix.Rotate(landRotate[0], 1, 0, 0);
  matrix.Rotate(landRotate[1], 0, 1, 0);
  matrix.Rotate(landRotate[2], 0, 0, 1);
  matrix.Scale(landScale[0], landScale[1], landScale[2]);
  matrix.LookAt(3, 1.5, 2, 0.5, 0, 0.5, 0, 1, 0);
  // matrix.LookAt();
  matrix.GetMatrix(m);
  glUniformMatrix4fv(h_modelViewMatrix, 1, GL_FALSE, m);
}

// creating Projection Matrix
void createProjectionMatrix(){
  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);
  glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, p);
}