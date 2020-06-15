struct VertexData
{
	vec3 pos;
	float pad0;
	vec2 uv_t;
	float pad1[2];
	vec2 uv_r;
	float pad2[2];
	vec4 color;
};

struct RayData
{
	vec3 dir;
	vec3 origin;
	vec4 color;

	int last_triangle;
};

layout(binding = 0, rgba32f) uniform image2D framebuffer;
layout (std430, binding = 1) readonly buffer vertex_buffer
{
	VertexData vertex[1000];
} vertices;
layout(local_size_x = 16, local_size_y = 16) in;

uniform sampler2D texture_sampler;
uniform int triangle_count = 0;
uniform mat4 ml_matrix = mat4(1.0f);
uniform mat4 vw_matrix = mat4(1.0f);
const float kEpsilon = 0.0001f;