#version 330 core
layout (location = 0) out vec4 couleur_fragment;

uniform sampler2D image;
smooth in vec2 UV;

void main()
{
    couleur_fragment = texture2D(image, UV);
}
