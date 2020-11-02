#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec2 pos;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

const float pxRange = 4.0;

void main() {
    vec2 msdfUnit = pxRange / vec2(textureSize(texSampler, 0));
    vec3 fontSample = texture(texSampler, fragTexCoord).rgb;
    float sigDist = median(fontSample.r, fontSample.g, fontSample.b) - 0.5;
    sigDist *= dot(msdfUnit, 0.5 / fwidth(pos));
    float opacity = clamp(sigDist + 0.5, 0.0, 1.0);
    if (opacity < 0.05) {
        discard;
    }
    outColor = vec4(mix(vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0), opacity), opacity);
}
