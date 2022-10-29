// Copyright(c) 2022, KaoruXun All rights reserved.

#version 150

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D tex;

void main(void)
{
	fragColor = texture2D(tex, texCoord) * color;
}