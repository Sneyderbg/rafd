#version 420

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 color;

#define uv fragTexCoord
#define lerp(a, b, x) (a + x*(b - a))
#define circ(x) (1. -  sqrt(1. - x*x))
#define PI 3.1415926535

uniform bool isCurrent;
uniform bool isPrevious;
uniform bool isNext;
uniform bool isTarget;
uniform float time;

float d;
const float ringWidth = .08;

void ring() {
  if (isCurrent || isTarget) {
    float ringPos = .5;
    float segLen = .5;
    if (isTarget) {
      ringPos += (ringWidth / 4.) * .5 * (sin(time * 6.) - 1.);
    }

    float r = smoothstep(ringPos - ringWidth, ringPos - ringWidth / 2., d) - smoothstep(ringPos - ringWidth / 2., ringPos, d);
    if (isCurrent) {
      float a = atan(uv.y - ringPos, uv.x - ringPos) / PI * 8. - time;
      a = fract(a);
      r *= smoothstep(ringPos - segLen, ringPos - segLen / 2., a) * smoothstep(ringPos + segLen, ringPos + segLen / 2., a);
    }

    color = r * fragColor;
  }
  if (color.a < .05) {
    color.a = 0.;
  }
}

void circle() {
  if (color.a <= 0.) {
    float border = smoothstep(.5 - ringWidth, .5 - ringWidth * 1.8, d);
    float c = circ(d / .5);
    color += border * fragColor * (1. - c);
    if (border >= .6) {
      float shade = lerp(0., 1., length(uv - vec2(1., 0.)));
      color.xyz = pow(color.xyz, vec3(shade / (1.0 + .2 * abs(sin(time * 3. - 2.4)))));
    }
  }
}

void main() {
  d = length(uv - .5);
  color = vec4(0.);

  ring();
  circle();

  color.a = pow(color.a, .8);
}
