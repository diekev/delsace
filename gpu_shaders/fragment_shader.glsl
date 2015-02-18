#version 330 core

out vec4 vFragColor;
smooth in vec2 vUV;
uniform sampler2D image;

void main()
{
	vFragColor = texture2D(image, vUV);
}
