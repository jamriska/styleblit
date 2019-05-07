// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy
// and modify this file as you see fit.

#ifdef GL_ES
precision highp float;
#endif

#ifndef BLEND_RADIUS
  #define BLEND_RADIUS 1
#endif

uniform sampler2D NNF;
uniform sampler2D sourceStyle;
uniform sampler2D targetMask;
uniform vec2 targetSize;
uniform vec2 sourceSize;

vec2 unpack(vec4 rgba)
{
  return vec2(rgba.r*255.0+rgba.g*255.0*255.0,
              rgba.b*255.0+rgba.a*255.0*255.0);
}

void main()
{ 
  vec2 xy = gl_FragCoord.xy;

  vec4 sumColor = vec4(0.0,0.0,0.0,0.0);
  float sumWeight = 0.0;
  
  if (texture2D(targetMask,(xy)/targetSize).a>0.0)
  {
    for(int oy=-BLEND_RADIUS;oy<=+BLEND_RADIUS;oy++)
    for(int ox=-BLEND_RADIUS;ox<=+BLEND_RADIUS;ox++)
    {
      if (texture2D(targetMask,(xy+vec2(ox,oy))/targetSize).a>0.0)
      {
        sumColor += texture2D(sourceStyle,((unpack(texture2D(NNF,(xy+vec2(ox,oy))/targetSize))-vec2(ox,oy))+vec2(0.5,0.5))/sourceSize);
        sumWeight += 1.0;
      }
    }
  }
  gl_FragColor = (sumWeight>0.0) ? sumColor/sumWeight : texture2D(sourceStyle,vec2(0.0,0.0));
}
