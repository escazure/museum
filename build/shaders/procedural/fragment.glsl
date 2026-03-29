#version 330 core

in vec3 FragPos;
in vec3 Normal;

uniform float time;
uniform float speed;
uniform float scale;
uniform int fbm_octaves;
uniform bool showNormals;

out vec4 FragColor;

float hash(vec3 p);
float value_noise(vec3 p);
float fbm(vec3 p, int octaves);
vec3 cloudGradient(float t);

void main(){
    vec3 normal = normalize(Normal);

    vec3 p = FragPos * 2.0 + vec3(time * speed, time * speed * 0.3, time * speed * 0.7);

    float base = fbm(p * 2.0, fbm_octaves);
    float detail = fbm(p  * 3.0, fbm_octaves);
    float fine = fbm(p * 4.0, fbm_octaves);

	float noiseValue = base * scale + detail * scale * 0.3 + fine * scale * 0.1;
	noiseValue = clamp(noiseValue, 0.0, 1.0);
	noiseValue = smoothstep(0.6, 0.9, noiseValue);

	float terrain = fbm(FragPos * 1.5, 6);
	terrain = clamp(terrain, 0.0, 1.0);
	float landMask = smoothstep(0.5, 0.6, terrain);

	vec3 groundColor = vec3(0.1, 0.35, 0.1);
	vec3 groundLight = vec3(0.3, 0.6, 0.3);
	float groundNoise = fbm(FragPos * 4.0, 3);
	vec3 land = mix(groundColor, groundLight, groundNoise);

	vec3 oceanColor = vec3(0.0, 0.2, 0.6);
	vec3 oceanLight = vec3(0.0, 0.5, 0.8);
	float oceanNoise = fbm(FragPos * 3.0, 3);
	vec3 ocean = mix(oceanColor, oceanLight, oceanNoise);

	vec3 earth = mix(ocean, land, landMask);

	float cloudMask = smoothstep(0.5, 0.9, noiseValue);
	vec3 cloudColor = cloudGradient(noiseValue) * 0.7;

	vec3 color = earth;
	color = mix(color, cloudColor, cloudMask);

	if(showNormals) FragColor = vec4(normal, 1.0);
	else FragColor = vec4(color, 1.0);
}

float hash(vec3 p){
	return fract(sin(dot(vec3(123.4, 64.7, 1.5), p)) * 847.2);
}

float value_noise(vec3 p){
	vec3 i = floor(p);
	vec3 f = fract(p);

	float v0 = hash(i);
	float v1 = hash(i + vec3(1.0, 0.0, 0.0));
	float v2 = hash(i + vec3(0.0, 1.0, 0.0));
	float v3 = hash(i + vec3(1.0, 1.0, 0.0));
	float v4 = hash(i + vec3(0.0, 0.0, 1.0));
	float v5 = hash(i + vec3(1.0, 0.0, 1.0));
	float v6 = hash(i + vec3(0.0, 1.0, 1.0));
	float v7 = hash(i + vec3(1.0, 1.0, 1.0));

	vec3 u = f * f * (3.0 - 2.0 * f);
	return mix(
			mix(mix(v0,v1,u.x), mix(v2,v3,u.x), u.y),
			mix(mix(v4,v5,u.x), mix(v6,v7,u.x), u.y),
			u.z
			); 
}

float fbm(vec3 p, int octaves){
	float sum = 0.0;
	float max_sum = 0.0;
	float amplitude =  1.0;
	float frequency =  1.0;

	for(int i = 0; i < octaves; i++){
		sum += value_noise(p * frequency) * amplitude;
		max_sum += amplitude;
		frequency *= 2;
		amplitude *= 0.5;
	}
	return sum / max_sum;
}

vec3 cloudGradient(float t) {
    vec3 sky = vec3(0.5, 0.7, 1.0);     
    vec3 light = vec3(1.0, 1.0, 1.0);   
    vec3 shadow = vec3(0.7, 0.75, 0.8);

    if (t < 0.5)
        return mix(sky, shadow, t / 0.5);
    else
        return mix(shadow, light, (t - 0.5) / 0.5);
}
