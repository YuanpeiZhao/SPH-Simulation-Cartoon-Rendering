#version 430

in vec2 ex_TexCoor;
in float ex_Neighbor;
in vec3 ex_DeltaCs;

out vec4 color;

uniform sampler2D texture;
uniform float time;

vec3 CalcParaLight(vec3 lightDir)
{
    vec3 paraLightDir = normalize(lightDir);
    float diff = max(dot(ex_DeltaCs, paraLightDir), 0.0);

    vec3 ambient  = vec3(0.6f, 0.6f, 1.0f)  * 0.4f;
    vec3 diffuse  = vec3(1.0f, 1.0f, 1.0f)  * diff * 0.6f;

    return (ambient + diffuse);
}

void main(void){
 
	if (distance(ex_TexCoor, vec2(0.5, 0.5)) > 0.5) {
        discard;
    }

	if(ex_Neighbor < 6)
	{
		color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		color = vec4(CalcParaLight(vec3(2.0f, -1.0f, 1.0f)), 1.0f);
	}
	
}