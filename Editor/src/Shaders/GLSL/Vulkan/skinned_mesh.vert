#version 460
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outWorldPos;

struct Vertex {

	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
	ivec4 boneIds;
	vec4 weights;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

//push constants block
layout( push_constant ) uniform constants
{	
	mat4 mvp_matrix;
	mat4 model_matrix;
	VertexBuffer vertexBuffer;
} PushConstants;

layout(set = 0, binding = 0) uniform SkeletalAnimationData
{   
	mat4 finalBoneMatrices[MAX_BONES];
} saData;

void main() 
{	
	//load vertex data from device adress
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	vec4 totalPosition = vec4(0.0f);
	vec3 totalNormal = vec3(0.0f);
    for(int i = 0 ; i < MAX_BONE_INFLUENCE; i++)
    {
        if(v.boneIds[i] == -1) 
            continue;
        if(v.boneIds[i] >=MAX_BONES) 
        {
            totalPosition = vec4(v.position, 1.0f);
            break;
        }
        vec4 localPosition = saData.finalBoneMatrices[v.boneIds[i]] * vec4(v.position, 1.0f);
        totalPosition += localPosition * v.weights[i];
		vec3 localNormal = mat3(saData.finalBoneMatrices[v.boneIds[i]]) * v.normal;
        totalNormal += localNormal * v.weights[i];
    }
	if (totalPosition == vec4(0.0f))
		totalPosition = vec4(v.position, 1.0f);
	if (totalNormal == vec3(0.0f))
		totalNormal = v.normal;

	//output data
	gl_Position = PushConstants.mvp_matrix * totalPosition;
	outColor = v.color.xyz;
	outUV.x = v.uv_x;
	outUV.y = v.uv_y;
	outNormal = normalize(mat3(transpose(inverse(PushConstants.model_matrix))) * totalNormal);
	outWorldPos = (PushConstants.model_matrix * totalPosition).xyz;
}