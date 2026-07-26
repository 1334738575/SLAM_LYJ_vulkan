#version 450

layout (location = 0) out uint fId;

layout (binding = 0) uniform UBO 
{
    float halfW;
    float halfH;
    float maxD;
    float csTh;
    uint useFaceIds;
    uint usePointIds;
    uint padding1;
    uint padding2;
} ubo;

layout(binding = 1) buffer NormalData
{
    float fncs[];
};

layout(binding = 2) buffer FaceIdData
{
    uint faceIds[];
};

layout(binding = 3) buffer FaceData
{
    uint faces[];
};

layout(binding = 4) buffer PointMaskData
{
    uint pointMask[];
};

void main() 
{
    uint fid = uint(gl_PrimitiveID);
    if (ubo.useFaceIds != 0)
        fid = faceIds[fid];
    if (ubo.usePointIds != 0) {
        uint p0 = faces[fid * 3 + 0];
        uint p1 = faces[fid * 3 + 1];
        uint p2 = faces[fid * 3 + 2];
        if (pointMask[p0] == 0 || pointMask[p1] == 0 || pointMask[p2] == 0)
            discard;
    }
    if (fncs[fid * 3 + 2] >= ubo.csTh)
        discard;

	fId = gl_PrimitiveID + 1;
	gl_FragDepth = gl_FragCoord.z;
}
