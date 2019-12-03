#version 330 core

layout(location = 0) out vec4 fragment_color;
smooth in vec3 UV;
uniform sampler3D volume;
uniform float scale;

vec3 lumiere1 = vec3(1.0, 1.0, 1.0);
vec3 lumiere2 = vec3(-1.0, -1.0, -1.0);

float fonction(in vec3 x)
{
	return texture(volume, x).r;
}

/* gardée pour référence
vec3 calcul_normal(in vec3 x, in float eps)
{
	vec2 e = vec2(eps, 0.0);
	return normalize(vec3(fonction(x + e.xyy) - fonction(x - e.xyy),
						  fonction(x + e.yxy) - fonction(x - e.yxy),
						  fonction(x + e.yyx) - fonction(x - e.yyx)));
}
*/

/* utilise un gradient directionnel voir
  http://www.iquilezles.org/www/articles/derivative/derivative.htm
 */
float calcul_lumiere(in vec3 lumiere, in float den, in float eps, in float puissance)
{
	vec3 dir = normalize(UV - lumiere);
	//vec3 nor = calcul_normal(UV, eps);
	//float dif = clamp(dot(nor, dir), 0.0, 1.0);
	float dif = clamp(0.5 * (1.0 + (fonction(UV + eps * dir) - den) / eps), 0.0, 1.0);

	return dif * puissance;
}

void main()
{
	float eps = 0.01;

	float den = fonction(UV);
	float dif = calcul_lumiere(lumiere1, den, eps, 1.0);
	dif += calcul_lumiere(lumiere2, den, eps, 0.2);

	fragment_color = vec4(dif, dif, dif, den);
}
