#version 330 core

out float FragColor;

uniform sampler2D Velocity;
uniform sampler2D obstacles;
uniform float half_inverse_dh;

void main()
{
    ivec2 T = ivec2(gl_FragCoord.xy);

    // Find neighboring velocities:
    vec2 vN = texelFetchOffset(Velocity, T, 0, ivec2(0, 1)).xy;
    vec2 vS = texelFetchOffset(Velocity, T, 0, ivec2(0, -1)).xy;
    vec2 vE = texelFetchOffset(Velocity, T, 0, ivec2(1, 0)).xy;
    vec2 vW = texelFetchOffset(Velocity, T, 0, ivec2(-1, 0)).xy;

    // Find neighboring obstacles:
    vec3 oN = texelFetchOffset(obstacles, T, 0, ivec2(0, 1)).xyz;
    vec3 oS = texelFetchOffset(obstacles, T, 0, ivec2(0, -1)).xyz;
    vec3 oE = texelFetchOffset(obstacles, T, 0, ivec2(1, 0)).xyz;
    vec3 oW = texelFetchOffset(obstacles, T, 0, ivec2(-1, 0)).xyz;

    // Use obstacle velocities for solid cells:
    if (oN.x > 0) vN = oN.yz;
    if (oS.x > 0) vS = oS.yz;
    if (oE.x > 0) vE = oE.yz;
    if (oW.x > 0) vW = oW.yz;

    FragColor = half_inverse_dh * (vE.x - vW.x + vN.y - vS.y);
}
