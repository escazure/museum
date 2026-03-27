#include "core.h"
Context g_context;

int main(){
	init();
	run();
	shutdown();
}

void init(){
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
	glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "forest", NULL, NULL);
	glfwMakeContextCurrent(window);
	
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	if(gl3wInit()){
		std::cout << "Failed initing gl3w\n";
	}

	g_context.win_width = mode->width;
	g_context.win_height = mode->height;
	g_context.window = window;
	g_context.show_normals = false;
	print_info();
}

void run(){
	Camera camera(glm::vec3(0.0, 0.0, 6.0), 4.0, 0.05);
	g_context.camera = &camera;
	SceneResources sr = load_scene_resources();

	unsigned int quad_vao, quad_vbo;
	screen_quad_setup(quad_vao, quad_vbo);

	unsigned int quad_fbo, quad_rbo, quad_texture;
	setup_screen_fbo(quad_fbo, quad_texture, quad_rbo);

	unsigned int depth_map_fbo, depth_map;
	setup_shadow_map(depth_map_fbo, depth_map);

	float near_plane = -2.0f, far_plane = 15.0f;
	glm::mat4 lightProjection = glm::ortho(-12.0f, 12.0f, -12.0f, 12.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(glm::vec3(4.0f, 4.0f, 4.0f),
									  glm::vec3(6.0f, 0.0f, 6.0f),
									  glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lightSpaceMatrix = lightProjection * lightView;

	unsigned int rock_vbo;
	setup_rock_instancing(sr.rock, rock_vbo);	
	unsigned int particle_vao, particle_vbo;
	setup_particle_system(particle_vao, particle_vbo);

	std::vector<std::string> faces = {
		"textures/skybox/right.png",
		"textures/skybox/left.png",
		"textures/skybox/top.png",
		"textures/skybox/bot.png",
		"textures/skybox/front.png",
		"textures/skybox/back.png",
	};
	unsigned int skybox_texture = loadCubeMap(faces);

	glm::vec3 lightPos = glm::vec3(0.0f, 2.0f, 3.0f);
	glm::vec3 lightPos2 = glm::vec3(12.0f, 2.0f, 12.0f);
	glm::vec3 lightColorWarm = glm::vec3(0.8f, 0.8f, 0.1f);
	glm::vec3 lightColorNeutral = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 lightColorCold = glm::vec3(0.3f, 0.4f, 1.0f);
	glm::vec3 light_cube_size = glm::vec3(0.05f, 0.05f, 0.05f);

	PointLight point_light;
	PointLight point_light2;
	DirLight dir_light;
	SpotLight spot_light;
	setup_lights(point_light, point_light2, dir_light, spot_light);

	unsigned int matrix_ubo, light_ubo;
	setup_ubos(matrix_ubo, light_ubo, point_light, point_light2, dir_light, spot_light);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	float current_frame = 0.0;
	float last_frame = 0.0;
	while(!glfwWindowShouldClose(g_context.window)){
		last_frame = current_frame;
		current_frame = glfwGetTime();
		g_context.delta_time = current_frame - last_frame;
		process_input();

		static float rotation_offset = 0.0f;
		if(g_context.rotate_light){
			rotation_offset += 1.2f * g_context.delta_time;
			lightPos.x = glm::sin(rotation_offset) * 3;
			lightPos.z = glm::cos(rotation_offset) * 3;
		}
		else{
			lightPos.x = glm::sin(rotation_offset) * 3;
			lightPos.z = glm::cos(rotation_offset) * 3;
		}

		// Update ubos //
		update_ubos(matrix_ubo, light_ubo, lightPos);
		// ------------------- //

		// Render scene to depth texture //
		glViewport(0, 0, g_context.shadow_width, g_context.shadow_height);
		glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo);
		glClear(GL_DEPTH_BUFFER_BIT);
		render_depth_map(sr, lightSpaceMatrix);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0,0,(int)g_context.win_width, (int)g_context.win_height);

		// Render scene normally //
		glBindFramebuffer(GL_FRAMEBUFFER, quad_fbo);
		glClearColor(0.2, 0.2, 0.2, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depth_map);

		// Draw floor //
		sr.phong_shader.use();
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(6.0f, -2.5f, 6.0f));
		model = glm::scale(model, glm::vec3(10.0f, 1.0f, 10.0f));
		sr.phong_shader.set_mat4("model", model);	
		sr.phong_shader.set_bool("hasSpecular", false);
		sr.phong_shader.set_mat4("lightSpaceMatrix", lightSpaceMatrix);
		sr.phong_shader.set_int("depth_map", 1);
		sr.phong_shader.set_vec3("specular_value", glm::vec3(0.1));
		sr.phong_shader.set_float("material.shininess", 64.0);
		sr.phong_shader.set_float("dirLightIntensity", 1.0);
		sr.phong_shader.set_float("pointLightIntensity", 1.0);
		sr.phong_shader.set_float("spotLightIntensity", 1.0);
		if(g_context.use_blinn)
			sr.phong_shader.set_float("material.shininess", 64.0f);
		else
			sr.phong_shader.set_float("material.shininess", 32.0f);
		sr.scene_floor.Draw(sr.phong_shader);	

		// Draw monkey with point light //
		sr.phong_shader.use();
		if(g_context.use_blinn)
			sr.phong_shader.set_float("material.shininess", 128.0f);
		else
			sr.phong_shader.set_float("material.shininess", 32.0f);
		sr.phong_shader.set_vec3("viewPos", camera.position);
		sr.phong_shader.set_bool("hasSpecular", true);
		sr.phong_shader.set_bool("showNormals", g_context.show_normals);
		sr.phong_shader.set_bool("useBlinn", g_context.use_blinn);

		model = glm::mat4(1.0f);
		sr.phong_shader.set_mat4("model", model);
		sr.phong_shader.set_float("dirLightIntensity", 0.1);
		sr.phong_shader.set_float("pointLightIntensity", 1.0);
		sr.phong_shader.set_float("spotLightIntensity", 0.0);
		sr.monkey.Draw(sr.phong_shader);
		// --------------------- //

		// Draw monkey with directional light //
		model = glm::mat4(1.0f);	
		model = glm::translate(model, glm::vec3(6.0f, 0.0f, 0.0f));
		sr.phong_shader.set_mat4("model", model);
		sr.phong_shader.set_int("depth_map", 1);
		sr.phong_shader.set_float("dirLightIntensity", 1.0);
		sr.phong_shader.set_float("pointLightIntensity", 0.0);
		sr.phong_shader.set_float("spotLightIntensity", 0.0);
		sr.monkey.Draw(sr.phong_shader);
		// --------------------- //

		// Draw monkey with spot light //
		model = glm::mat4(1.0f);	
		model = glm::translate(model, glm::vec3(12.0f, 0.0f, 0.0f));
		sr.phong_shader.set_mat4("model", model);
		sr.phong_shader.set_float("dirLightIntensity", 0.1);
		sr.phong_shader.set_float("pointLightIntensity", 0.0);
		sr.phong_shader.set_float("spotLightIntensity", 1.0);
		sr.monkey.Draw(sr.phong_shader);
		// --------------------- //
		
		// Draw pedestals //
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depth_map);
		if(g_context.use_blinn)
			sr.phong_shader.set_float("material.shininess", 32.0f);
		else
			sr.phong_shader.set_float("material.shininess", 8.0f);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
		sr.phong_shader.set_mat4("model", model);
		sr.phong_shader.set_bool("hasSpecular", false);
		sr.phong_shader.set_vec3("specular_value", glm::vec3(0.02f, 0.02f, 0.02f));
		sr.phong_shader.set_float("dirLightIntensity", 1.0);
		sr.phong_shader.set_float("pointLightIntensity", 1.0);
		sr.phong_shader.set_float("spotLightIntensity", 1.0);
		sr.pedestal.Draw(sr.phong_shader);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(6.0f, -2.0f, 0.0f));
		sr.phong_shader.set_mat4("model", model);
		sr.pedestal.Draw(sr.phong_shader);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(12.0f, -2.0f, 0.0f));
		sr.phong_shader.set_mat4("model", model);
		sr.pedestal.Draw(sr.phong_shader);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -2.0f, 12.0f));
		sr.phong_shader.set_mat4("model", model);
		sr.pedestal.Draw(sr.phong_shader);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(6.0f, -2.0f, 12.0f));
		sr.phong_shader.set_mat4("model", model);
		sr.pedestal.Draw(sr.phong_shader);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(12.0f, -2.0f, 12.0f));
		sr.phong_shader.set_mat4("model", model);
		sr.pedestal.Draw(sr.phong_shader);
		// --------------------- //
		
		// Draw procedurally textured cube //
		sr.procedural_shader.use();
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(12.0f, 0.1f, 12.0f));
		model = glm::rotate(model, (float)glm::radians(glfwGetTime() * 5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5));
		sr.procedural_shader.set_mat4("model", model);
		sr.procedural_shader.set_float("time", glfwGetTime());
		sr.cube.Draw(sr.procedural_shader);
		// --------------------- //

		// Draw asteroid field and planet //
		glm::vec3 dirLightPos = glm::vec3(-2.0f, 4.0f, -1.0f);
		glm::vec3 lightDir = glm::normalize(glm::vec3(6.0f, 0.0f, 6.0f) - dirLightPos);
		sr.planet_shader.use();
		sr.planet_shader.set_vec3("viewPos", camera.position);
		sr.planet_shader.set_vec3("light.direction", glm::vec3(lightDir));
		sr.planet_shader.set_vec3("light.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
		sr.planet_shader.set_vec3("light.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
		sr.planet_shader.set_vec3("light.specular", glm::vec3(0.05f, 0.05f, 0.05f));
		sr.planet_shader.set_vec3("specular_value", glm::vec3(0.02f, 0.02f, 0.02f));
		sr.planet_shader.set_float("shininess", 32.0f);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 12.0f));
		model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
		sr.planet_shader.set_mat4("model", model);
		sr.planet.Draw(sr.planet_shader);

		sr.rock_shader.use();
		sr.rock_shader.set_vec3("viewPos", camera.position);
		sr.rock_shader.set_vec3("light.direction", glm::vec3(0.0f, -1.0f, 1.0f));
		sr.rock_shader.set_vec3("light.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
		sr.rock_shader.set_vec3("light.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
		sr.rock_shader.set_vec3("viewPos", camera.position);
		sr.rock_shader.set_float("time", glfwGetTime() * 0.03f);
		for(unsigned int i = 0; i < sr.rock.meshes.size(); i++){
			glBindVertexArray(sr.rock.meshes[i].VAO);
			glDrawElementsInstanced(GL_TRIANGLES, sr.rock.meshes[i].indices.size(), GL_UNSIGNED_INT, 0, g_context.rock_amount);
			glBindVertexArray(0);
		}
		// --------------------- //
		
		// Draw light cubes //
		sr.light_cube_shader.use();
		model = glm::mat4(1.0f);
		model = glm::translate(model, lightPos);
		model = glm::scale(model, light_cube_size);
		sr.light_cube_shader.set_mat4("model", model);
		sr.light_cube_shader.set_vec3("lightColor", lightColorWarm);
		sr.light_cube.Draw(sr.light_cube_shader);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(dirLightPos));
		model = glm::scale(model, light_cube_size);
		sr.light_cube_shader.set_mat4("model", model);
		sr.light_cube_shader.set_vec3("lightColor", lightColorNeutral);
		sr.light_cube.Draw(sr.light_cube_shader);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(12.0f, 2.0f, 3.0f));
		model = glm::scale(model, light_cube_size);
		sr.light_cube_shader.set_mat4("model", model);
		sr.light_cube_shader.set_vec3("lightColor", lightColorCold);
		sr.light_cube.Draw(sr.light_cube_shader);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(dirLightPos + glm::vec3(0.0f, 0.0f, 11.0f)));
		model = glm::scale(model, light_cube_size);
		sr.light_cube_shader.set_mat4("model", model);
		sr.light_cube_shader.set_vec3("lightColor", lightColorNeutral);
		sr.light_cube.Draw(sr.light_cube_shader);
		// --------------------- //	

		// Skybox //
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_CULL_FACE);
		sr.skybox_shader.use();
		sr.skybox_shader.set_int("skybox", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);
		sr.skybox.Draw(sr.skybox_shader);
		glDepthFunc(GL_LESS);
		glEnable(GL_CULL_FACE);
		// --------------------- //

		// Draw snowflakes particles //
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		render_particles(sr, particle_vao);
		glDisable(GL_BLEND);
		// --------------------- //

		// Post processing //
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		render_postprocess(sr, quad_vao, quad_texture);
		// --------------------- //

		glfwSwapBuffers(g_context.window);
		glfwPollEvents();
	}
}

void shutdown(){
	glfwDestroyWindow(g_context.window);
	glfwTerminate();
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos){
	static float lastx = g_context.win_width/2;
	static float lasty = g_context.win_height/2;
	static bool first_mouse = true;

	if(first_mouse){
		lastx = (float)xpos;
		lasty = (float)ypos;
		first_mouse = false;
	}

	float xoffset = xpos - lastx;	
	float yoffset = lasty - ypos;	
	lastx = xpos;
	lasty = ypos;

	g_context.camera->process_mouse_mov(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	g_context.camera->process_scroll(yoffset);
}

void process_input(){
	if(glfwGetKey(g_context.window, GLFW_KEY_W) == GLFW_PRESS)	
		g_context.camera->move_forward(g_context.delta_time);
	if(glfwGetKey(g_context.window, GLFW_KEY_S) == GLFW_PRESS)
		g_context.camera->move_back(g_context.delta_time);
	if(glfwGetKey(g_context.window, GLFW_KEY_A) == GLFW_PRESS)
		g_context.camera->move_left(g_context.delta_time);
	if(glfwGetKey(g_context.window, GLFW_KEY_D) == GLFW_PRESS)
		g_context.camera->move_right(g_context.delta_time);
	if(glfwGetKey(g_context.window, GLFW_KEY_SPACE) == GLFW_PRESS)
		g_context.camera->move_up(g_context.delta_time);
	if(glfwGetKey(g_context.window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		g_context.camera->move_down(g_context.delta_time);

	static bool zeroKeyWasPressed = false;
	bool zeroKeyIsPressed = glfwGetKey(g_context.window, GLFW_KEY_0) == GLFW_PRESS;
	if (zeroKeyIsPressed && !zeroKeyWasPressed) {
   		g_context.show_normals = !g_context.show_normals;
	}
	zeroKeyWasPressed = zeroKeyIsPressed;	

	static bool oneKeyWasPressed = false;
	bool oneKeyIsPressed = glfwGetKey(g_context.window, GLFW_KEY_1) == GLFW_PRESS;
	if(oneKeyIsPressed && !oneKeyWasPressed){
   		g_context.rotate_light = !g_context.rotate_light;
	}
	oneKeyWasPressed = oneKeyIsPressed;
	
	static bool threeKeyWasPressed = false;
	bool threeKeyIsPressed = glfwGetKey(g_context.window, GLFW_KEY_3) == GLFW_PRESS;
	if(threeKeyIsPressed && !threeKeyWasPressed){
		g_context.use_blinn = !g_context.use_blinn;
	}
	threeKeyWasPressed = threeKeyIsPressed;

	static bool fourKeyWasPressed = false;
	bool fourKeyIsPressed = glfwGetKey(g_context.window, GLFW_KEY_4) == GLFW_PRESS;
	if(fourKeyIsPressed && !fourKeyWasPressed){
		g_context.post_processing_mode = 0;
	}
	fourKeyWasPressed = fourKeyIsPressed;

	static bool fiveKeyWasPressed = false;
	bool fiveKeyIsPressed = glfwGetKey(g_context.window, GLFW_KEY_5) == GLFW_PRESS;
	if(fiveKeyIsPressed && !fiveKeyWasPressed){
		g_context.post_processing_mode = 1;
	}
	fiveKeyWasPressed = fiveKeyIsPressed;

	static bool sixKeyWasPressed = false;
	bool sixKeyIsPressed = glfwGetKey(g_context.window, GLFW_KEY_6) == GLFW_PRESS;
	if(sixKeyIsPressed && !sixKeyWasPressed){
		g_context.post_processing_mode = 2;
	}
	sixKeyWasPressed = sixKeyIsPressed;

	static bool sevenKeyWasPressed = false;
	bool sevenKeyIsPressed = glfwGetKey(g_context.window, GLFW_KEY_7) == GLFW_PRESS;
	if(sevenKeyIsPressed && !sevenKeyWasPressed){
		g_context.post_processing_mode = 3;
	}
	sevenKeyWasPressed = sevenKeyIsPressed;

	static bool gKeyWasPressed = false;
	bool gKeyIsPressed = glfwGetKey(g_context.window, GLFW_KEY_G) == GLFW_PRESS;
	if(gKeyIsPressed && !gKeyWasPressed){
		g_context.use_gamma_correction = !g_context.use_gamma_correction;
	}
	gKeyWasPressed = gKeyIsPressed;
}

unsigned int loadCubeMap(std::vector<std::string> faces){
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	unsigned char* data;
	for(unsigned int i = 0; i < faces.size(); i++){
		data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if(data)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		else
			std::cerr << "ERROR:FAILED_TO_LOAD_TEXTURE" << std::endl;
		stbi_image_free(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void print_info(){
	std::cout << "Info: \n = Camera movements:\n -- Up - Space\n -- Down - Left Control\n -- Left - A\n -- Right - D\n -- Forward - W\n -- Backward - S\n";
	std::cout << " = Zoom - Scroll\n";
	std::cout << " = Show normals - 0\n";
	std::cout << " = Enable light cube rotation for point light - 1\n";
	std::cout << " = Toggle Blinn-Phong shading model - 3\n";
	std::cout << " = Toggle Gamma correction- G\n";
	std::cout << " = Filters:\n -- 4 - Normal\n -- 5 - Inverse\n -- 6 - Grayscale\n -- 7 - Gaussian blur\n";
}


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

SceneResources load_scene_resources(){
	stbi_set_flip_vertically_on_load(true);
	SceneResources sr;
	stbi_set_flip_vertically_on_load(false);
	return sr;
}

void setup_rock_instancing(Model& rock, unsigned int& vbo){
	glm::mat4* modelMatrices = new glm::mat4[g_context.rock_amount];
	srand((int)glfwGetTime());
	float radius = 2.0f;
	float offset = 1.0f;
	for(unsigned int i = 0; i < g_context.rock_amount; i++){
		glm::mat4 model = glm::mat4(1.0f);
		float angle = (float)i/(float)g_context.rock_amount*360.0f;
		float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float x = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float y = displacement * 0.05f;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float z = cos(angle) * radius + displacement;

		model = glm::translate(model, glm::vec3(x, y, 12.0f + z));

		float scale = (rand() % 5) / 10000.0f + 0.005;
		model = glm::scale(model, glm::vec3(scale));
		
		float rotAngle = (rand() % 360);
		model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

		modelMatrices[i] = model;
	}
	std::size_t vec4Size = sizeof(glm::vec4);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * g_context.rock_amount, &modelMatrices[0], GL_STATIC_DRAW);
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
	glm::mat4* particleModelMatrices = new glm::mat4[g_context.particle_count];
	for(unsigned int i = 0; i < g_context.particle_count; i++){
		glm::mat4 model = glm::mat4(1.0f);

		float dy = (rand() % 25) / 100.0f;
		float dis = (rand() % 5) / 100.0f;

		float angle = (float)i/(float)g_context.particle_count*360.0f;
		float t = (rand() % 100) / 100.0f;

		float radius = g_context.top_radius * t + dis;

		float x = sin(angle) * radius;
		float y = t * g_context.cone_height + dy;
		float z = cos(angle) * radius;

		model = glm::translate(model, glm::vec3(6.0f + x, y - 0.7f, 12.0f + z));

		float scale = (rand() % 20) / 10000.0f + 0.005;
		model = glm::scale(model, glm::vec3(scale));
		
		particleModelMatrices[i] = model;
	}


	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * g_context.particle_count, &particleModelMatrices[0], GL_STATIC_DRAW);
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

void setup_lights(PointLight& p1, PointLight& p2, DirLight& dir, SpotLight& spot){
	glm::vec3 lightPos = glm::vec3(0.0f, 2.0f, 3.0f);
	glm::vec3 lightPos2 = glm::vec3(12.0f, 2.0f, 12.0f);
	glm::vec3 lightColorBlue = glm::vec3(0.4f, 1.0f, 1.0f);
	glm::vec3 lightColorWarm = glm::vec3(0.8f, 0.8f, 0.1f);
	glm::vec3 lightColorNeutral = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 lightColorCold = glm::vec3(0.3f, 0.4f, 1.0f);

	p1.position = glm::vec4(lightPos, 0.0f);
	p1.ambient = glm::vec4(lightColorWarm * 0.1f, 1.0f);
	p1.diffuse = glm::vec4(lightColorWarm * 0.7f, 0.05f);
	p1.specular = glm::vec4(lightColorWarm * 0.7f, 0.012f);

	p2.position = glm::vec4(lightPos2, 0.0f);
	p2.ambient = glm::vec4(lightColorBlue * 0.1f, 1.0f);
	p2.diffuse = glm::vec4(lightColorBlue * 0.5f, 0.09f);
	p2.specular = glm::vec4(lightColorBlue * 0.7f, 0.032f);

	lightPos = glm::vec3(4.0f, 4.0f, 4.0f);
	glm::vec3 lightDir = glm::normalize(glm::vec3(6.0f, 0.0f, 6.0f) - lightPos);

	dir.direction = glm::vec4(lightDir, 0.0f);
	dir.ambient = glm::vec4(lightColorNeutral * 0.1f, 0.0f);
	dir.diffuse = glm::vec4(lightColorNeutral * 0.4f, 0.0f);
	dir.specular = glm::vec4(lightColorNeutral * 0.4f, 0.0f);

	spot.position = glm::vec4(12.0f, 2.0f, 3.0f, glm::cos(glm::radians(7.0f)));
	spot.direction = glm::vec4(0.0f, -1.0f, -1.3f, glm::cos(glm::radians(10.0f)));
	spot.ambient = glm::vec4(lightColorCold * 0.1f, 1.0f);
	spot.diffuse = glm::vec4(lightColorCold * 0.5f, 0.08f);
	spot.specular = glm::vec4(lightColorCold * 0.7f, 0.03f);
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

void update_ubos(unsigned int& matrix_ubo, unsigned int& light_ubo, glm::vec3& lightPos){
	glm::mat4 projection = glm::perspective(glm::radians(g_context.camera->fov), g_context.win_width/g_context.win_height, 0.01f, 200.0f);
	glm::mat4 view = g_context.camera->get_view_mat();
	glBindBuffer(GL_UNIFORM_BUFFER, matrix_ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBuffer(GL_UNIFORM_BUFFER, light_ubo);	
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::vec3), glm::value_ptr(lightPos));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void render_depth_map(SceneResources& sr, glm::mat4& lightSpaceMatrix){
	sr.depth_shader.use();
	sr.depth_shader.set_mat4("lightSpaceMatrix", lightSpaceMatrix);
		
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(6.0f, -2.5f, 6.0f));
	model = glm::scale(model, glm::vec3(10.0f, 1.0f, 10.0f));
	sr.depth_shader.set_mat4("model", model);	
	sr.scene_floor.Draw(sr.depth_shader);

	model = glm::mat4(1.0f);
	sr.depth_shader.set_mat4("model", model);
	sr.monkey.Draw(sr.depth_shader);

	model = glm::mat4(1.0f);	
	model = glm::translate(model, glm::vec3(6.0f, 0.0f, 0.0f));
	sr.depth_shader.set_mat4("model", model);
	sr.monkey.Draw(sr.depth_shader);

	model = glm::mat4(1.0f);	
	model = glm::translate(model, glm::vec3(12.0f, 0.0f, 0.0f));
	sr.depth_shader.set_mat4("model", model);
	sr.monkey.Draw(sr.depth_shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
	sr.depth_shader.set_mat4("model", model);
	sr.pedestal.Draw(sr.depth_shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(6.0f, -2.0f, 0.0f));
	sr.depth_shader.set_mat4("model", model);
	sr.pedestal.Draw(sr.depth_shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(12.0f, -2.0f, 0.0f));
	sr.depth_shader.set_mat4("model", model);
	sr.pedestal.Draw(sr.depth_shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -2.0f, 12.0f));
	sr.depth_shader.set_mat4("model", model);
	sr.pedestal.Draw(sr.depth_shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(6.0f, -2.0f, 12.0f));
	sr.depth_shader.set_mat4("model", model);
	sr.pedestal.Draw(sr.depth_shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(12.0f, -2.0f, 12.0f));
	sr.depth_shader.set_mat4("model", model);
	sr.pedestal.Draw(sr.depth_shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(12.0f, 0.1f, 12.0f));
	model = glm::rotate(model, (float)glm::radians(glfwGetTime() * 5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.5));
	sr.depth_shader.set_mat4("model", model);
	sr.cube.Draw(sr.depth_shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 12.0f));
	model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
	sr.depth_shader.set_mat4("model", model);
	sr.planet.Draw(sr.depth_shader);
}

void render_postprocess(SceneResources& sr,unsigned int& vao, unsigned int& texture){
	sr.screen_shader.use();
	sr.screen_shader.set_int("screen", 0);
	sr.screen_shader.set_int("mode", g_context.post_processing_mode);
	sr.screen_shader.set_float("gamma", 2.2f);
	sr.screen_shader.set_bool("correct_gamma", g_context.use_gamma_correction);
	
	glBindVertexArray(vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void render_particles(SceneResources& sr, unsigned int& vao){
	sr.particle_shader.use();
	sr.particle_shader.set_float("time", glfwGetTime());
	sr.particle_shader.set_float("cone_height", g_context.cone_height); 
	sr.particle_shader.set_float("top_radius", g_context.top_radius);
	glBindVertexArray(vao);
	glDrawArraysInstanced(GL_POINTS, 0, 1, g_context.particle_count);
	glBindVertexArray(0);
}


