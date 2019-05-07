// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy
// and modify this file as you see fit.

#ifdef GL_ES
precision highp float;
#endif

varying vec3 color;

void main()
{
  gl_FragColor = vec4(color,1.0);
}
