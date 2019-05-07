// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy
// and modify this file as you see fit.

uniform mat4 projviewMatrix;
uniform mat4 normalMatrix;

attribute vec3 position;
attribute vec3 normal;

varying vec3 color;

void main()
{
  gl_Position = projviewMatrix*vec4(position,1.0);
  vec4 n = normalMatrix*vec4(normal,0.0);
  color = n.xyz*0.5+vec3(0.5,0.5,0.5);
}
