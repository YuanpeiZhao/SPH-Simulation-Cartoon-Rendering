#version 430

layout(location=0) in vec4 currentPos;
layout(location=1) in float nCount;
layout(location=2) in vec4 dCs;

out float vNeighbor;
out vec3 vDeltaCs;

void main(void){
	vNeighbor = nCount;
	vDeltaCs = dCs.xyz;
	gl_Position = currentPos;
	//gl_PointSize = 40.0f;
}