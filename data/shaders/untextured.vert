// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#version 330

uniform vec3 gpu_3d_Vertex;
uniform vec4 gpu_3d_Color;
uniform mat4 gpu_3d_ModelViewProjectionMatrix;

out vec4 color;

void main(void)
{
    color = gpu_3d_Color;
    gl_Position = gpu_3d_ModelViewProjectionMatrix * vec4(gpu_3d_Vertex, 1.0);
}