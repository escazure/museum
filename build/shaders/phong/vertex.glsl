#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

layout (std140) uniform Matrices {
	mat4 projection;
	mat4 view;
};
uniform mat4 model;
uniform mat4 lightSpaceMatrix;

out vec3 Normal;
out vec2 TexCoords;
out vec3 fragPos;
out vec4 fragLightPos;

void main(){
	vec4 worldPos = model * vec4(aPos, 1.0);
	fragPos = worldPos.xyz;
	fragLightPos = lightSpaceMatrix * worldPos;
	TexCoords = aTexCoords;
	Normal = mat3(transpose(inverse(model))) * aNormal;
	gl_Position = projection * view * worldPos;
}
