#version 120
uniform mat4 model;
uniform mat4 normalMat;
uniform mat4 view;
uniform mat4 projection;

attribute vec3 inPosition;
attribute vec3 inNormal;
attribute vec2 inTex;

varying vec2 outTex;
varying vec3 outNormal;
varying vec4 outPosition;
void main(void)
{
	mat4 modelView = view * model;
	gl_Position = projection * modelView * vec4(inPosition, 1.0f);
	outNormal = normalize((view * normalMat * vec4(inNormal, 0.0)).xyz);
	outPosition = modelView * vec4(inPosition, 1.0);
	outTex = inTex;
}