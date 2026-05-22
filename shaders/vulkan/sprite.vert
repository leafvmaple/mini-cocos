#version 450

// Vertex shader for the Vulkan sprite/text pipeline.
// Matches the layout consumed by RenderDeviceVulkan:
//   - Push constant: column-major mat4 MVP (sizeof(Mat4) bytes, vertex stage)
//   - Attribute 0: vec2 position (NDC after multiplying by transform)
//   - Attribute 1: vec2 uv
//   - Attribute 2: vec4 color (already premultiplied with per-draw opacity)
// Outputs feed sprite.frag.

layout(push_constant) uniform PushConstants {
    mat4 transform;
} pushConstants;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

void main() {
    outUV = inUV;
    outColor = inColor;
    gl_Position = pushConstants.transform * vec4(inPos, 0.0, 1.0);
}
