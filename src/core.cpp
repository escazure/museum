#include "core.h"

void init(){
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
	glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "glshowcase", NULL, NULL);
	glfwMakeContextCurrent(window);
	
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

	if(gl3wInit()){
		std::cout << "Failed initing gl3w\n";
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); 
	(void)io;
	io.IniFilename = nullptr;
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	g_context.win_width = mode->width;
	g_context.win_height = mode->height;
	g_context.window = window;
	g_context.show_normals = false;
}

void run(){
	Camera camera(glm::vec3(-6.0, 0.5, 0.0), 3.0, 0.05);
	g_context.camera = &camera;
	SceneResources sr = load_scene_resources();

	unsigned int quad_vao, quad_vbo;
	screen_quad_setup(quad_vao, quad_vbo);

	unsigned int quad_fbo, quad_rbo, quad_texture;
	setup_screen_fbo(quad_fbo, quad_texture, quad_rbo);

	unsigned int depth_map_fbo, depth_map;
	setup_shadow_map(depth_map_fbo, depth_map);

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

	glm::vec3 lightPos;

	LightData point_light_data;
	point_light_data.position = glm::vec3(0.0f, 4.0f, 4.0f);
	point_light_data.color = glm::vec3(0.8f, 0.8f, 0.1f);
	LightData spot_light_data;
	spot_light_data.position = glm::vec3(-5.0f, 4.0f, 0.0f);
	spot_light_data.color = glm::vec3(0.3f, 0.4f, 1.0f);
	LightData dir_light_data;
	dir_light_data.position = glm::vec3(2.0f, 4.0f, 2.0f);
	dir_light_data.color = glm::vec3(1.0f, 1.0f, 1.0f);

	PointLight point_light;
	PointLight point_light2;
	DirLight dir_light;
	SpotLight spot_light;
	setup_lights(point_light, point_light2, dir_light, spot_light, point_light_data, spot_light_data, dir_light_data);

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
		update_particles();
		if(g_context.resize_requested){
			resize_framebuffer(quad_fbo, quad_texture);
			g_context.resize_requested = false;
		}

		static float rotation_offset = 0.0f;
		if(g_context.rotate_light){
			rotation_offset += g_context.point_light_rotation_speed * g_context.delta_time;
			lightPos.x = glm::sin(rotation_offset) * g_context.point_light_rotation_radius;
			lightPos.z = glm::cos(rotation_offset) * g_context.point_light_rotation_radius;
		}
		else{
			lightPos.x = glm::sin(rotation_offset) * g_context.point_light_rotation_radius;
			lightPos.z = glm::cos(rotation_offset) * g_context.point_light_rotation_radius;
		}
		lightPos.y = point_light_data.position.y;
		point_light_data.position = lightPos;

		// Update ubos //
		update_ubos(matrix_ubo, light_ubo, lightPos);
		// ------------------- //

		// Render scene to depth texture //
		float near_plane = 0.1f, far_plane = 20.0f;
		glm::mat4 lightProjection, lightView, lightSpaceMatrix;
		switch(g_context.demo_mode){
			case 0:
				lightProjection = glm::perspective(glm::radians(60.0f), (float)g_context.shadow_width/(float)g_context.shadow_height, near_plane, far_plane);
				lightView = glm::lookAt(glm::vec3(point_light_data.position),
									glm::vec3(0.0f, -2.5f, 0.0f), 
									glm::vec3(0.0f, 1.0f, 0.0f));
				break;
			case 1:
				lightProjection = glm::perspective(glm::radians(30.0f), (float)g_context.shadow_width/(float)g_context.shadow_height, near_plane, far_plane);
				lightView = glm::lookAt(glm::vec3(spot_light_data.position),
									glm::vec3(0.0f, -2.0f, 0.0f), 
									glm::vec3(0.0f, 1.0f, 0.0f));
				break;
			default:
				lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
				lightView = glm::lookAt(glm::vec3(dir_light_data.position),
									glm::vec3(0.0f, -2.5f, 0.0f), 
									glm::vec3(0.0f, 1.0f, 0.0f));
				break;

		}	
		lightSpaceMatrix = lightProjection * lightView;

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
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);
		glActiveTexture(GL_TEXTURE0);

		render_floor(sr, depth_map, lightSpaceMatrix);
		render_pedestal(sr, depth_map, lightSpaceMatrix);
	
		switch(g_context.demo_mode){
			case 0:
				render_point_light(sr, point_light_data);	
				break;
			case 1:
				render_spot_light(sr, spot_light_data);
				break;
			case 2:
				render_dir_light(sr, dir_light_data);	
				break;
			case 3:
				render_instancing(sr, dir_light_data);
				break;
			case 4:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				render_particles(sr, particle_vao, particle_vbo, dir_light_data);
				glDisable(GL_BLEND);
				break;
			case 5:
				render_texturing(sr, dir_light_data);
				break;
		}
		
		// Skybox //
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_CULL_FACE);
		render_skybox(sr, skybox_texture);
		glDepthFunc(GL_LESS);
		glEnable(GL_CULL_FACE);
		// --------------------- //

		// Post processing //
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		render_postprocess(sr, quad_vao, g_context.show_depth_map ? depth_map : quad_texture);
		// --------------------- //
		
		// Render gui //
		render_gui();
		// --------------------- //
		glfwSwapBuffers(g_context.window);
		glfwPollEvents();
	}
}

void shutdown(){
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
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

	if(g_context.capture_mouse)
		g_context.camera->process_mouse_mov(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	if(g_context.capture_mouse)
		g_context.camera->process_scroll(yoffset);
}

void framebuffer_resize_callback(GLFWwindow* window, int width, int height){
	g_context.win_width = width;
	g_context.win_height = height;
	g_context.resize_requested = true;	
}

void process_input(){
	if(g_context.capture_mouse){
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
	}

	static bool escKeyWasPressed = false;
	bool escKeyIsPressed = glfwGetKey(g_context.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
	if (escKeyIsPressed && !escKeyWasPressed) {
   		g_context.capture_mouse = !g_context.capture_mouse;
		glfwSetInputMode(g_context.window, GLFW_CURSOR, g_context.capture_mouse ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}
	escKeyWasPressed = escKeyIsPressed;	
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

SceneResources load_scene_resources(){
	SceneResources sr;
	return sr;
}

void resize_framebuffer(unsigned int& fbo, unsigned int& texture){
	glViewport(0, 0, (int)g_context.win_width, (int)g_context.win_height);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);	

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, g_context.win_width, g_context.win_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
}

void update_particles(){
	for(std::size_t i = 0; i < g_context.particles_list.size(); i++){
		g_context.particles_list[i].life -= g_context.delta_time;
		if(g_context.particles_list[i].life <= 0.0f){
			respawn(g_context.particles_list[i]);
		}
	}	
}

void respawn(Particle& p){
    float dy = (rand() % 25) / 100.0f;
    float dis = (rand() % 5) / 100.0f;
    float t = (rand() % 100) / 100.0f;

    float angle = ((rand() % 1000) / 1000.0f) * 360.0f;

    float radius = g_context.top_radius * t + dis;

    float x = sin(angle) * radius;
    float y = t * g_context.cone_height + dy;
    float z = cos(angle) * radius;

    p.position = glm::vec3(x, y - 0.7f, z);

    p.life = (rand() % 100)/ 100.0f * g_context.particle_lifetime;
    p.max_life = g_context.max_particle_lifetime;

    p.scale = (rand() % 20) / 10000.0f + 0.005f;
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

