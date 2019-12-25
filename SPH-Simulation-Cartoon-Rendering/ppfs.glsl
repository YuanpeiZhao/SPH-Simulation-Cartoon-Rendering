#version 430

in vec2 TexCoord;

out vec4 color;

uniform sampler2D depthMap;

const vec3 sobelX[3] ={
	vec3(-1, 0, 1),
	vec3(-2, 0, 2),
	vec3(-1, 0, 1)
};

const vec3 sobelY[3] ={
	vec3( 1,  2,  1),
	vec3( 0,  0,  0),
	vec3(-1, -2, -1)
};

const float offsetX = 1.0f / 1024.0f;
const float offsetY = 1.0f / 768.0f;

float degreeX()
{
	float result = 0.0f;
	for(int i = -1; i < 2; i ++)
	{
		for(int j = -1; j < 2; j ++)
		{
			result += sobelX[i+1][j+1] * texture(depthMap, vec2(clamp(TexCoord.x + i * offsetX, 0.0f, 1.0f), clamp(TexCoord.y - j * offsetY, 0.0f, 1.0f))).r;
		}
	}
	return result;
}

float degreeY()
{
	float result = 0.0f;
	for(int i = -1; i < 2; i ++)
	{
		for(int j = -1; j < 2; j ++)
		{
			result += sobelY[i+1][j+1] * texture(depthMap, vec2(clamp(TexCoord.x + i * offsetX, 0.0f, 1.0f), clamp(TexCoord.y + j * offsetY, 0.0f, 1.0f))).r;
		}
	}
	return result;
}

void main(void){

	float degree = sqrt(pow(degreeX(), 2) + pow(degreeY(), 2));
	float depthValue = texture(depthMap, TexCoord).r;

	if(degree < 0.02) discard;
	
	color = vec4(vec3(0.0, 0.0, 0.0), 1.0);
}