// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy
// and modify this file as you see fit.

#ifdef GL_ES
precision highp float;
#endif

varying vec2 texCoord;
uniform sampler2D tex;

void main(void)
{
  gl_FragColor = texture2D(tex,texCoord);
}
