// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy
// and modify this file as you see fit.

attribute vec3 position;

void main()
{
  gl_Position = vec4(position,1.0);
}
