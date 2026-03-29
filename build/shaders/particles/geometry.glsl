#version 330 core
layout (points) in;
layout (line_strip, max_vertices = 12) out;

in vec3 FragPos[];
out vec3 fragPos;

void drawSnowflake(vec4 position){
	float arm_length = 0.01;    

    for (int i = 0; i < 6; i++) {
   		float angle = radians(60.0 * i);
    	vec2 dir = vec2(cos(angle), sin(angle)) * arm_length;

    	gl_Position = position;
		fragPos = FragPos[0];
   		EmitVertex();

    	gl_Position = position + vec4(dir, 0.0, 0.0);
		fragPos = FragPos[0];
	    EmitVertex();

    	EndPrimitive();
	}
}

void main(){
	drawSnowflake(gl_in[0].gl_Position);
}
