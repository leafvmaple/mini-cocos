#version 450

// Fragment shader for the Vulkan sprite/text pipeline.
// `fontSampler` is bound at set=0 binding=0 (the per-texture descriptor set).
// For A8 atlas textures the image view swizzle is configured so the sampler
// returns (1, 1, 1, r), letting the same `color * texture` formula handle both
// RGBA sprites and single-channel font glyphs without branching.

layout(set = 0, binding = 0) uniform sampler2D fontSampler;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = inColor * texture(fontSampler, inUV);
}
