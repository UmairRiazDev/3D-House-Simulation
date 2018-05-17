#version 120
varying vec2 outTex;
varying vec3 outNormal;
varying vec4 outPosition;

uniform vec4 ka;
uniform vec4 kd;
uniform vec4 ks;
uniform float shine;
uniform sampler2D texture_diffuse1;
uniform int hasTexture;
uniform int hasNormal;

void main(void)
{
	if (hasTexture == 1)
	{
		if (hasNormal == 1)
		{
			// assuming light in eye position
			vec4 texel = texture2D(texture_diffuse1, outTex);
			vec3 L = normalize(-outPosition.xyz);
			vec3 N = normalize(outNormal);
			vec3 V = normalize(-outPosition.xyz);
			vec3 R = normalize(reflect(L, N));

			float diff_coef = abs(dot(N, L));
			float spec_coef = pow(abs(dot(R, V)), shine);

			gl_FragColor = ka + (texel * (kd*diff_coef + ks * spec_coef));
			gl_FragColor.a = 1.0;
		}
		else
			gl_FragColor = kd * texture2D(texture_diffuse1, outTex);
	}
	else if (hasNormal == 1)
	{
		// assuming light in eye position
		vec3 L = normalize(-outPosition.xyz);
		vec3 N = normalize(outNormal);
		vec3 V = normalize(-outPosition.xyz);
		vec3 R = normalize(reflect(L, N));

		float diff_coef = dot(N, L);
		float spec_coef = pow(abs(dot(R, V)), shine);

		gl_FragColor = ka + diff_coef * kd + spec_coef * ks;
		gl_FragColor.a = 1.0;
	}
	else
		gl_FragColor = kd;
}