#version 330 core

out vec2 FragColor;
uniform sampler2D Velocity;
uniform sampler2D temperature;
uniform sampler2D density;
uniform float temperature_ambient;
uniform float dt;
uniform float sigma;
uniform float kappa;

void main()
{
    ivec2 TC = ivec2(gl_FragCoord.xy);
    float T = texelFetch(temperature, TC, 0).r;
    vec2 V = texelFetch(Velocity, TC, 0).xy;

    FragColor = V;

    if (T > temperature_ambient) {
        float D = texelFetch(density, TC, 0).x;
        FragColor += (dt * (T - temperature_ambient) * sigma - D * kappa ) * vec2(0, 1);
    }
}
