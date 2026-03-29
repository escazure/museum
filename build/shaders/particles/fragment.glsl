#version 330 core

in vec3 fragPos;

uniform float cone_height;
uniform float time;

out vec4 FragColor;

void main(){
	float t = clamp(fragPos.y/cone_height, 0.0, 1.0);
	float opacity = 1.0 - smoothstep(-0.7, 1.0, t);

	FragColor = vec4(0.0, 1.0, 1.0, opacity);
}
