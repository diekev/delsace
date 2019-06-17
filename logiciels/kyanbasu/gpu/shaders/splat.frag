#version 330 core

out vec4 FragColor;

uniform vec2 point;
uniform float radius;
uniform vec3 fill_color;

void main()
{
    float d = distance(point, gl_FragCoord.xy);

    if (d < radius) {
        float a = (radius - d) * 0.5;
        a = min(a, 1.0);
        FragColor = vec4(fill_color, a);
    }
	else {
        FragColor = vec4(0);
    }
}
