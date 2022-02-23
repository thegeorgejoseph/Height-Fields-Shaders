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
void populateHeightFieldPointsAndColors(int m, int n, int i);
int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = TRANSLATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

typedef enum { POINTS, LINES, TRIANGLES, SMOOTH } MODE_STATE;
MODE_STATE render = POINTS;

int smoothen ;

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;

//initialising VBO, EBO, VAO
unsigned int PointVertexBufferObject, ColorVertexBufferObject, ElementBufferObject, VertexArrayObject, VertexBufferObject, NeighbourVertexBufferObject;
vector<glm::vec3> heightFieldPoints;
vector<glm::vec4> heightFieldColors;
int vertices, indices, heightFieldIndices;
int c = 0;
int height,width;
OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;


int h_modelViewMatrix, h_projectionMatrix;
unsigned int pos_loc, color_loc, neighbour_loc;

int globalVertices;

float pixelScale = 256.0f;
float grayscaleColor;

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
  
  glEnableVertexAttribArray(pos_loc);
  glVertexAttribPointer(pos_loc, 3, GL_FLOAT, GL_FALSE, 3* sizeof(float), (void*) 0);
  glEnableVertexAttribArray(color_loc);
  glVertexAttribPointer(color_loc, 4, GL_FLOAT, GL_FALSE, 4* sizeof(float), (void*) (3 * vertices * sizeof(float)));
  if (smoothen == 1){
    glBindBuffer(GL_ARRAY_BUFFER, NeighbourVertexBufferObject);
    glEnableVertexAttribArray(neighbour_loc);
    glVertexAttribPointer(neighbour_loc, 4, GL_FLOAT, GL_FALSE, 4 *  sizeof(float), (void*) 0);
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
  // do some stuff... 

  // for example, here, you can save the screenshots to disk (to make the animation)

  // make the screen update 
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
    case '1':
      render = POINTS;
      smoothen = 0;
      break;
      
    case '2':
      render = LINES;
      smoothen = 0;
      break;
      
    case '3':
      render = TRIANGLES;
      smoothen = 0;
      break;
    
    case '4':
      render = SMOOTH;
      smoothen = 1;
      break;
    // glut cannot keep track of the CTRL key for Mac (which is Command on Mac), change the detection method
    case 't':
      controlState = TRANSLATE;
      break;
  
    case 's':
      controlState = SCALE;
      break;
  
    case 'r':
      controlState = ROTATE;
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

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  
  
  height = heightmapImage->getHeight();
  width = heightmapImage->getWidth();
  vertices = height * width;
  heightFieldIndices = (height + 1) * (width*2);
  indices = (height + 1) * (width*2);
  vector<float> pointPos(3*vertices);
  vector<float> pointCol(4*vertices);
  vector<int> pointIdx(indices);
  vector<float> neighbours(4*vertices);
  
  unsigned char *pixels = heightmapImage->getPixels();
  float maxHeight = *max_element(pixels, pixels + vertices);

  
  // float pixelHeight;

  // int m,n;
  // for (int i = 0; i <= height; ++i){
  //   for (int j = 0; j< width; ++j){
  //     if (i % 2 == 1){
  //       n = width - j - 1;
  //     } else{
  //       n = j;
  //     }
  //     if (height - 1 > i){
  //       m = i;
  //     } else{
  //       m = height - 1;
  //     }

  //     pixelHeight = heightmapImage->getPixel(m,n,0);

  //     populateHeightFieldPointsAndColors(m,n,c);
      
  //     c += 1;
  //   }

  // }
  
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      pointPos[(j * width + i) * 3] = (float) i / width;
      pointPos[(j * width + i) * 3 + 1] = heightmapImage->getPixel(i, j, 0) / (5*maxHeight);
      pointPos[(j * width + i) * 3 + 2] = (float) j / height;
      
      grayscaleColor = heightmapImage->getPixel(i, j, 0) / pixelScale; 
      pointCol[(j * width + i) * 4] = grayscaleColor ;
      pointCol[(j * width + i) * 4 + 1] = grayscaleColor ;
      pointCol[(j * width + i) * 4 + 2] = grayscaleColor + 255;
      pointCol[(j * width + i) * 4 + 3] = 1;
    }
  }

  for (int i = 0; i< width; i++){
    for (int j = 0; j < height; j++){
      float left,right,up,down;
      if (i == 0){
        up = pointPos[(j * width + (i+1)) * 3 + 1];
      }
      else{
       up = pointPos[(j * width + (i-1)) * 3 + 1]; 
      }

      if (j == 0){
        left = pointPos[((j+1) * width + (i)) * 3 + 1];
      }
      else{
       left = pointPos[((j-1) * width + (i)) * 3 + 1]; 
      }

      if (i == width - 1){
       down = pointPos[(j * width + (i-1)) * 3 + 1];  
      }
      else{
       down = pointPos[(j * width + (i+1)) * 3 + 1];  
      }

      if (j == height - 1){
        right = pointPos[((j-1) * width + (i)) * 3 + 1];
      }
      else{
       right = pointPos[((j+1) * width + (i)) * 3 + 1]; 
      }

      neighbours[(j * width + i) * 4] = up;
      neighbours[(j * width + i) * 4  + 1] = down;
      neighbours[(j * width + i) * 4  + 2] = left;
      neighbours[(j * width + i) * 4  + 3] = right;
    }
  }  


  // init facets index for triangle
  globalVertices = vertices;
  bool isDown = true;
  for (int row = 0; row < height - 1; row++) {
    pointIdx[row * (width*2 + 1)] = (row) * width;
    for (int col = 1; col < width*2; col++)
    {
      if (isDown)
        pointIdx[row * (width*2 + 1) + col] = pointIdx[row * (width*2 + 1) + col - 1] + width;
      else
        pointIdx[row * (width*2 + 1) + col] = pointIdx[row * (width*2 + 1) + col - 1] - (width - 1);
      
      isDown = !isDown;
    }
    isDown = true;
    pointIdx[row * (width*2 + 1) + width*2] =vertices;
  }
  
  // make the VBO

  glGenBuffers(1, &PointVertexBufferObject);
  glBindBuffer(GL_ARRAY_BUFFER, PointVertexBufferObject);
  glBufferData(GL_ARRAY_BUFFER, heightFieldPoints.size() * sizeof(glm::vec3) , &heightFieldPoints[0], GL_STATIC_DRAW);

  glGenBuffers(1, &ColorVertexBufferObject);
  glBindBuffer(GL_ARRAY_BUFFER, ColorVertexBufferObject);
  glBufferData(GL_ARRAY_BUFFER, heightFieldColors.size() * sizeof(glm::vec4) , &heightFieldColors[0], GL_STATIC_DRAW);

  glGenBuffers(1, &VertexBufferObject);
  glBindBuffer(GL_ARRAY_BUFFER, VertexBufferObject);
  glBufferData(GL_ARRAY_BUFFER, 3 * vertices * sizeof(float) + 4 * vertices * sizeof(float), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * vertices * sizeof(float), &pointPos[0]);
  glBufferSubData(GL_ARRAY_BUFFER, 3 * vertices * sizeof(float), 4 * vertices * sizeof(float), &pointCol[0]);
  
  glGenBuffers(1, &NeighbourVertexBufferObject);
  glBindBuffer(GL_ARRAY_BUFFER, NeighbourVertexBufferObject);
  glBufferData(GL_ARRAY_BUFFER, 4 * vertices * sizeof(float) , &neighbours[0], GL_STATIC_DRAW);

  // init the program
  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  // make the VAO
  glGenVertexArrays(1, &VertexArrayObject);
  glBindVertexArray(VertexArrayObject);

 
  pos_loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  color_loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  neighbour_loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "neighbour"); 

  glGenBuffers(1, &ElementBufferObject);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBufferObject);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,  (height - 1) * (width*2 + 1) * sizeof(int), &pointIdx[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

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

void displayRendering()
{
  switch (render)
  {
    case LINES:
      glDrawElements(GL_LINE_STRIP, indices, GL_UNSIGNED_INT, 0);
      break;
    case TRIANGLES:
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

void createModelViewMatrix(){
  float m[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  matrix.Rotate(landRotate[0], 1, 0, 0);
  matrix.Rotate(landRotate[1], 0, 1, 0);
  matrix.Rotate(landRotate[2], 0, 0, 1);
  matrix.Scale(landScale[0], landScale[1], landScale[2]);
  matrix.LookAt(3, 1.5, 1.0, 0.5, 0.3, 0.5, 0, 5, 0);
  // matrix.LookAt();
  matrix.GetMatrix(m);
  glUniformMatrix4fv(h_modelViewMatrix, 1, GL_FALSE, m);
}

void createProjectionMatrix(){
  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);
  glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, p);
}

void populateHeightFieldPointsAndColors(int m, int n, int i){
  int heightFieldPointSize = heightFieldPoints.size();
  int heightFieldColorSize = heightFieldColors.size();
  float pixelHeight = heightmapImage->getPixel(m,n,0);
  float negativePixelHeight = heightmapImage->getPixel(n,m,0);
  float value = pixelHeight/pixelScale;
  heightFieldPoints[i] = glm::vec3(m,(1.0f * 20)*value,n);
  heightFieldColors[i] = glm::vec4(value,value,value,1.0f);
  value = negativePixelHeight / pixelScale;
  heightFieldPoints[heightFieldPointSize - i - 1] = glm::vec3(n, (1.0f * 20) * value,m);
  heightFieldColors[heightFieldColorSize - i - 1] = glm::vec4(value, value, value, 1.0f);
}