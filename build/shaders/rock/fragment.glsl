#version 330 core
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform bool showNormals;

out vec4 FragColor;

struct Material {
	sampler2D texture_diffuse1;
};

struct DirLight {
	vec3 direction;
	
	vec3 ambient;
	vec3 diffuse;
};

uniform Material material;

uniform DirLight light;
uniform vec3 viewPos;

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir);

void main(){
	vec3 normal = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);
	if(showNormals) FragColor = vec4(normal,1.0);
	else FragColor = vec4(calcDirLight(light, normal, viewDir), 1.0);
}

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir){
	vec3 lightDir = normalize(-light.direction);
	float diff = max(dot(normal, lightDir), 0.0);

	vec3 ambient = light.ambient * texture(material.texture_diffuse1, TexCoords).rgb;
	vec3 diffuse = light.diffuse * diff * texture(material.texture_diffuse1, TexCoords).rgb;

	return (ambient + diffuse);
}
