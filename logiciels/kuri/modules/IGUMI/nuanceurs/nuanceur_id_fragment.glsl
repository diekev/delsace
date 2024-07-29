#version 330 core
layout (location = 0) out uint id_fragment;

flat in uint id_vertex;

void main()
{
    id_fragment = id_vertex;
}
