// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#version 330

in vec2 gpu_Vertex;
in vec2 gpu_TexCoord;
in vec4 gpu_Color;
uniform mat4 gpu_ModelViewProjectionMatrix;

out vec4 color;
out vec2 texCoord;

void main(void)
{
	color = gpu_Color;
	texCoord = vec2(gpu_TexCoord);
	gl_Position = gpu_ModelViewProjectionMatrix * vec4(gpu_Vertex, 0.0, 1.0);
}