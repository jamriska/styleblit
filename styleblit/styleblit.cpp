// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy
// and modify this file as you see fit.

#include <GL/glew.h>

#include "styleblit.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

static GLuint createTexture2D(GLint format,int width,int height,GLint filter,GLint wrap)
{
  GLuint texture;
  glGenTextures(1,&texture);  
  glBindTexture(GL_TEXTURE_2D,texture);
  glTexImage2D(GL_TEXTURE_2D,0,format,width,height,0,format,GL_UNSIGNED_BYTE,0);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrap);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrap);
  return texture;
}

static GLuint createFBO(GLuint texture)
{
  GLuint fbo;
  glGenFramebuffers(1,&fbo);
  glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glBindTexture(GL_TEXTURE_2D,texture);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
  const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status!=GL_FRAMEBUFFER_COMPLETE) { printf("incomplete FBO!\n"); exit(1); }
  return fbo;
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

static void drawFullscreenTriangle(GLint positionLocation)
{
  static GLuint vbo = 0;
  static GLuint vao = 0;

  if (vbo==0)
  {
    const float vertices[] = { -1, -1, 0,
                               +4, -1, 0,
                               -1, +4, 0 };

    glGenBuffers(1,&vbo);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER,0);
  }

  if (vao==0)
  {
    glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glVertexAttribPointer(positionLocation,3,GL_FLOAT,GL_FALSE,0,0);
    glEnableVertexAttribArray(positionLocation);
  }

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES,0,3);
}

void styleblit(int targetWidth,
               int targetHeight,
               GLuint texTargetNormals,
               int sourceWidth,
               int sourceHeight,
               GLuint texSourceNormals,
               GLuint texSourceStyle,
               float threshold,
               int blendRadius,
               bool jitter)
{
  static GLuint progMain = 0;
  static GLuint progBlend = 0;
  static GLuint texNNF = 0;
  static GLuint fboNNF = 0;
  static GLuint texJitterTable = 0;
  const int jitterTableWidth = 256;
  const int jitterTableHeight = 256;
  static unsigned char* jitterTableData = 0;
  static int oldTargetWidth = 0;
  static int oldTargetHeight = 0;
  static int oldBlendRadius = 0;
  static bool initialized = false;

  if (!initialized)
  {
    progMain = createProgram("styleblit/styleblit_pass.vert","styleblit/styleblit_main.frag");
    texNNF  = createTexture2D(GL_RGBA,targetWidth,targetHeight,GL_NEAREST,GL_CLAMP_TO_EDGE);
    fboNNF  = createFBO(texNNF);
    texJitterTable = createTexture2D(GL_RGBA,jitterTableWidth,jitterTableHeight,GL_NEAREST,GL_REPEAT);
    jitter = true; 
    initialized = true;
  }

  if (progBlend==0 || blendRadius!=oldBlendRadius)
  {
    char votePrefix[256];
    sprintf(votePrefix,"#define BLEND_RADIUS %d\n",blendRadius);
    if (progBlend!=0) { glDeleteProgram(progBlend); }
    progBlend = createProgram("styleblit/styleblit_pass.vert","styleblit/styleblit_blend.frag",votePrefix);
    oldBlendRadius = blendRadius;
  }

  if (targetWidth!=oldTargetWidth || targetHeight!=oldTargetHeight)
  {
    glBindTexture(GL_TEXTURE_2D,texNNF);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,targetWidth,targetHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,0);

    oldTargetWidth  = targetWidth;
    oldTargetHeight = targetHeight;
  }

  if (jitter)
  {
    const int jitterTableSize = jitterTableWidth*jitterTableHeight*4;    
    if (jitterTableData==0) { jitterTableData = new unsigned char[jitterTableSize]; }
    for(int i=0;i<jitterTableSize;i++) { jitterTableData[i] = (float(rand())/float(RAND_MAX))*255.0f; }
    
    glBindTexture(GL_TEXTURE_2D,texJitterTable);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,jitterTableWidth,jitterTableHeight,GL_RGBA,GL_UNSIGNED_BYTE,jitterTableData);
  }

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  ///////////////////////////////////////////////////////////////////////////

  glBindFramebuffer(GL_FRAMEBUFFER,fboNNF);
  glViewport(0,0,targetWidth,targetHeight);
  glUseProgram(progMain);
  glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,texTargetNormals);
  glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,texSourceNormals);
  glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D,texJitterTable);
  glUniform1i(glGetUniformLocation(progMain,"target"),0);
  glUniform1i(glGetUniformLocation(progMain,"source"),1);
  glUniform1i(glGetUniformLocation(progMain,"noise"),2);
  glUniform2f(glGetUniformLocation(progMain,"targetSize"),targetWidth,targetHeight);
  glUniform2f(glGetUniformLocation(progMain,"sourceSize"),sourceWidth,sourceHeight);
  glUniform1f(glGetUniformLocation(progMain,"threshold"),threshold);
  drawFullscreenTriangle(glGetAttribLocation(progMain,"position"));

  ///////////////////////////////////////////////////////////////////////////

  glBindFramebuffer(GL_FRAMEBUFFER,0);
  glEnable(GL_DEPTH_TEST);
  glViewport(0,0,targetWidth,targetHeight);
  glUseProgram(progBlend);
  glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,texNNF);
  glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,texSourceStyle);
  glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D,texTargetNormals);
  glUniform1i(glGetUniformLocation(progBlend,"NNF"),0);
  glUniform1i(glGetUniformLocation(progBlend,"sourceStyle"),1);
  glUniform1i(glGetUniformLocation(progBlend,"targetMask"),2);
  glUniform2f(glGetUniformLocation(progBlend,"targetSize"),targetWidth,targetHeight);
  glUniform2f(glGetUniformLocation(progBlend,"sourceSize"),sourceWidth,sourceHeight);
  drawFullscreenTriangle(glGetAttribLocation(progBlend,"position"));
}
