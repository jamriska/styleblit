// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy
// and modify this file as you see fit.

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "styleblit.h"

#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

float threshold = 24.0f;
int jitter = 12;
int blendRadius = 1;

int sourceSize = 235;

int targetWidth = -1;
int targetHeight = -1;

GLuint texSourceStyle = 0;
GLuint texSourceNormals = 0;
GLuint texTargetNormals = 0;

GLuint fbo = 0;
GLuint depthBuffer = 0;

GLuint vaoModel = 0;
int numModelVerts = 0;

GLuint progDrawNormals = 0;

std::vector<std::string> styles =
{
  "0.png",
  "01c.png",
  "03b.png",
  "05.png",
  "10b.png",
  "12b.png",
  "15.png",
  "new10.png",
  "new11.png",
  "new12.png",
  "new15.png",
  "new16.png"
};

std::vector<GLuint> texStyleIcons;

glm::mat4 viewMatrix;
glm::vec3 lookAt;

GLFWwindow* window;

int windowWidth = 0;
int windowHeight = 0;

int buttonStates[3]     = { GLFW_RELEASE, GLFW_RELEASE, GLFW_RELEASE };
int lastButtonStates[3] = { GLFW_RELEASE, GLFW_RELEASE, GLFW_RELEASE };

float mouseWheelDelta = 0;

bool done = false;

static GLuint createTexture2D(GLint format,int width,int height,const void* data,GLint filter,GLint wrap)
{
  GLuint texture;
  glGenTextures(1,&texture);  
  glBindTexture(GL_TEXTURE_2D,texture);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glTexImage2D(GL_TEXTURE_2D,0,format,width,height,0,format,GL_UNSIGNED_BYTE,data);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrap);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrap);
  return texture;
}

static GLuint loadTexture(const std::string& fileName,GLint format,const int resolution,GLint filter)
{
  const int numChannels = (format==GL_RGBA) ? 4 : 3;
  int width;
  int height;
 
  stbi_set_flip_vertically_on_load(1);  
  unsigned char* image = stbi_load(fileName.c_str(),&width,&height,NULL,numChannels);
  unsigned char* resizedImage = new unsigned char[resolution*resolution*numChannels];
  stbir_resize_uint8(image,width,height,0,resizedImage,resolution,resolution,0,numChannels);

  GLuint texture = createTexture2D(format,resolution,resolution,resizedImage,filter,GL_CLAMP_TO_EDGE);

  delete[] resizedImage;
  stbi_image_free(image);

  return texture;
}

static void bindFBO(GLuint fbo,GLuint texture,GLuint depthBuffer)
{
  glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glBindTexture(GL_TEXTURE_2D,texture);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depthBuffer);

  const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status!=GL_FRAMEBUFFER_COMPLETE) { printf("incomplete fbo!\n"); exit(1); }
}

static char* stringFromFile(const char* fileName,const char* stringPrefix = "")
{
  FILE* f = fopen(fileName,"rb");
  if (!f) { return 0; }
  fseek(f,0,SEEK_END);
  const long int fileSize = ftell(f);
  rewind(f);
  const size_t prefixLength = strlen(stringPrefix);
  const size_t stringLength = prefixLength + fileSize + 1;  
  char* string = new char[stringLength];
  strcpy(string,stringPrefix);
  string[stringLength-1] = '\0';
  if (fread(string+prefixLength,sizeof(char),fileSize,f)!=fileSize) { delete[] string; string = 0; }
  fclose(f);
  return string;
}

static GLuint compileShader(GLenum type,const GLchar* source)
{
  const GLuint shader = glCreateShader(type);
  glShaderSource(shader,1,&source,0);
  glCompileShader(shader);
  return shader;
}

static GLuint createProgram(const char* vertexShaderFileName,
                            const char* fragmentShaderFileName,
                            const char* fragmentShaderPrefix = "")
{ 
  const char* vertexShaderSource = stringFromFile(vertexShaderFileName);
  const char* fragmentShaderSource = stringFromFile(fragmentShaderFileName,fragmentShaderPrefix);

  const GLuint vertexShader = compileShader(GL_VERTEX_SHADER,vertexShaderSource);
  const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER,fragmentShaderSource);

  const GLuint program = glCreateProgram();

  glAttachShader(program,vertexShader);
  glAttachShader(program,fragmentShader);

  glLinkProgram(program);

  GLint linkStatus;
  glGetProgramiv(program,GL_LINK_STATUS,&linkStatus);

  GLint logLength = 0;
  glGetProgramiv(program,GL_INFO_LOG_LENGTH,&logLength);

  if (logLength>0)
  {
    char* infoLog = new char[logLength];
    glGetProgramInfoLog(program,logLength,&logLength,infoLog);
    printf("%s",infoLog);
    delete[] infoLog;
  }

  glDetachShader(program,vertexShader);
  glDetachShader(program,fragmentShader);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  
  delete[] vertexShaderSource;
  delete[] fragmentShaderSource;

  return program;
}

static GLuint createVertexArrayFromOBJ(const std::string& objFileName,GLuint positionLocation,GLuint normalLocation,int* numModelVerts)
{ 
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string err;
  bool ret = tinyobj::LoadObj(&attrib,&shapes,&materials,&err,objFileName.c_str(),"",true);

  if (!err.empty()) { return 0; }

  if (!ret) { return 0; }

  int numTriangles = 0;
  for (int s=0;s<shapes.size();s++)
  {
    numTriangles += shapes[s].mesh.num_face_vertices.size();
  }
  
  std::vector<glm::ivec3> triangles = std::vector<glm::ivec3>(numTriangles);
  int triangleIndex = 0;
  for (int s=0;s<shapes.size();s++)
  {
    for (int f=0;f<shapes[s].mesh.num_face_vertices.size();f++)
    {
      triangles[triangleIndex] = glm::ivec3(shapes[s].mesh.indices[3*f+0].vertex_index,
                                            shapes[s].mesh.indices[3*f+2].vertex_index,
                                            shapes[s].mesh.indices[3*f+1].vertex_index);
      triangleIndex++;
    }
  }

  const int numVertices = attrib.vertices.size()/3;
  std::vector<glm::vec3> vertices = std::vector<glm::vec3>(numVertices);
  for (int i=0;i<numVertices;i++)
  {
    vertices[i] = glm::vec3(attrib.vertices[i*3+0],
                            attrib.vertices[i*3+2],
                            attrib.vertices[i*3+1]);
  }

  {
    glm::vec3 centroid = glm::vec3(0,0,0);
    for (int i=0;i<vertices.size();i++) { centroid += vertices[i]; }
    centroid = centroid / float(vertices.size());

    float radius = 0;
    for (int i=0;i<vertices.size();i++) { radius += glm::length(vertices[i]-centroid); }
    radius = radius / float(vertices.size());

    for (int i=0;i<vertices.size();i++) { vertices[i] = (vertices[i]-centroid)/radius; }
  }

  std::vector<glm::vec3> normals = std::vector<glm::vec3>(numVertices,glm::vec3(0,0,0));
  {
    std::vector<glm::vec3> faceNormals(triangles.size());
    for(int i=0;i<triangles.size();i++)
    {
      const glm::ivec3 t = triangles[i];
      faceNormals[i] = glm::normalize(glm::cross(vertices[t[1]]-vertices[t[0]],
                                                 vertices[t[2]]-vertices[t[0]]));
    }

    std::vector<int> counts = std::vector<int>(vertices.size(),0);
    for(int i=0;i<triangles.size();i++)
    {
      for(int j=0;j<3;j++)
      {
        normals[triangles[i][j]] += faceNormals[i];
        counts[triangles[i][j]] += 1;
      }
    }
    for(int i=0;i<normals.size();i++)
    {
      normals[i] = normalize(normals[i] / float(counts[i]));
    }
  }

  std::vector<glm::vec3> vertexBuffer(triangles.size()*3);
  std::vector<glm::vec3> normalBuffer(triangles.size()*3);

  for(int i=0;i<triangles.size();i++)
  {
    for(int j=0;j<3;j++)
    {
      vertexBuffer[i*3+j] = vertices[triangles[i][j]];
      normalBuffer[i*3+j] = normals[triangles[i][j]];;
    }
  }

  GLuint vbo;
  glGenBuffers(1,&vbo);
  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBufferData(GL_ARRAY_BUFFER,sizeof(glm::vec3)*vertexBuffer.size(),vertexBuffer.data(),GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  GLuint nbo;
  glGenBuffers(1,&nbo);
  glBindBuffer(GL_ARRAY_BUFFER,nbo);
  glBufferData(GL_ARRAY_BUFFER,sizeof(glm::vec3)*normalBuffer.size(),normalBuffer.data(),GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  GLuint vao;
  glGenVertexArrays(1,&vao);
  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glVertexAttribPointer(positionLocation,3,GL_FLOAT,GL_FALSE,0,0);
  glEnableVertexAttribArray(positionLocation);

  glBindBuffer(GL_ARRAY_BUFFER,nbo);
  glVertexAttribPointer(normalLocation,3,GL_FLOAT,GL_FALSE,0,0);
  glEnableVertexAttribArray(normalLocation);

  *numModelVerts = vertexBuffer.size();
  
  return vao;
}

static float clamp(float x,float xmin,float xmax)
{
  return std::min(std::max(x,xmin),xmax);
}

static float lerp(float a,float b,float t)
{
  return (1.0-t)*a+t*b;
}

static bool buttonWentDown(int button)
{
  return (buttonStates[button]==GLFW_PRESS && lastButtonStates[button]==GLFW_RELEASE);
}

static bool buttonIsDown(int button)
{
  return buttonStates[button];
}

static bool buttonWentUp(int button)
{
  return (buttonStates[button]==GLFW_RELEASE && lastButtonStates[button]==GLFW_PRESS);
}

void keyCallback(GLFWwindow* window,int key,int scancode,int action,int mods)
{
  if (key==GLFW_KEY_J      && action==GLFW_PRESS) { jitter = (jitter==0) ? 12 : 0; }
  if (key==GLFW_KEY_UP     && (action==GLFW_PRESS||action==GLFW_REPEAT)) { if (threshold<64)  { threshold += 4;   } }
  if (key==GLFW_KEY_DOWN   && (action==GLFW_PRESS||action==GLFW_REPEAT)) { if (threshold>=4)  { threshold -= 4;   } }
  if (key==GLFW_KEY_RIGHT  && (action==GLFW_PRESS||action==GLFW_REPEAT)) { if (blendRadius<8) { blendRadius += 1; } }
  if (key==GLFW_KEY_LEFT   && (action==GLFW_PRESS||action==GLFW_REPEAT)) { if (blendRadius>0) { blendRadius -= 1; } }
  if (key==GLFW_KEY_ESCAPE && (action==GLFW_PRESS||action==GLFW_REPEAT)) { done = true; }
}
  
void scrollCallback(GLFWwindow* window,double xoffset,double yoffset)
{
  mouseWheelDelta = yoffset;
}

static void handleCamera(glm::mat4* inout_viewMatrix,
                         glm::vec3* inout_lookAt,
                         float      dt,
                         int        minMouseY)
{
  double mouseX;
  double mouseY;
  glfwGetCursorPos(window,&mouseX,&mouseY);

  glm::mat4& viewMatrix = *inout_viewMatrix;
  glm::vec3& lookAt = *inout_lookAt;

  static glm::vec3 lastCamPos;
  static glm::vec3 lastCamDir;
  static glm::vec3 lastCamUp;
  static glm::vec3 lastLookAt;

  static float dragStartX;
  static float dragStartY;

  static int navigationMode = 0;

  static float velAlpha = 0;
  static float velBeta = 0;

  static float smoothAlpha;
  static float smoothBeta;

  static int lastMouseX;
  static int lastMouseY;

  static bool inertia = false;

  static float initialVelBeta = -0.02;

  static bool firstCall = true;

  const float scale = clamp(1125.0f/float(windowWidth),1.0f,2.0f);

  {
    const glm::mat4 invViewMatrix = glm::inverse(viewMatrix);

    glm::vec3 cameraPosition  = glm::vec3(invViewMatrix[3][0],
                                          invViewMatrix[3][1],
                                          invViewMatrix[3][2]);

    glm::vec3 cameraDirection = glm::vec3(-viewMatrix[0][2],
                                          -viewMatrix[1][2],
                                          -viewMatrix[2][2]);

    glm::vec3 cameraUp        = glm::vec3(viewMatrix[0][1],
                                          viewMatrix[1][1],
                                          viewMatrix[2][1]);

    if ((mouseY>minMouseY && (buttonWentDown(GLFW_MOUSE_BUTTON_LEFT)||buttonWentDown(GLFW_MOUSE_BUTTON_RIGHT)))||firstCall)
    {
      if (!firstCall) { initialVelBeta = 0; }

      if (firstCall)
      {
        navigationMode = 1; inertia = true;
      }
      smoothAlpha = 0;
      smoothBeta = 0;

      dragStartX = mouseX;
      dragStartY = mouseY;

      lastCamPos = cameraPosition;
      lastCamDir = cameraDirection;
      lastCamUp  = cameraUp;
      lastLookAt = lookAt;

      lastMouseX = mouseX;
      lastMouseY = mouseY;

      firstCall = false;      
    }

    if      (mouseY>minMouseY && buttonWentDown(GLFW_MOUSE_BUTTON_LEFT))  { navigationMode = 1; inertia = true;  }
    else if (mouseY>minMouseY && buttonWentDown(GLFW_MOUSE_BUTTON_RIGHT)) { navigationMode = 2; inertia = false; }

    if (buttonWentUp(GLFW_MOUSE_BUTTON_LEFT) || buttonWentUp(GLFW_MOUSE_BUTTON_RIGHT))
    {
      navigationMode = 0;
    }

    if (navigationMode==1 || inertia || initialVelBeta!=0)
    {
      if (mouseY>minMouseY && (buttonWentDown(GLFW_MOUSE_BUTTON_LEFT) || buttonIsDown(GLFW_MOUSE_BUTTON_LEFT)))
      {
        static float lastTime = 0;
        float time = glfwGetTime()*1000.0f;          
        if (time-lastTime>30.0f)
        {
          float dx = float(mouseX-lastMouseX);
          float dy = float(mouseY-lastMouseY);

          lastMouseX = mouseX;
          lastMouseY = mouseY;
          
          velAlpha = lerp(velAlpha,0.002f*scale*clamp(dy,-300.0f,+300.0f),0.6f);
          velBeta  = lerp(velBeta,-0.0035f*scale*clamp(dx,-300.0f,+300.0f),0.6f);

          lastTime = time;
        }
      }
      if (buttonWentUp(GLFW_MOUSE_BUTTON_LEFT))
      {
        velAlpha = velAlpha*0.75f;
        velBeta = velBeta*0.8f;
      }

      velAlpha = velAlpha-velAlpha*0.004f*std::min(dt/30.0f,6.0f);
      velBeta  = velBeta-velBeta*0.004*std::min(dt/30.0f,6.0f);

      if (initialVelBeta!=0) { velBeta = initialVelBeta; }

      smoothAlpha = smoothAlpha + velAlpha*(dt/30.0f);
      smoothBeta = smoothBeta + velBeta*(dt/30.0f);

      glm::mat3 R = glm::rotate(glm::rotate(glm::mat4(1.0f),smoothBeta,glm::vec3(0,1,0)),smoothAlpha,glm::normalize(glm::cross(lastCamUp,lastCamDir)));

      lastCamPos=lastCamPos+mouseWheelDelta*lastCamDir*0.5f;
      cameraPosition = lookAt + R*(lastCamPos-lookAt);
      cameraDirection = R*lastCamDir;
      cameraUp = R*lastCamUp;
    }
    else if (navigationMode==2)
    {
      glm::vec3 cameraRight = cross(cameraDirection,cameraUp);

      glm::vec3 shift = (float(mouseX-dragStartX)*cameraRight +
                        -float(mouseY-dragStartY)*cameraUp)*0.003f*scale;

      cameraPosition = lastCamPos - shift;
      lookAt = lastLookAt - shift;
    }

    if (mouseWheelDelta!=0 && navigationMode==0 && !inertia)
    {
      cameraPosition += mouseWheelDelta*cameraDirection*0.5f;
    }

    viewMatrix = glm::lookAt(cameraPosition,cameraPosition+cameraDirection,cameraUp);
  }
}

static void drawRectTex(const glm::mat4& matrix,float x0,float y0,float x1,float y1,int texId)
{
  static GLuint prog = 0;
  static GLint positionLocation = 0;
  static GLint texcoordLocation = 0;

  if (prog==0)
  {
    prog = createProgram("data/texrect.vert","data/texrect.frag");
    positionLocation = glGetAttribLocation(prog,"position");
    texcoordLocation = glGetAttribLocation(prog,"texCoord_in");
  }

  std::vector<glm::vec2> vertices = { {x0,y0},{x1,y0},{x1,y1},
                                      {x1,y1},{x0,y1},{x0,y0} };
  
  std::vector<glm::vec2> texcoords = { {0,1},{1,1},{1,0},
                                       {1,0},{0,0},{0,1} };

  static GLuint vbo = 0;
  static GLuint tbo = 0;
  static GLuint vao = 0;

  if (vbo==0) { glGenBuffers(1,&vbo); }
  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBufferData(GL_ARRAY_BUFFER,sizeof(glm::vec2)*vertices.size(),vertices.data(),GL_STATIC_DRAW);

  if (tbo==0) { glGenBuffers(1,&tbo); }
  glBindBuffer(GL_ARRAY_BUFFER,tbo);
  glBufferData(GL_ARRAY_BUFFER,sizeof(glm::vec2)*texcoords.size(),texcoords.data(),GL_STATIC_DRAW);

  if (vao==0)
  {
    glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(positionLocation,2,GL_FLOAT,GL_FALSE,0,0);
    glEnableVertexAttribArray(positionLocation);

    glBindBuffer(GL_ARRAY_BUFFER, tbo);
    glVertexAttribPointer(texcoordLocation,2,GL_FLOAT,GL_FALSE,0,0);
    glEnableVertexAttribArray(texcoordLocation);
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,texId);

  glUseProgram(prog);
  glUniformMatrix4fv(glGetUniformLocation(prog,"modelviewproj"),1,GL_FALSE,glm::value_ptr(matrix));
  glUniform1i(glGetUniformLocation(prog,"tex"),0);

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES,0,vertices.size());

  glDisable(GL_BLEND);
}

static bool iconClicked(const glm::mat4& projMatrix,int x0,int y0,int x1,int y1,int texIcon)
{ 
  double mouseX,mouseY;
  glfwGetCursorPos(window,&mouseX,&mouseY);

  const bool clicked = (buttonWentDown(GLFW_MOUSE_BUTTON_LEFT) &&
                        mouseX>=x0 && mouseX<=x1 &&
                        mouseY>=y0 && mouseY<=y1);
 
  drawRectTex(projMatrix,x0,y0,x1,y1,texIcon);

  return clicked;
}

static void mainloop()
{
  for(int button=0;button<3;button++)
  {
    lastButtonStates[button] = buttonStates[button];
    buttonStates[button] = glfwGetMouseButton(window,button);
  }

  glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

  static float lastTicks = 0;
  float ticks = glfwGetTime()*1000.0f;
  float dt = (ticks-lastTicks);
  float fps = 1000.0f/dt;
  lastTicks = ticks;

  bool jitterThisFrame = false;

  if      (jitter==-1) { jitterThisFrame = true; }
  else if (jitter==0)  { jitterThisFrame = false; }
  else if (jitter==12)
  {
    static float lastJitterTime = 0;
    float time = glfwGetTime()*1000.0f;
    if (time-lastJitterTime>(1000.0/12.0f))
    {
      jitterThisFrame = true;
      lastJitterTime = time;
    }
  }

  if (glfwWindowShouldClose(window)) { done = true; }

  if (targetWidth!=windowWidth ||
      targetHeight!=windowHeight)
  {
    targetWidth = windowWidth;
    targetHeight = windowHeight;

    glDeleteTextures(1,&texTargetNormals);
    texTargetNormals = createTexture2D(GL_RGBA,targetWidth,targetHeight,0,GL_NEAREST,GL_CLAMP_TO_EDGE);

    glBindRenderbuffer(GL_RENDERBUFFER,depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT16,targetWidth,targetHeight);
  }

  glViewport(0,0,windowWidth,windowHeight);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

  bindFBO(fbo,texTargetNormals,depthBuffer);

  glViewport(0,0,targetWidth,targetHeight);
  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const float maxIconSize = float(windowWidth)/float(styles.size());
  const float iconSize = std::min(std::max(float(windowHeight)*0.1f,48.0f),maxIconSize);

  handleCamera(&viewMatrix,&lookAt,dt,iconSize);

  const glm::mat4 projMatrix = glm::translate(glm::perspective(glm::radians(40.0f),float(windowWidth)/float(windowHeight),0.1f,100.f),
                                              glm::vec3(0,-(1.2f*iconSize)/float(windowHeight),0));
  
  const glm::mat4 normalMatrix = glm::transpose(glm::inverse(viewMatrix));
  
  const glm::mat4 projViewMatrix = projMatrix*viewMatrix;

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  glUseProgram(progDrawNormals);

  glUniformMatrix4fv(glGetUniformLocation(progDrawNormals,"projviewMatrix"),1,GL_FALSE,glm::value_ptr(projViewMatrix));
  glUniformMatrix4fv(glGetUniformLocation(progDrawNormals,"normalMatrix"),1,GL_FALSE,glm::value_ptr(normalMatrix));

  glBindVertexArray(vaoModel);
  glDrawArrays(GL_TRIANGLES,0,numModelVerts);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER,0);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glBindFramebuffer(GL_FRAMEBUFFER,0);

  styleblit(targetWidth,
            targetHeight,
            texTargetNormals,
            sourceSize,
            sourceSize,
            texSourceNormals,
            texSourceStyle,
            threshold,
            blendRadius,
            jitterThisFrame);

  {
    glDisable(GL_DEPTH_TEST);
    const int barWidth = (styles.size()*iconSize);
    const glm::mat4 projMatrix = glm::ortho(0.0f,float(windowWidth),float(windowHeight),0.0f,-1.0f,+1.0f);
    for(int i=0;i<styles.size();i++)
    {
      const int spacing = iconSize*0.1;
      const int x = windowWidth/2-barWidth/2 + i*(iconSize) + spacing/4; 
      const int y = spacing/3;
      if (iconClicked(projMatrix,x+spacing/2,y+spacing/2,x+iconSize-spacing,y+iconSize-spacing,texStyleIcons[i]))
      {
        glDeleteTextures(1,&texSourceStyle);
        glDeleteTextures(1,&texSourceNormals);

        sourceSize = (i==0||i==1||i==2||i==5||i==9) ? windowHeight/4 : windowHeight/3;

        texSourceStyle   = loadTexture("data/"+styles[i],GL_RGB,sourceSize,GL_NEAREST);
        texSourceNormals = loadTexture("data/normals.png",GL_RGB,sourceSize,GL_NEAREST);
      }
    }
  }
  
  mouseWheelDelta = 0;

  glfwSwapBuffers(window);
  glfwPollEvents();
}

int main(int argc,char* args[])
{
  printf("Controls                                \n");
  printf("========================================\n");
  printf("Left button  - orbit camera             \n");
  printf("Right button - move camera              \n");
  printf("Mouse wheel  - zoom in/out              \n");
  printf("Key J        - toggle jitter            \n");
  printf("Up arrow     - increase treshold        \n");
  printf("Down arrow   - decrease treshold        \n");
  printf("Left arrow   - decrease blending radius \n");
  printf("Right arrow  - increase blending radius \n");
 
  if (!glfwInit()) { return -1; }

  window = glfwCreateWindow(640,700,"StyleBlit",NULL,NULL);
  
  if (!window) { glfwTerminate(); return -1; }

  glfwSetKeyCallback(window,keyCallback);
  glfwSetScrollCallback(window,scrollCallback);

  glfwMakeContextCurrent(window);
  
  glfwGetFramebufferSize(window,&windowWidth,&windowHeight);
  
  glewInit();

  glGenFramebuffers(1,&fbo);
  glGenRenderbuffers(1,&depthBuffer);

  viewMatrix = glm::lookAt(glm::vec3(+5.0f,0.25f,-3.0f)*0.9f,
                           glm::vec3(0.0f,0.25f,0.0f),
                           glm::vec3(0.0f,1.0f,0.0f));

  lookAt = glm::vec3(0,0.5,0);


  progDrawNormals = createProgram("data/normals.vert","data/normals.frag");

  vaoModel = createVertexArrayFromOBJ("data/golem.obj",
                                      glGetAttribLocation(progDrawNormals,"position"),
                                      glGetAttribLocation(progDrawNormals,"normal"),
                                      &numModelVerts);

  sourceSize = windowHeight/4;

  texSourceStyle = loadTexture("data/"+styles[0],GL_RGB,sourceSize,GL_NEAREST);
  texSourceNormals = loadTexture("data/normals.png",GL_RGB,sourceSize,GL_NEAREST);

  for(int i=0;i<styles.size();i++)
  {
    texStyleIcons.push_back(loadTexture("data/"+styles[i],GL_RGBA,92,GL_LINEAR));
  }

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(mainloop,0,0);
#else
  while (!done)
  {
    mainloop();
  }
  glfwTerminate();
#endif

  return 0;
}
