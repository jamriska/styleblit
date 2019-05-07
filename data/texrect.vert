// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy
// and modify this file as you see fit.

attribute vec2 position;
attribute vec2 texCoord_in;

uniform mat4 modelviewproj;

varying vec2 texCoord;

void main()
{
  gl_Position = modelviewproj*vec4(position,0,1);
  texCoord = texCoord_in;
}
