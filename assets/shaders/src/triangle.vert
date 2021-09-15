
#version 450

layout(binding = 0) uniform Transformations {
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout (location = 0) out vec3 fragColor;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    fragColor = color;
}