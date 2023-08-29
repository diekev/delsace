#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

smooth out vec2 UV;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(position, 1.0);
    UV = vec2(uv.x, 1.0 - uv.y);
}
