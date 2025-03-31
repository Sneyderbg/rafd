#version 420 core

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 color;

uniform vec2 resolution;
uniform vec2 cam_target;

void main() {
    vec2 uv = gl_FragCoord.xy / resolution.x;

    uv *= 20.;
    vec2 fr = fract(uv);

    vec2 sm = smoothstep(-.1, .1, fr + .05);
    float grid = sm.x * sm.y;
    color = vec4(vec3(grid), 1.);
}
