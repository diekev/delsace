#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 couleur;

smooth out vec4 couleur_vertex;

void main()
{
    vec3 P = position;
    gl_Position = vec4(P * 2.0 - 1.0, 1.0);
    couleur_vertex = couleur;
}
