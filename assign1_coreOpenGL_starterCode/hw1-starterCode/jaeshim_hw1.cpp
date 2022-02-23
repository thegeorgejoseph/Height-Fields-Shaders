/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: <type your USC username here>
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

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;

GLuint positionVertexBuffer, colorVertexBuffer;
GLuint vertexArray;
int imageWidth, imageHeight;

OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;

std::vector<glm::vec3> heightField;
std::vector<glm::vec4> grayScale;

GLenum mode;
GLsizei vertices;

GLuint triPositionVertexBuffer, triColorVertexBuffer;
GLuint triVertexArray;

std::vector<glm::vec3> triHeightField;
std::vector<glm::vec4> triGrayScale;

GLboolean smooth;

GLuint triPositionNeighborsVertexBuffer;

std::vector<glm::vec4> triNeighborsHeightField;

void generateVertexBufferObjects()
{
  heightField.resize((imageHeight + 1) * imageWidth * 2);
  grayScale.resize((imageHeight + 1) * imageWidth * 2);

  triHeightField.resize((imageHeight - 1) * imageWidth * 2);
  triGrayScale.resize((imageHeight - 1) * imageWidth * 2);

  triNeighborsHeightField.resize((imageHeight - 1) * imageWidth * 2);

  int index = 0;
  
  for(int i = 0; i <= imageHeight; i++)
  {
    for(int j = 0; j < imageWidth; j++, index++)
    {
      int x = min(i, imageHeight - 1);
      int y = i % 2 ? imageWidth - j - 1 : j;

      float height = heightmapImage->getPixel(x, y, 0) / 256.0f;

      // Points/Lines
      {
        heightField[index] = glm::vec3(x, 20.0f * height, y);

        grayScale[index] = glm::vec4(height, height, height, 1.0f);

        float reverseHeight = heightmapImage->getPixel(y, x, 0) / 256.0f;

        heightField[heightField.size() - index - 1] = glm::vec3(y, 20.0f * reverseHeight, x);

        grayScale[heightField.size() - index - 1] = glm::vec4(reverseHeight, reverseHeight, reverseHeight, 1.0f);
      }

      // Triangles
      if(i < imageHeight - 1)
      {
        triHeightField[index * 2] = glm::vec3(x, 20.0f * height, y);

        triGrayScale[index * 2] = glm::vec4(height, height, height, 1.0f);

        height = heightmapImage->getPixel(x + 1, y, 0) / 256.0f;

        triHeightField[index * 2 + 1] = glm::vec3(x + 1, 20.0f * height, y);

        triGrayScale[index * 2 + 1] = glm::vec4(height, height, height, 1.0f);

        // Smoothened Triangles
        {
          glm::vec4& neighborsOne = triNeighborsHeightField[index * 2];
          glm::vec4& neighborsTwo = triNeighborsHeightField[index * 2 + 1];

          neighborsOne[0] = heightmapImage->getPixel(x, y + (y == 0 ? 1 : -1), 0) / 256.0f;
          neighborsOne[1] = heightmapImage->getPixel(x, y + (y == imageWidth - 1 ? -1 : 1), 0) / 256.0f;
          neighborsOne[2] = heightmapImage->getPixel(x + 1, y, 0) / 256.0f;
          neighborsOne[3] = heightmapImage->getPixel(x + (x == 0 ? 1 : -1), y, 0) / 256.0f;

          neighborsTwo[0] = heightmapImage->getPixel(x + 1, y + (y == 0 ? 1 : -1), 0) / 256.0f;
          neighborsTwo[1] = heightmapImage->getPixel(x + 1, y + (y == imageWidth - 1 ? -1 : 1), 0) / 256.0f;
          neighborsTwo[2] = heightmapImage->getPixel(x + 1 + (x == imageHeight - 1 ? -1 : 1), y, 0) / 256.0f;
          neighborsTwo[3] = heightmapImage->getPixel(x, y, 0) / 256.0f;

        }
      }
    }
  }
}

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

  pipelineProgram->Bind();

  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.LookAt(imageWidth / 2, (imageHeight + imageWidth) / 4, -imageHeight / 2, imageWidth / 2, 0, imageHeight / 2, 0, 0, 1);
  //matrix.LookAt(0, 100, imageHeight / 2, 0, 0, -imageHeight / 2, 0, 0, -1);

  // Transform
  matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  matrix.Rotate(landRotate[0], 1.0f, 0.0f, 0.0f);
  matrix.Rotate(landRotate[1], 0.0f, 1.0f, 0.0f);
  matrix.Rotate(landRotate[2], 0.0f, 0.0f, 1.0f);
  matrix.Scale(landScale[0], landScale[1], landScale[2]);

  float m[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(m);

  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);
  //
  // bind shader
  pipelineProgram->Bind();

  // set variable
  pipelineProgram->SetModelViewMatrix(m);
  pipelineProgram->SetProjectionMatrix(p);
  pipelineProgram->SetMode(smooth);

  glBindBuffer(GL_ARRAY_BUFFER, (mode != GL_TRIANGLE_STRIP) ? positionVertexBuffer : triPositionVertexBuffer);
  GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, (mode != GL_TRIANGLE_STRIP) ? colorVertexBuffer : triColorVertexBuffer);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  if(smooth)
  {
    glBindBuffer(GL_ARRAY_BUFFER, triPositionNeighborsVertexBuffer);
    loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "positionNeighbors");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);
  }

  glDrawArrays(mode, 0, vertices);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);

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
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
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
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
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
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
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

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
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

    case '1':
      mode = GL_POINTS;
      vertices = imageHeight * imageWidth;
      smooth = false;
    break;

    case '2':
      mode = GL_LINE_STRIP;
      vertices = heightField.size();
      smooth = false;
    break;

    case '3':
      mode = GL_TRIANGLE_STRIP;
      vertices = triHeightField.size();
      smooth = false;
    break;

    case '4':
      mode = GL_TRIANGLE_STRIP;
      vertices = triHeightField.size();
      smooth = true;
    break;

    case 'z':
      controlState = TRANSLATE;
    break;
  }
}

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  
  imageHeight = heightmapImage->getHeight();
  imageWidth = heightmapImage->getWidth();

  keyboardFunc('1', 0, 0);

  generateVertexBufferObjects();

  glGenBuffers(1, &positionVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, positionVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * heightField.size(), &heightField[0], GL_STATIC_DRAW);

  glGenBuffers(1, &colorVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, colorVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * grayScale.size(), &grayScale[0], GL_STATIC_DRAW);

  glGenBuffers(1, &triPositionVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, triPositionVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * triHeightField.size(), &triHeightField[0], GL_STATIC_DRAW);

  glGenBuffers(1, &triColorVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, triColorVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * triGrayScale.size(), &triGrayScale[0], GL_STATIC_DRAW);

  glGenBuffers(1, &triPositionNeighborsVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, triPositionNeighborsVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * triNeighborsHeightField.size(), &triNeighborsHeightField[0], GL_STATIC_DRAW);

  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  glGenVertexArrays(1, &vertexArray);
  glBindVertexArray(vertexArray);

  glGenVertexArrays(1, &triVertexArray);
  glBindVertexArray(triVertexArray);

  glEnable(GL_DEPTH_TEST);
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

