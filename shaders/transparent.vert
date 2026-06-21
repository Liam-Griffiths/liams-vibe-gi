#version 430 core
// Forward pass for transparent / refractive materials (glass). Runs after the deferred
// opaque composite; outputs view-space position+normal so the fragment shader can sample
// the opaque scene behind it with a refraction offset.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 ViewPos;
out vec3 ViewNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 viewPos = view * model * vec4(aPos, 1.0);
    ViewPos = viewPos.xyz;
    ViewNormal = mat3(transpose(inverse(view * model))) * aNormal;
    gl_Position = projection * viewPos;
}
