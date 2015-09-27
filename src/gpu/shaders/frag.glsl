#version 330 core

layout (location = 0) out vec4 vFragColor;
smooth in vec2 vUV;
uniform sampler2D image;

void main()
{
	vFragColor = texture(image, vUV);
}
