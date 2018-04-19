#version 330 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTex;

out vec2 outTex;
out vec3 outNormal;
out vec4 outPosition;

uniform mat4 model;
uniform mat4 normalMat;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	mat4 modelView = view * model;
    gl_Position = projection * modelView * vec4(inPosition, 1.0f);
	outNormal = normalize((view * normalMat * vec4(inNormal, 0.0)).xyz);
	outPosition   = modelView * vec4(inPosition, 1.0);
    outTex = inTex;
}