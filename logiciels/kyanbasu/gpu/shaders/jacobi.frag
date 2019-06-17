#version 330 core

out vec4 FragColor;

uniform sampler2D pressure;
uniform sampler2D divergence;
uniform sampler2D obstacles;

uniform float alpha;
uniform float inverse_beta;

void main()
{
    ivec2 T = ivec2(gl_FragCoord.xy);

    // Find neighboring pressure:
    vec4 pN = texelFetchOffset(pressure, T, 0, ivec2(0, 1));
    vec4 pS = texelFetchOffset(pressure, T, 0, ivec2(0, -1));
    vec4 pE = texelFetchOffset(pressure, T, 0, ivec2(1, 0));
    vec4 pW = texelFetchOffset(pressure, T, 0, ivec2(-1, 0));
    vec4 pC = texelFetch(pressure, T, 0);

    // Find neighboring obstacles:
    vec3 oN = texelFetchOffset(obstacles, T, 0, ivec2(0, 1)).xyz;
    vec3 oS = texelFetchOffset(obstacles, T, 0, ivec2(0, -1)).xyz;
    vec3 oE = texelFetchOffset(obstacles, T, 0, ivec2(1, 0)).xyz;
    vec3 oW = texelFetchOffset(obstacles, T, 0, ivec2(-1, 0)).xyz;

    // Use center pressure for solid cells:
    if (oN.x > 0) pN = pC;
    if (oS.x > 0) pS = pC;
    if (oE.x > 0) pE = pC;
    if (oW.x > 0) pW = pC;

    vec4 bC = texelFetch(divergence, T, 0);
    FragColor = (pW + pE + pS + pN + alpha * bC) * inverse_beta;
}
