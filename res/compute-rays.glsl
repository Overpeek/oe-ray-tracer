#version 430 core

layout(binding = 0, rgba32f) uniform image2D framebuffer;



layout (local_size_x = 16, local_size_y = 16) in;
void main(void) {
	ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(framebuffer);
	if (pix.x >= size.x || pix.y >= size.y) {
		return;
	}
	vec2 pos = vec2(pix) / vec2(size.x - 1, size.y - 1);
	// vec3 dir = normalize(mix(mix(ray00, ray01, pos.y), mix(ray10, ray11, pos.y), pos.x));
    
	// rayData ray = rayData(eye, dir, newHitInfo());
	vec4 color = vec4(pos.x, pos.y, 0.0, 1.0);
	imageStore(framebuffer, pix, color);
}
