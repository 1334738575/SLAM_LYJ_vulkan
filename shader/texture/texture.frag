#version 450

// layout (binding = 3) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;

// layout (location = 0) out vec4 outFragColor0;
layout (location = 0) out vec4 outFragColor1;
layout (location = 1) out uint outFragColor2;
layout (location = 2) out vec4 outFragColor3;

void main() 
{
	// vec4 color = texture(samplerColor, inUV, 0);
	vec4 color = vec4(1,0,0,0);
	// outFragColor0 = color;//vec4(1,1,1,0);
	outFragColor1 = color;//uvec4(255,255,255,255);
	// outFragColor2 = color;//uvec4(255,255,255,255);
	outFragColor2 = gl_PrimitiveID + 1;
	outFragColor3 = color;//uvec4(255,255,255,255);
	gl_FragDepth = gl_FragCoord.z;
}