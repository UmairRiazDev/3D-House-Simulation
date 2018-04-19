#version 330 core

in vec2 outTex;
in vec3 outNormal;
in vec4 outPosition;

out vec4 color;

uniform vec4 ka;
uniform vec4 kd;
uniform vec4 ks;
uniform float shine;
uniform sampler2D texture_diffuse1;
uniform int hasTexture;
uniform int hasNormal;

void main()
{
    if (hasTexture == 1)
    {
        if (hasNormal == 1)
        {
            // assuming light in eye position
            vec4 texel = texture(texture_diffuse1, outTex);
            vec3 L = normalize(-outPosition.xyz);
            vec3 N = normalize(outNormal);
            vec3 V = normalize(-outPosition.xyz);
            vec3 R = normalize(reflect(L, N));

            float diff_coef = abs(dot(N, L));
            float spec_coef = pow(abs(dot(R, V)), shine);

            color = ka + (texel * (kd*diff_coef + ks*spec_coef));
            color.a = 1.0;
        }
        else
            color = kd * texture(texture_diffuse1, outTex);
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

        color = ka + diff_coef  * kd + spec_coef * ks;
        color.a = 1.0;
    }
    else
        color = kd;

}