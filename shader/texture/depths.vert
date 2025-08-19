#version 450

layout (location = 0) in vec3 uvds;
// layout (location = 1) in vec3 normals;

layout (binding = 0) uniform UBO 
{
    uint halfW;
    uint halfH;
    float maxD;
} ubo;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() 
{
	gl_Position.x = (uvds.x - ubo.halfW) / ubo.halfW;
	gl_Position.y = (uvds.y - ubo.halfH) / ubo.halfH;
	gl_Position.z = uvds.z / ubo.maxD;
	gl_Position.w = 1.0f;
}
