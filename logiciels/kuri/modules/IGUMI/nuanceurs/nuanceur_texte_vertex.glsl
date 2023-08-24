#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 couleur;
layout(location = 2) in vec2 uv;

smooth out vec2 UV;
smooth out vec4 couleur_vertex;

uniform int largeur_fenetre = 0;
uniform int hauteur_fenetre = 0;

void main()
{
    vec3 P = position;
    P.x /= float(largeur_fenetre);
    P.y /= float(hauteur_fenetre);
    gl_Position = vec4(P * 2.0 - 1.0, 1.0);
    UV = uv;
    couleur_vertex = couleur;
}
