#version 330 core

layout (location = 0) out vec4 fragment_color;
in vec2 UV;
uniform sampler2D sampler;
uniform vec3 fill_color;
uniform vec2 scale;

void main()
{
    float L = texture2D(sampler, UV).r;
    fragment_color = vec4(fill_color, L);
}
