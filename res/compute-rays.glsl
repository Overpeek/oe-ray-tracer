#version 430 core

struct VertexData
{
	vec3 pos;
	vec2 uv;
	vec4 color;
};

layout(binding = 0, rgba32f) uniform image2D framebuffer;
layout (std430, binding = 1) readonly buffer vertex_buffer
{
	VertexData vertex[1000];
} vertices;
layout(local_size_x = 16, local_size_y = 16) in;

uniform int triangle_count = 0;
uniform mat4 ml_matrix = mat4(1.0f);
uniform mat4 vw_matrix = mat4(1.0f);



VertexData getVertex(int index)
{
	VertexData vrt = vertices.vertex[index];
	vrt.pos = (ml_matrix * vw_matrix * vec4(vrt.pos, 0.0f)).xyz;
	return vrt;
}



bool rayTriangleIntersect(in vec3 origin, in vec3 direction, int index_buffer_first, out float t, out float u, out float v);

void main(void) {
	// ray dir calculating
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(framebuffer);
	if (pix.x >= size.x || pix.y >= size.y) {
		return;
	}
	vec2 pos = (2.0f * vec2(pix) / vec2(size.x - 1, size.y - 1)) - 1.0f;
	vec3 ray = normalize(vec3(pos, 1.0f));

	// ray tracing
	vec4 color = vec4(0.0f, 0.0f, 0.0f, 0.5f);
	for(int i = 0; i < triangle_count; i++)
	{
		float t, u, v;
		// origin ray and direction ray, should result in orthographic like parallel rays
		if (rayTriangleIntersect(vec3(pos, 0.0f), ray, i * 3, t, u, v))
		{
			// getVertex(i * 3).verte;
			// first    1.0f-u-v
			// second   v
			// third    u
			vec4 c = vec4(0.0f, 0.0f, 0.0f, 0.0f);
			c += getVertex(i * 3 + 0).color * (1.0f-u-v);
			c += getVertex(i * 3 + 1).color * (v);
			c += getVertex(i * 3 + 2).color * (u);

			color = vec4(c.rgb, 1.0f);
			break;
		}
	}
    
	// store pixel result
	imageStore(framebuffer, pix, color);
}

bool rayTriangleIntersect(in vec3 origin, in vec3 direction, int index_first, out float t, out float u, out float v)
{
	const float kEpsilon = 0.0001f;

	vec3 v0 = getVertex(index_first + 0).pos;
	vec3 v1 = getVertex(index_first + 1).pos;
	vec3 v2 = getVertex(index_first + 2).pos;

    vec3 v0v1 = v1 - v0;
    vec3 v0v2 = v2 - v0;
    vec3 pvec = cross(direction, v0v2);
    float det = dot(v0v1, pvec);

	// cull mode
    // if (det < kEpsilon) return false;
    if (abs(det) < kEpsilon) return false;


    float invDet = 1.0f / det;
 
    vec3 tvec = origin - v0;
    u = dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f) return false;
 
    vec3 qvec = cross(tvec, v0v1);
    v = dot(direction, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;
 
    t = dot(v0v2, qvec) * invDet;
 
    return true;
} 