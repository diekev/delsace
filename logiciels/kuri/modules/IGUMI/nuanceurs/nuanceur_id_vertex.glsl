#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in uint id;

flat out uint id_vertex;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(position, 1.0);
    id_vertex = id;
}
