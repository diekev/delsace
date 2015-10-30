#version 330 core

layout (location = 0) out vec4 fragment_color;
smooth in vec2 UV;
uniform sampler2D image;

void main()
{
	fragment_color = texture(image, UV);
}
