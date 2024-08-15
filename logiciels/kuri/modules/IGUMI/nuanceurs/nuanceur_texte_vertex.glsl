#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 couleur;
layout(location = 2) in vec2 uv;

out vec2 UV;
out vec4 couleur_vertex;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(position, 1.0);
    UV = uv;
    couleur_vertex = couleur;
}
