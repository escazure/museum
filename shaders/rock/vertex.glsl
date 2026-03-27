#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in mat4 instanceMatrix;

layout (std140) uniform Matrices {
	mat4 projection;
	mat4 view;
};

uniform float time;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

void main(){
	TexCoords = aTexCoords;	
	Normal = mat3(transpose(inverse(instanceMatrix))) * aNormal;

	vec3 pivot = vec3(0.0, 0.0, 0.0);
	vec4 worldPos = instanceMatrix * vec4(aPos, 1.0);
	float s = sin(time);
	float c = cos(time);

	vec3 pos = worldPos.xyz - pivot;
	float x = pos.x * c - pos.z * s;
	float z = pos.x * s + pos.z * c;
	pos.x = x;
	pos.z = z;
	worldPos.xyz = pos + pivot;

	FragPos = worldPos.xyz;

	gl_Position = projection * view * worldPos;
}
