#version 100

precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

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

    gl_FragColor = r * fragColor;
  }
  if (gl_FragColor.a < .05) {
    gl_FragColor.a = 0.;
  }
}

void circle() {
  if (gl_FragColor.a <= 0.) {
    float border = smoothstep(.5 - ringWidth, .5 - ringWidth * 1.8, d);
    float c = circ(d / .5);
    gl_FragColor += border * fragColor * (1. - c);
    if (border >= .6) {
      float shade = lerp(0., 1., length(uv - vec2(1., 0.)));
      gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(shade / (1.0 + .2 * abs(sin(time * 3. - 2.4)))));
    }
    if (isPrevious && !isCurrent) {
      gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(c + .1));
    }
  }
}

void main() {
  d = length(uv - .5);
  gl_FragColor = vec4(0.);

  ring();
  circle();

  gl_FragColor.a = pow(gl_FragColor.a, 1.2);
  float postLevels = 4.;
  gl_FragColor.rgb *= postLevels;
  gl_FragColor.rgb = floor(gl_FragColor.rgb);
  gl_FragColor.rgb /= postLevels;
}
