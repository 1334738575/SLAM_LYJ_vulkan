#version 450

layout (binding = 3) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor0;
layout (location = 1) out vec4 outFragColor1;
layout (location = 2) out vec4 outFragColor2;

void main() 
{
	vec4 color = texture(samplerColor, inUV, 0);
	outFragColor0 = color;
	outFragColor1 = color;//uvec4(255,255,255,255);
	outFragColor2 = color;//uvec4(255,255,255,255);
}