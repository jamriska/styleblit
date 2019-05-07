// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy
// and modify this file as you see fit.

#ifndef STYLEBLIT_H_
#define STYLEBLIT_H_

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

void styleblit(int    targetWidth,
               int    targetHeight,
               GLuint texTargetNormals,
               int    sourceWidth,
               int    sourceHeight,
               GLuint texSourceNormals,
               GLuint texSourceStyle,
               float  threshold,
               int    blendRadius,
               bool   jitter);

#endif
