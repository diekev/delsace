#version 330 core
layout (location = 0) out vec4 couleur_fragment;

uniform sampler2D atlas;
smooth in vec4 couleur_vertex;
smooth in vec2 UV;

void main()
{
    float a = texture2D(atlas, UV).r;
    couleur_fragment = vec4(couleur_vertex.xyz, couleur_vertex.a * a);
}
