#version 330 core

in vec3 Normal;
in vec2 TexCoords;
in vec3 fragPos;
in vec4 fragLightPos;

struct Material {
	sampler2D texture_diffuse1;
	sampler2D texture_specular1;
	float shininess;
};

struct PointLight {
	vec4 position;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

struct DirLight {
	vec4 direction;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

struct SpotLight {
	vec4 position;
	vec4 direction;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

layout (std140) uniform Light {
	PointLight point_light;
	PointLight point_light2;
	DirLight dir_light;
	SpotLight spot_light;
};

uniform Material material;
uniform sampler2D depth_map;

uniform vec3 specular_value;
uniform vec3 viewPos;

uniform bool showNormals;
uniform bool hasSpecular;
uniform bool useBlinn;
uniform bool calculateShadows;
uniform bool useSmallerBias;

uniform float dirLightIntensity;
uniform float pointLightIntensity;
uniform float spotLightIntensity;

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float calcShadows(vec4 fragLightSpacePos, vec3 normal, vec3 lightDir);

out vec4 FragColor;

void main(){
	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 normal = normalize(Normal);
	vec3 color = vec3(0.0);
	if(showNormals)
		color = Normal;
	else{
		color += calcPointLight(point_light, normal, fragPos, viewDir) * pointLightIntensity;
		color += calcPointLight(point_light2, normal, fragPos, viewDir) * pointLightIntensity;
		color += calcDirLight(dir_light, normal, viewDir) * dirLightIntensity;
		color += calcSpotLight(spot_light, normal, fragPos, viewDir) * spotLightIntensity;
	}

	FragColor = vec4(color, 1.0);
}

vec3 calcDirLight(DirLight light, vec3 normal, vec3 viewDir){
	vec3 lightDir = normalize(-light.direction.xyz);
	float diff = max(dot(normal, lightDir), 0.0);
	float spec;

	if(useBlinn){
		vec3 halfway = normalize(viewDir + lightDir);	
		spec = pow(max(dot(normal, halfway), 0.0), material.shininess);
	}
	else{
		vec3 reflectDir = reflect(-lightDir, normal);
		spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	}

	vec3 ambient_color = texture(material.texture_diffuse1, TexCoords).rgb;
	vec3 diffuse_color = texture(material.texture_diffuse1, TexCoords).rgb;
	vec3 specular_color;

	if(hasSpecular)
		specular_color = texture(material.texture_specular1, TexCoords).rgb;
	else
		specular_color = specular_value;
			
	vec3 ambient = light.ambient.xyz * ambient_color;
	vec3 diffuse = light.diffuse.xyz * diff * diffuse_color;
	vec3 specular = light.specular.xyz * spec * specular_color;
			
	if(calculateShadows){
		float shadow = calcShadows(fragLightPos, normal, lightDir);
		return ambient + (1.0 - shadow)*(diffuse + specular);
	}
	else{
		return ambient + diffuse + specular;	
	}
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir){
	vec3 lightDir = normalize(light.position.xyz - fragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	float spec;

	if(useBlinn){
		vec3 halfway = normalize(viewDir + lightDir);	
		spec = pow(max(dot(normal, halfway), 0.0), material.shininess);
	}
	else{
		vec3 reflectDir = reflect(-lightDir, normal);
		spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	}
		
	float dist = length(light.position.xyz - fragPos);
	float attenuation = 1.0 / (light.ambient.w + light.diffuse.w * dist + light.specular.w * dist * dist);

	vec3 ambient_color = texture(material.texture_diffuse1, TexCoords).rgb;
	vec3 diffuse_color = texture(material.texture_diffuse1, TexCoords).rgb;
	vec3 specular_color;

	if(hasSpecular)
		specular_color = texture(material.texture_specular1, TexCoords).rgb;
	else
		specular_color = specular_value;
			
	vec3 ambient = light.ambient.xyz * ambient_color;
	vec3 diffuse = light.diffuse.xyz * diff * diffuse_color;
	vec3 specular = light.specular.xyz * spec * specular_color;

	ambient *= attenuation;	
	diffuse *= attenuation;
	specular *= attenuation;	
	
	if(calculateShadows){
		float shadow = calcShadows(fragLightPos, normal, lightDir);
		return ambient + (1.0 - shadow)*(diffuse + specular);
	}
	else{
		return ambient + diffuse + specular;	
	}
}

vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir){
	vec3 lightDir = normalize(light.position.xyz - fragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	float spec;

	if(useBlinn){
		vec3 halfway = normalize(viewDir + lightDir);	
		spec = pow(max(dot(normal, halfway), 0.0), material.shininess);
	}
	else{
		vec3 reflectDir = reflect(-lightDir, normal);
		spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	}	
	
	float dist = length(light.position.xyz - fragPos);
	float attenuation = 1.0 / (light.ambient.w + light.diffuse.w * dist + light.specular.w * dist * dist);

	float theta = dot(lightDir, normalize(-light.direction.xyz));
	float epsilon = (light.position.w - light.direction.w);
	float intensity = clamp((theta - light.direction.w)/epsilon , 0.0, 1.0);
	
	vec3 ambient_color = texture(material.texture_diffuse1, TexCoords).rgb;
	vec3 diffuse_color = texture(material.texture_diffuse1, TexCoords).rgb;
	vec3 specular_color;

	if(hasSpecular)
		specular_color = texture(material.texture_specular1, TexCoords).rgb;
	else
		specular_color = specular_value;
			
	vec3 ambient = light.ambient.xyz * ambient_color;
	vec3 diffuse = light.diffuse.xyz * diff * diffuse_color;
	vec3 specular = light.specular.xyz * spec * specular_color;
	
	ambient *= attenuation; 
	diffuse *= attenuation;
	specular *= attenuation;

	diffuse *= intensity;
	specular *= intensity;
	
	if(calculateShadows){
		float shadow = calcShadows(fragLightPos, normal, lightDir);
		return ambient + (1.0 - shadow)*(diffuse + specular);
	}
	else{
		return ambient + diffuse + specular;	
	}
}

float calcShadows(vec4 fragLightSpacePos, vec3 normal, vec3 lightDir){
	vec3 projCoords = fragLightSpacePos.xyz/fragLightSpacePos.w;
	projCoords = projCoords * 0.5 + 0.5;
	float currentDepth = projCoords.z;
	float bias;
	bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
	if(useSmallerBias){
		bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
	}

	float shadow = 0.0;
	vec2 texel_size = 1.0 / textureSize(depth_map, 0);
	
	for(int x = -1; x <= 1; x++){
		for(int y = -1; y <= 1; y++){
			vec2 offset = vec2(x,y);
			float pcf_depth = texture(depth_map, projCoords.xy + offset * texel_size).r;
			shadow += currentDepth - bias > pcf_depth ? 1.0 : 0.0;
		}	
	}
	shadow /= 9.0;
	if(projCoords.z > 1.0) shadow = 0.0;

    return shadow;
}
