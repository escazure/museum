#version 330 core
in vec3 Normal;

out vec4 FragColor;

uniform vec3 lightColor;
uniform bool showNormals;

void main(){
	if(showNormals) FragColor = vec4(normalize(Normal), 1.0);
	else FragColor = vec4(lightColor, 1.0);
}
