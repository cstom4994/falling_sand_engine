// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#version 330

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D tex;

void main(void)
{
	fragColor = texture(tex, texCoord) * color;
}