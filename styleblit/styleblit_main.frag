// This software is in the public domain. Where that dedication is not
// recognized, you are granted a perpetual, irrevocable license to copy
// and modify this file as you see fit.

#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D target;
uniform sampler2D source;
uniform sampler2D noise;
uniform vec2 targetSize;
uniform vec2 sourceSize;
uniform float threshold;

float sum(vec3 xyz) { return xyz.x + xyz.y + xyz.z; }

float frac(float x) { return x-floor(x); }

vec4 pack(vec2 xy)
{
  float x = xy.x/255.0;
  float y = xy.y/255.0;
  return vec4(frac(x),floor(x)/255.0,
              frac(y),floor(y)/255.0);
}

bool inside(vec2 uv,vec2 size)
{
  return (all(greaterThanEqual(uv,vec2(0,0))) && all(lessThan(uv,size)));
}

vec2 RandomJitterTable(vec2 uv)
{
  return texture2D(noise,(uv+vec2(0.5,0.5))/vec2(256,256)).xy;
}

vec2 SeedPoint(vec2 p,float h)
{
  vec2 b = floor(p/h);
  vec2 j = RandomJitterTable(b);  
  return floor(h*(b+j));
}

vec2 NearestSeed(vec2 p,float h)
{
  vec2 s_nearest = vec2(0,0);
  float d_nearest = 10000.0;

  for(int x=-1;x<=+1;x++)
  for(int y=-1;y<=+1;y++)
  {
    vec2 s = SeedPoint(p+h*vec2(x,y),h);
    float d = length(s-p);
    if (d<d_nearest)
    {
      s_nearest = s;
      d_nearest = d;     
    }
  }

  return s_nearest;
}

vec3 GS(vec2 uv) { return texture2D(source,(uv+vec2(0.5,0.5))/sourceSize).rgb; }
vec3 GT(vec2 uv) { return texture2D(target,(uv+vec2(0.5,0.5))/targetSize).rgb; }

vec2 ArgMinLookup(vec3 targetNormal)
{
  return vec2(targetNormal.x,targetNormal.y)*sourceSize;
}

void main()
{
  vec2 p = gl_FragCoord.xy-vec2(0.5,0.5);
  vec2 o = ArgMinLookup(GT(p));

  for(int level=6;level>=0;level--)
  {
    vec2 q = NearestSeed(p,pow(2.0,float(level)));
    vec2 u = ArgMinLookup(GT(q));
    
    float e = sum(abs(GT(p)-GS(u+(p-q))))*255.0;
    
    if (e<threshold)
    {
      o = u+(p-q); if (inside(o,sourceSize)) { break; }
    }
  }

  gl_FragColor = pack(o);
}
