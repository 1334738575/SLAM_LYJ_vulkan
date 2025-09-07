#version 450

layout (location = 0) out uint fId;

layout (binding = 0) uniform UBO 
{
    float halfW;
    float halfH;
    float maxD;
} ubo;

void main() 
{
	fId = gl_PrimitiveID + 1;
	gl_FragDepth = gl_FragCoord.z;
}