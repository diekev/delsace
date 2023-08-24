#version 330 core
layout (location = 0) out vec4 couleur_fragment;

smooth in vec4 couleur_vertex;

void main()
{
    couleur_fragment = couleur_vertex;
}
