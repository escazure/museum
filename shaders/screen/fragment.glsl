#version 330 core

in vec2 TexCoords;

uniform sampler2D screen;
uniform int mode;
uniform bool correct_gamma;
uniform float gamma;

const float offset = 1.0/300.0;
const float near_plane = 0.1;
const float far_plane = 10.0;

out vec4 FragColor;

float LinearizeDepth(float depth){
    float z = depth * 2.0 - 1.0; 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}

void main(){
	vec2 offsets[9] = vec2[](
		vec2(-offset, offset),
		vec2(0.0, offset),
		vec2(offset, offset),

		vec2(-offset, 0.0),
		vec2(0.0, 0.0),
		vec2(offset, 0.0),
		
		vec2(-offset, -offset),
		vec2(0.0, -offset),
		vec2(offset, -offset)
	);

	float kernel[9]; 
	vec3 color = vec3(0.0, 0.0, 0.0);
	if(mode == 4){
		color = vec3(LinearizeDepth(texture(screen, TexCoords).r)/far_plane);
	}
	else if(mode == 3){
		kernel = float[](
			1.0, 2.0, 1.0,	
			2.0, 4.0, 2.0,	
			1.0, 2.0, 1.0	
		);

		for(int i = 0; i < 9; i++)
			color += kernel[i] * texture(screen, TexCoords + offsets[i]).rgb;

		color *=  0.0625; // 1/16
	}
	else if(mode == 2){
		vec3 tex = texture(screen, TexCoords).rgb;
		float avg = (tex.r * 0.2126 + tex.g * 0.7152 + tex.b * 0.722) * 0.333; // 1/3
		color = vec3(avg);
	}
	else if(mode == 1){
		vec3 tex = texture(screen, TexCoords).rgb;
		color = 1 - tex;
	}
	else if(mode == 0){
		color = texture(screen, TexCoords).rgb;
	}

	if(correct_gamma)
		color = pow(color, vec3(1.0/gamma));

	FragColor = vec4(color, 1.0);
}
