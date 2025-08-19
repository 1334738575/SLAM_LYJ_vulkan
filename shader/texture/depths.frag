#version 450

layout (location = 0) out uint fId;

void main() 
{
	fId = gl_PrimitiveID + 1;
	gl_FragDepth = gl_FragCoord.z;
}