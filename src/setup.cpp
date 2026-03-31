#include "setup.h"

void screen_quad_setup(unsigned int &vao, unsigned int &vbo){
	float quad_vertices[] = {
		-1.0, -1.0, 0.0, 0.0, 0.0,
		1.0, -1.0, 0.0, 1.0, 0.0,
		-1.0, 1.0, 0.0, 0.0, 1.0,

		-1.0, 1.0, 0.0, 0.0, 1.0,
		1.0, -1.0, 0.0, 1.0, 0.0,
		1.0, 1.0, 0.0, 1.0, 1.0
	};

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);
}

void setup_screen_fbo(unsigned int& fbo, unsigned int& texture, unsigned int& rbo){
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, g_context.win_width, g_context.win_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, g_context.win_width, g_context.win_height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR::FAILED_TO_COMPLETE_RENDERBUFFER" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setup_shadow_map(unsigned int& depth_fbo, unsigned int& depth_map){
	glGenFramebuffers(1, &depth_fbo);

	glGenTextures(1, &depth_map);
	glBindTexture(GL_TEXTURE_2D, depth_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, g_context.shadow_width, g_context.shadow_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = {1.0, 1.0, 1.0, 1.0};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, depth_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setup_rock_instancing(Model& rock, unsigned int& vbo){
	glm::mat4* modelMatrices = new glm::mat4[g_context.max_rock_count];
	srand((int)glfwGetTime());
	float radius = 2.0f;
	float offset = 1.0f;
	for(unsigned int i = 0; i < g_context.max_rock_count; i++){
		glm::mat4 model = glm::mat4(1.0f);
		float angle = (float)i/(float)g_context.rock_count*360.0f;
		float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float x = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float y = displacement * 0.05f;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float z = cos(angle) * radius + displacement;

		model = glm::translate(model, glm::vec3(x, y, z));

		float scale = (rand() % 5) / 10000.0f + 0.005;
		model = glm::scale(model, glm::vec3(scale));
		
		float rotAngle = (rand() % 360);
		model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

		modelMatrices[i] = model;
	}
	std::size_t vec4Size = sizeof(glm::vec4);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * g_context.max_rock_count , &modelMatrices[0], GL_STATIC_DRAW);
	for(unsigned int i = 0; i < rock.meshes.size(); i++){
		unsigned int vao = rock.meshes[i].VAO;	
		glBindVertexArray(vao);

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
		glVertexAttribDivisor(3,1);
		glVertexAttribDivisor(4,1);
		glVertexAttribDivisor(5,1);
		glVertexAttribDivisor(6,1);

		glBindVertexArray(0);
	}
}

void setup_particle_system(unsigned int& vao, unsigned int& vbo){
	std::size_t vec4Size = sizeof(glm::vec4);
	for(unsigned int i = 0; i < g_context.particle_count; i++){
		respawn(g_context.particles_list[i]);
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * g_context.max_particle_count, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
	glVertexAttribDivisor(0,1);
	glVertexAttribDivisor(1,1);
	glVertexAttribDivisor(2,1);
	glVertexAttribDivisor(3,1);
	glBindVertexArray(0);
}

void setup_lights(PointLight& p1, PointLight& p2, DirLight& dir, SpotLight& spot, LightData& point_light_data, LightData& spot_light_data, LightData& dir_light_data){
	glm::vec3 lightPos2 = glm::vec3(0.0f, 2.0f, 0.0f);
	glm::vec3 lightColorBlue = glm::vec3(0.4f, 1.0f, 1.0f);

	p1.position = glm::vec4(point_light_data.position, 0.0f);
	p1.ambient = glm::vec4(point_light_data.color * 0.1f, 1.0f);
	p1.diffuse = glm::vec4(point_light_data.color * 0.7f, 0.05f);
	p1.specular = glm::vec4(point_light_data.color * 0.7f, 0.012f);

	p2.position = glm::vec4(lightPos2, 0.0f);
	p2.ambient = glm::vec4(lightColorBlue * 0.1f, 1.0f);
	p2.diffuse = glm::vec4(lightColorBlue * 0.6f, 0.09f);
	p2.specular = glm::vec4(lightColorBlue * 0.7f, 0.032f);

	glm::vec3 lightDir = glm::normalize(glm::vec3(0.0f, -2.0f, 0.0f) - dir_light_data.position);
	dir.direction = glm::vec4(lightDir, 0.0f);
	dir.ambient = glm::vec4(dir_light_data.color * 0.1f, 0.0f);
	dir.diffuse = glm::vec4(dir_light_data.color * 0.6f, 0.0f);
	dir.specular = glm::vec4(dir_light_data.color * 0.4f, 0.0f);

	lightDir = glm::normalize(glm::vec3(0.0f, -2.5f, 0.0f) - spot_light_data.position);
	spot.position = glm::vec4(spot_light_data.position, glm::cos(glm::radians(15.0f)));
	spot.direction = glm::vec4(lightDir, glm::cos(glm::radians(20.0f)));
	spot.ambient = glm::vec4(spot_light_data.color * 0.05f, 1.0f);
	spot.diffuse = glm::vec4(spot_light_data.color * 0.4f, 0.08f);
	spot.specular = glm::vec4(spot_light_data.color * 0.7f, 0.03f);
}

void setup_ubos(unsigned int& matrix_ubo, unsigned int& light_ubo, PointLight& p1, PointLight& p2, DirLight& dir, SpotLight& spot){
	glGenBuffers(1, &matrix_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, matrix_ubo);
	glBufferData(GL_UNIFORM_BUFFER, 2*sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, matrix_ubo);

	glGenBuffers(1, &light_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, light_ubo);
	glBufferData(GL_UNIFORM_BUFFER, 272, NULL, GL_STATIC_DRAW); // PointLight = 64*2, DirLight = 64, SpotLight = 80
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, light_ubo);

	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PointLight), &p1);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(PointLight), sizeof(PointLight), &p2);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(PointLight)*2, sizeof(DirLight), &dir);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(PointLight)*2 + sizeof(DirLight), sizeof(SpotLight), &spot);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
