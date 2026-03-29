#version 330 core
layout (location = 0) in mat4 instanceMatrix;

layout (std140) uniform Matrices {
	mat4 projection;
	mat4 view;
};

uniform float time;
uniform float cone_height;
uniform float top_radius;

out vec3 FragPos;

void main(){
	float size = length(instanceMatrix[0].xyz);
	gl_PointSize = size;

	vec4 worldPos = instanceMatrix * vec4(1.0);
	vec3 pivot = vec3(0.0, -0.7, 0.0);
	float speed = 1.0 + worldPos.y;
	float angle = time * speed;

	float h_ratio = (worldPos.y - pivot.y) / cone_height;
	float dy = sin(time + float(gl_InstanceID)) * 0.3 * h_ratio;
	float half_angle = atan(top_radius/cone_height);
	float radius_scale = dy / tan(half_angle);

	vec3 dir = normalize(worldPos.xyz - pivot);
	dir.y = 0.0;

	vec3 pos = worldPos.xyz - pivot;
	pos.x += dir.x * radius_scale;
	pos.z += dir.z * radius_scale;
	pos.y += dy;

	float s = sin(angle);
	float c = cos(angle);
	float x = pos.x * c - pos.z * s;
	float z = pos.x * s + pos.z * c;
	pos.x = x;
	pos.z = z;
	worldPos.xyz = pos + pivot;

	FragPos = worldPos.xyz;
	gl_Position = projection * view * worldPos;
}
