// structs
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
struct SphereObj
{
	vec3 pos;
	float pad0;
	vec4 extra0; // x = radius, y = unused, z = unused, w = unused
	vec4 extra1; // x = refractivity, y = reflectivity, z = smoothness, w = luminance
	vec4 color;
};
struct RayData
{
	vec3 dir;
	vec3 origin;
	vec4 color;
	vec4 color_comb;

	vec3 hit_pos;
	vec3 hit_normal;
	vec3 hit_data; // t, u, v

	int last_hit;
	bool last_type; // false == triangle, true == sphere
};
RayData constructRayData(in vec2 pos)
{
	RayData ray;
	ray.dir = normalize(vec3(pos, -1.0f));
	ray.origin = vec3(0.0f, 0.0f, 0.0f);
	ray.color = vec4(1.0f);
	ray.color_comb = vec4(0.0f);
	ray.last_hit = -1;
	ray.last_type = false;
	ray.hit_pos = vec3(0.0f);
	ray.hit_normal = vec3(0.0f);
	ray.hit_data = vec3(0.0f);
	return ray;
}

// layout
layout(binding = 0, rgba32f) uniform image2D framebuffer;
layout (std430, binding = 1) readonly buffer vertex_buffer
{
	VertexData vertex[1000];
} vertices;
layout (std430, binding = 2) readonly buffer sphereobj_buffer
{
	SphereObj sphere[1000];
} spheres;
layout(local_size_x = 16, local_size_y = 16) in;

// uniforms
uniform sampler2D texture_sampler;
uniform int triangle_count = 0;
uniform int sphere_count = 0;
uniform int normaldraw = 0;
uniform mat4 vw_matrix = mat4(1.0f);
const float kEpsilon = 0.0001f;

// getters
VertexData getVertex(int index)
{
	VertexData vrt = vertices.vertex[index];
	vec4 _gl_pos = vw_matrix * vec4(vrt.pos, 1.0f);
	vrt.pos = _gl_pos.xyz;
	return vrt;
}
SphereObj getSphere(int index)
{
	SphereObj sphr = spheres.sphere[index];
	vec4 _gl_pos = vw_matrix * vec4(sphr.pos, 1.0f);
	sphr.pos = _gl_pos.xyz;
	return sphr;
}



// intersections etc.
bool solveQuadratic(in float a, in float b, in float c, out float x0, out float x1)
{
    float discriminant = b*b - 4*a*c;
    if (discriminant < 0.0f) return false; // no solutions
    else if (discriminant == 0.0f) x0 = x1 = - 0.5f * b / a; // one solution
    else { // two solutions
		float root_discr = sqrt(discriminant);
        float q = (b > 0.0f) ? -0.5f * (b + root_discr) : -0.5f * (b - root_discr);
        x0 = q / a;
        x1 = c / q;
    }
    if (x0 > x1) { float tmp = x0; x0 = x1; x1 = tmp; }

    return true;
} 

// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
bool raySphereIntersect(in vec3 origin, in vec3 direction, int index, out float t)
{
	SphereObj sphere = getSphere(index);
	vec3 center = sphere.pos;
	float rad = sphere.extra0.x;

	float t0, t1;
	vec3 L = origin - center;
	float a = dot(direction, direction);
	float b = 2.0f * dot(direction, L);
	float c = dot(L, L) - rad*rad;
	if (!solveQuadratic(a, b, c, t0, t1)) return false;
	if (t0 > t1) { float tmp = t0; t0 = t1; t1 = tmp; }

	t = t0;
	return true; 
} 

// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
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
    if (abs(det) < kEpsilon) return false; // no backface culling


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

void normal_triangle(in vec3 v[3], out vec3 n)
{
	vec3 vA = v[1] - v[0];
	vec3 vB = v[2] - v[0];
	n = normalize(cross(vA, vB));
}

void normal_sphere(in vec3 c, in vec3 point, out vec3 n)
{
	n = normalize(point - c);
}