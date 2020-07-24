#version 430 core

#include "compute-rays.extra.glsl"



float shadow_rays(in RayData ray)
{
	float light_access = 0.0f;

	for(int i = 0; i < sphere_count; i++)
	{
		if (ray.last_hit == i && ray.last_type == true) continue;

		SphereObj light_src = getSphere(i);

		vec3 dir = light_src.pos - ray.hit_pos;

		float _t;
		if (raySphereIntersect(ray.origin, dir, i, _t))
		{
			light_access += light_src.extra1.w / _t;
		}
	}

	return light_access; // broken, notices lights behind other lights and every light source is just a point light
}

bool ray_scene(inout RayData ray)
{
	int triangle_index = -1;
	int sphere_index = -1;
	bool type = false;

	float t = 1000.0f, u, v;
	// triangles
	for(int i = 0; i < triangle_count; i++)
	{
		if (ray.last_hit == i && ray.last_type == false) continue;

		float _t, _u, _v;
		if (rayTriangleIntersect(ray.origin, ray.dir, i * 3, _t, _u, _v))
		{
			if (_t > 0.0f && _t < t) // front of the camera only and only points that are closer to the camera
			{
				t = _t; u = _u; v = _v;
				triangle_index = i;
				type = false;
			}
		}
	}
	for(int i = 0; i < sphere_count; i++)
	{
		if (ray.last_hit == i && ray.last_type == true) continue;

		float _t;
		if (raySphereIntersect(ray.origin, ray.dir, i, _t))
		{
			if (_t > 0.0f && _t < t) // front of the camera only and only points that are closer to the camera
			{
				t = _t;
				sphere_index = i;
				type = true;
			}
		}
	}

	if (triangle_index == -1 && sphere_index == -1) return false;

	ray.last_type = type;
	if (type) ray.last_hit = sphere_index;
	else ray.last_hit = triangle_index;

	ray.hit_pos = ray.origin + ray.dir * t;
	if (type)
	{
		normal_sphere(getSphere(sphere_index).pos, ray.hit_pos, ray.hit_normal);
		ray.hit_data = vec3(t, 0.0f, 0.0f);
	}
	else
	{
		VertexData v0 = getVertex(triangle_index * 3 + 0);
		VertexData v1 = getVertex(triangle_index * 3 + 1);
		VertexData v2 = getVertex(triangle_index * 3 + 2);
		
		vec3 vertices[3] = { v0.pos, v1.pos, v2.pos };
		normal_triangle(vertices, ray.hit_normal);
		ray.hit_data = vec3(t, u, v);
	}

	return true;
}

vec4 drawmode_normals(in vec2 pos)
{
	// init ray
	RayData ray = constructRayData(pos);

	// do ray
	bool hit = ray_scene(ray);
	if (!hit) return vec4(0.0f); // miss

	return vec4(ray.hit_normal, 1.0f);
}

vec4 drawmode_regular(in vec2 pos)
{
	// init ray
	RayData ray = constructRayData(pos);
	const int bounces = 16;
	for(int i = 0; i < bounces; i++)
	{
		bool hit = ray_scene(ray);
		
		if (hit)
		{
			float r, l;
			if (!ray.last_type) // triangle
			{
				float first = 1.0f-ray.hit_data.y-ray.hit_data.z;
				float second = ray.hit_data.y;
				float third = ray.hit_data.z;
				
				// vertices
				VertexData v0 = getVertex(ray.last_hit * 3 + 0);
				VertexData v1 = getVertex(ray.last_hit * 3 + 1);
				VertexData v2 = getVertex(ray.last_hit * 3 + 2);
				
				// texture
				vec2 uv_t = vec2(0.0f);
				uv_t += v0.uv_t * first;
				uv_t += v1.uv_t * second;
				uv_t += v2.uv_t * third;

				// color
				vec4 col = vec4(0.0f);
				col += v0.color * first;
				col += v1.color * second;
				col += v2.color * third;
				
				ray.color *= texture(texture_sampler, uv_t) * col;

				// reflectivity
				vec2 uv_r = vec2(0.0f);
				uv_r += v0.uv_r * first;
				uv_r += v1.uv_r * second;
				uv_r += v2.uv_r * third;
				r = texture(texture_sampler, uv_r).r;

				l = 0.0f;
			}
			else // sphere
			{
				SphereObj s = getSphere(ray.last_hit);
				// color
				ray.color *= s.color;
				r = s.extra1.y;
				l = s.extra1.w;
			}

			// shadowray
			float light_access = shadow_rays(ray);
			ray.color_comb += ray.color * light_access * (1.0f - r);

			// luminocity
			if (l > kEpsilon)
			{
				// break;
			}
			
			// reflection
			if (r > kEpsilon) {
				// normal
				ray.origin = ray.hit_pos;
				ray.dir = reflect(ray.dir, ray.hit_normal);
				ray.color *= r;
				continue;
			}
			else
			{
				hit = false;
			}
		}
		// not elif so i can hack ambient color when not bouncing
		if (!hit) // miss
		{
			// ambient color
			ray.color_comb += ray.color * vec4(0.0f, 0.0f, 0.0f, 1.0f);
			break;
		}
	}

	return ray.color;
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
	vec4 color;
	switch(normaldraw)
	{
	case 0:
		color = drawmode_regular(pos);
		break;
	default:
		color = drawmode_normals(pos);
		break;
	}
    
	// store pixel result
	imageStore(framebuffer, pix, color);
}