#version 430 core

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



VertexData getVertex(int index)
{
	VertexData vrt = vertices.vertex[index];
	vec4 _gl_pos = vw_matrix * ml_matrix * vec4(vrt.pos, 1.0f);
	vrt.pos = _gl_pos.xyz;
	return vrt;
}
bool rayTriangleIntersect(in vec3 origin, in vec3 direction, int index_buffer_first, out float t, out float u, out float v);

struct RayData
{
	vec3 dir;
	vec3 origin;
	vec4 color;

	int last_triangle;
};


void ray_scene(in RayData ray, out int triangle_index, out vec3 hit_data)
{
	triangle_index = -1;
	float t = 1000.0f, u, v;
	for(int i = 0; i < triangle_count; i++)
	{
		if (ray.last_triangle == i) continue;

		// orthographic rays
		float _t, _u, _v;
		if (rayTriangleIntersect(ray.origin, ray.dir, i * 3, _t, _u, _v))
		{
			if (_t > 0.0f && _t < t) // front of the camera only and only points that are closer to the camera
			{
				t = _t; u = _u; v = _v;
				triangle_index = i;
			}
		}
	}

	hit_data = vec3(t, u, v);
}

void main(void) {
	// ray dir calculating
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(framebuffer);
	if (pix.x >= size.x || pix.y >= size.y) {
		return;
	}
	vec2 pos = (2.0f * vec2(pix) / vec2(size.x - 1, size.y - 1)) - 1.0f;
	pos.y *= -1.0f;

	// ray tracing
	RayData ray;
	ray.dir = normalize(vec3(pos, -1.0f));
	ray.origin = vec3(0.0f, 0.0f, 1.0f);
	ray.color = vec4(1.0f);
	ray.last_triangle = -1;
	const int bounces = 16;
	for(int i = 0; i < bounces; i++)
	{
		int triangle_index = -1;
		vec3 hit_data;
		ray_scene(ray, triangle_index, hit_data);
		ray.last_triangle = triangle_index;
		
		if (triangle_index == -1)
		{
			// miss
			break;
		}
		else
		{
			// triangle hit
			float first = 1.0f-hit_data.y-hit_data.z;
			float second = hit_data.y;
			float third = hit_data.z;
			VertexData v0 = getVertex(triangle_index * 3 + 0);
			VertexData v1 = getVertex(triangle_index * 3 + 1);
			VertexData v2 = getVertex(triangle_index * 3 + 2);

			vec2 uv_t = vec2(0.0f);
			uv_t += v0.uv_t * first;
			uv_t += v1.uv_t * second;
			uv_t += v2.uv_t * third;

			vec4 col = vec4(0.0f);
			col += v0.color * first;
			col += v1.color * second;
			col += v2.color * third;

			ray.color *= texture(texture_sampler, uv_t) * col;

			// bounce or not?
			vec2 uv_r = vec2(0.0f);
			uv_r += v0.uv_r * first;
			uv_r += v1.uv_r * second;
			uv_r += v2.uv_r * third;
			float r = texture(texture_sampler, uv_r).r;
			if (r > kEpsilon) {
				// normal
				vec3 vA = v1.pos - v0.pos;
				vec3 vB = v2.pos - v0.pos;
				vec3 n = vec3(
					vA.y * vB.z - vA.z * vB.y, 
					vA.z * vB.x - vA.x * vB.z, 
					vA.x * vB.y - vA.y * vB.x
				);
				
				vec3 hitpos = ray.origin + hit_data.x * ray.dir;

				ray.origin = hitpos;
				ray.dir = reflect(ray.dir, n);
				ray.color *= r;
				continue;
			}
			break;
		}
	}


    
	// store pixel result
	imageStore(framebuffer, pix, ray.color);
}

bool rayTriangleIntersect(in vec3 origin, in vec3 direction, int index_first, out float t, out float u, out float v)
{

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