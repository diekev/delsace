#version 330 core

out vec4 FragColor;

uniform sampler2D VelocityTexture;
uniform sampler2D source;
uniform sampler2D obstacles;

uniform vec2 inverse_size;
uniform float dt;
uniform float dissipation;

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    float solid = texture2D(obstacles, inverse_size * fragCoord).x;

    if (solid > 0) {
        FragColor = vec4(0);
        return;
    }

    vec2 u = texture2D(VelocityTexture, inverse_size * fragCoord).xy;
    vec2 coord = inverse_size * (fragCoord - dt * u);

    FragColor = dissipation * texture2D(source, coord);
}
