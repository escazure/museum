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
	model = glm::translate(model, glm::vec3(0.0f, -2.5f, 0.0f));
	model = glm::scale(model, glm::vec3(3.0f, 1.0f, 3.0f));
	sr.depth_shader.set_mat4("model", model);	
	sr.scene_floor.Draw(sr.depth_shader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
	sr.depth_shader.set_mat4("model", model);
	sr.pedestal.Draw(sr.depth_shader);

	if(g_context.demo_mode < 3){
		model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.8f));
		sr.depth_shader.set_mat4("model", model);
		sr.monkey.Draw(sr.depth_shader);
	}

	if(g_context.demo_mode == 5){
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.1f, 0.0f));
		model = glm::rotate(model, (float)glm::radians(glfwGetTime() * 5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5));
		sr.depth_shader.set_mat4("model", model);
		sr.cube.Draw(sr.depth_shader);
	}

	if(g_context.demo_mode == 3){
		model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
		sr.depth_shader.set_mat4("model", model);
		sr.planet.Draw(sr.depth_shader);
	}
}

void render_postprocess(SceneResources& sr,unsigned int& vao, unsigned int& texture){
	sr.screen_shader.use();
	sr.screen_shader.set_int("screen", 0);
	sr.screen_shader.set_int("mode", g_context.post_processing_mode);
	if(g_context.show_depth_map)
		sr.screen_shader.set_int("mode", 4);
	sr.screen_shader.set_float("gamma", g_context.gamma); 
	sr.screen_shader.set_bool("correct_gamma", g_context.use_gamma_correction);
	
	glBindVertexArray(vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void render_particles(SceneResources& sr, unsigned int& vao, unsigned int& vbo, LightData& light_data){
	static std::vector<glm::mat4> matrices(g_context.particle_count);

    glm::mat4 model = glm::mat4(1.0f);
	for (int i = 0; i < g_context.particle_count; i++){
		model = glm::mat4(1.0f);
    	model = glm::translate(model, g_context.particles_list[i].position);
    	model = glm::scale(model, glm::vec3(g_context.particles_list[i].scale));

    	matrices[i] = model;
	}

	sr.particle_shader.use();
	sr.particle_shader.set_float("time", glfwGetTime());
	sr.particle_shader.set_float("cone_height", g_context.cone_height); 
	sr.particle_shader.set_float("top_radius", g_context.top_radius);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::mat4) * g_context.particle_count, matrices.data());

	glBindVertexArray(vao);
	glDrawArraysInstanced(GL_POINTS, 0, 1, g_context.particle_count);
	glBindVertexArray(0);
	
	sr.light_cube_shader.use();
	model = glm::mat4(1.0f);
	model = glm::translate(model, light_data.position);
	model = glm::scale(model, glm::vec3(g_context.light_cube_size));
	sr.light_cube_shader.set_mat4("model", model);
	sr.light_cube_shader.set_vec3("lightColor", light_data.color);
	sr.light_cube.Draw(sr.light_cube_shader);
}

void render_point_light(SceneResources& sr, LightData& light_data){
	sr.phong_shader.use();
	if(g_context.use_blinn)
		sr.phong_shader.set_float("material.shininess", 128.0f);
	else
		sr.phong_shader.set_float("material.shininess", 32.0f);

	sr.phong_shader.set_vec3("viewPos", g_context.camera->position);
	sr.phong_shader.set_bool("hasSpecular", true);
	sr.phong_shader.set_bool("showNormals", g_context.show_normals);
	sr.phong_shader.set_bool("useBlinn", g_context.use_blinn);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	sr.phong_shader.set_mat4("model", model);
	sr.phong_shader.set_int("depth_map", 1);
	sr.phong_shader.set_bool("calculateShadows", false);
	sr.phong_shader.set_float("dirLightIntensity", 0.1);
	sr.phong_shader.set_float("pointLightIntensity", 1.0);
	sr.phong_shader.set_float("spotLightIntensity", 0.0);
	sr.monkey.Draw(sr.phong_shader);
	
	sr.light_cube_shader.use();
	model = glm::mat4(1.0f);
	model = glm::translate(model, light_data.position);
	model = glm::scale(model, glm::vec3(g_context.light_cube_size));
	sr.light_cube_shader.set_mat4("model", model);
	sr.light_cube_shader.set_vec3("lightColor", light_data.color);
	sr.light_cube.Draw(sr.light_cube_shader);
}

void render_spot_light(SceneResources& sr, LightData& light_data){
	sr.phong_shader.use();
	if(g_context.use_blinn)
		sr.phong_shader.set_float("material.shininess", 128.0f);
	else
		sr.phong_shader.set_float("material.shininess", 32.0f);

	sr.phong_shader.set_vec3("viewPos", g_context.camera->position);
	sr.phong_shader.set_bool("hasSpecular", true);
	sr.phong_shader.set_bool("calculateShadows", false);
	sr.phong_shader.set_bool("showNormals", g_context.show_normals);
	sr.phong_shader.set_bool("useBlinn", g_context.use_blinn);
	sr.phong_shader.set_int("depth_map", 1);

	glm::mat4 model = glm::mat4(1.0f);	
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	sr.phong_shader.set_mat4("model", model);
	sr.phong_shader.set_int("depth_map", 1);
	sr.phong_shader.set_float("dirLightIntensity", 0.1);
	sr.phong_shader.set_float("pointLightIntensity", 0.0);
	sr.phong_shader.set_float("spotLightIntensity", 1.0);
	sr.monkey.Draw(sr.phong_shader);
	
	sr.light_cube_shader.use();
	model = glm::mat4(1.0f);
	model = glm::translate(model, light_data.position);
	model = glm::scale(model, glm::vec3(g_context.light_cube_size));
	sr.light_cube_shader.set_mat4("model", model);
	sr.light_cube_shader.set_vec3("lightColor", light_data.color);
	sr.light_cube.Draw(sr.light_cube_shader);
}

void render_dir_light(SceneResources& sr, LightData& light_data){
	sr.phong_shader.use();
	if(g_context.use_blinn)
		sr.phong_shader.set_float("material.shininess", 128.0f);
	else
		sr.phong_shader.set_float("material.shininess", 32.0f);

	sr.phong_shader.set_vec3("viewPos", g_context.camera->position);
	sr.phong_shader.set_bool("hasSpecular", true);
	sr.phong_shader.set_bool("showNormals", g_context.show_normals);
	sr.phong_shader.set_bool("calculateShadows", false);
	sr.phong_shader.set_bool("useBlinn", g_context.use_blinn);
	sr.phong_shader.set_int("depth_map", 1);

	glm::mat4 model = glm::mat4(1.0f);	
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	sr.phong_shader.set_mat4("model", model);
	sr.phong_shader.set_int("depth_map", 1);
	sr.phong_shader.set_float("dirLightIntensity", 1.0);
	sr.phong_shader.set_float("pointLightIntensity", 0.0);
	sr.phong_shader.set_float("spotLightIntensity", 0.0);
	sr.monkey.Draw(sr.phong_shader);
	
	sr.light_cube_shader.use();
	model = glm::mat4(1.0f);
	model = glm::translate(model, light_data.position);
	model = glm::scale(model, glm::vec3(g_context.light_cube_size));
	sr.light_cube_shader.set_mat4("model", model);
	sr.light_cube_shader.set_vec3("lightColor", light_data.color);
	sr.light_cube.Draw(sr.light_cube_shader);
}

void render_instancing(SceneResources& sr, LightData& light_data){
	glm::vec3 lightDir = glm::normalize(glm::vec3(6.0f, 0.0f, 6.0f) - light_data.position);
	sr.planet_shader.use();
	sr.planet_shader.set_vec3("viewPos", g_context.camera->position);
	sr.planet_shader.set_vec3("light.direction", lightDir);
	sr.planet_shader.set_vec3("light.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
	sr.planet_shader.set_vec3("light.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
	sr.planet_shader.set_vec3("light.specular", glm::vec3(0.05f, 0.05f, 0.05f));
	sr.planet_shader.set_vec3("specular_value", glm::vec3(0.02f, 0.02f, 0.02f));
	sr.planet_shader.set_float("shininess", 32.0f);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
	sr.planet_shader.set_mat4("model", model);
	sr.planet.Draw(sr.planet_shader);

	sr.rock_shader.use();
	sr.rock_shader.set_vec3("viewPos", g_context.camera->position);
	sr.rock_shader.set_vec3("light.direction", lightDir);
	sr.rock_shader.set_vec3("light.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
	sr.rock_shader.set_vec3("light.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
	sr.rock_shader.set_vec3("viewPos", g_context.camera->position);
	sr.rock_shader.set_float("time", glfwGetTime() * 0.03f);
	for(unsigned int i = 0; i < sr.rock.meshes.size(); i++){
		glBindVertexArray(sr.rock.meshes[i].VAO);
		glDrawElementsInstanced(GL_TRIANGLES, sr.rock.meshes[i].indices.size(), GL_UNSIGNED_INT, 0, g_context.rock_count);
		glBindVertexArray(0);
	}

	sr.light_cube_shader.use();
	model = glm::mat4(1.0f);
	model = glm::translate(model, light_data.position);
	model = glm::scale(model, glm::vec3(g_context.light_cube_size));
	sr.light_cube_shader.set_mat4("model", model);
	sr.light_cube_shader.set_vec3("lightColor", light_data.color);
	sr.light_cube.Draw(sr.light_cube_shader);
}

void render_texturing(SceneResources& sr, LightData& light_data){
	sr.procedural_shader.use();
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.1f, 0.0f));
	model = glm::rotate(model, (float)glm::radians(glfwGetTime() * 5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.5));
	sr.procedural_shader.set_mat4("model", model);
	sr.procedural_shader.set_float("time", glfwGetTime());
	sr.procedural_shader.set_float("speed", g_context.cloud_speed);
	sr.procedural_shader.set_float("scale", g_context.scale);
	sr.procedural_shader.set_int("fbm_octaves", g_context.octaves);
	sr.cube.Draw(sr.procedural_shader);

	sr.light_cube_shader.use();
	model = glm::mat4(1.0f);
	model = glm::translate(model, light_data.position);
	model = glm::scale(model, glm::vec3(g_context.light_cube_size));
	sr.light_cube_shader.set_mat4("model", model);
	sr.light_cube_shader.set_vec3("lightColor", light_data.color);
	sr.light_cube.Draw(sr.light_cube_shader);
}

void render_pedestal(SceneResources& sr, unsigned int& depth_map, glm::mat4& lightSpaceMatrix){
	sr.phong_shader.use();
	if(g_context.use_blinn)
		sr.phong_shader.set_float("material.shininess", 32.0f);
	else
		sr.phong_shader.set_float("material.shininess", 8.0f);
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -2.0f, 0.0f));
	sr.phong_shader.set_mat4("model", model);
	sr.phong_shader.set_bool("hasSpecular", false);
	sr.phong_shader.set_vec3("specular_value", glm::vec3(0.02f, 0.02f, 0.02f));
	sr.phong_shader.set_int("depth_map", 1);
	sr.phong_shader.set_bool("calculateShadows", true);
	sr.phong_shader.set_mat4("lightSpaceMatrix", lightSpaceMatrix);
	if(g_context.demo_mode == 0){
		sr.phong_shader.set_float("dirLightIntensity", 0.0f);
		sr.phong_shader.set_float("pointLightIntensity", 1.0f);
		sr.phong_shader.set_float("spotLightIntensity", 0.0f);
		sr.phong_shader.set_bool("useSmallerBias", false);
	}
	else if(g_context.demo_mode == 1){
		sr.phong_shader.set_float("dirLightIntensity", 0.0f);
		sr.phong_shader.set_float("pointLightIntensity", 0.0f);
		sr.phong_shader.set_float("spotLightIntensity", 1.0f);
		sr.phong_shader.set_bool("useSmallerBias", false);
	}
	else{
		sr.phong_shader.set_float("dirLightIntensity", 1.0f);
		sr.phong_shader.set_float("pointLightIntensity", 0.0f);
		sr.phong_shader.set_float("spotLightIntensity", 0.0f);
		sr.phong_shader.set_bool("useSmallerBias", true);
	}
	sr.pedestal.Draw(sr.phong_shader);
}

void render_floor(SceneResources& sr, unsigned int& depth_map, glm::mat4& lightSpaceMatrix){
	sr.phong_shader.use();
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -2.5f, 0.0f));
	model = glm::scale(model, glm::vec3(3.0f, 1.0f, 3.0f));
	sr.phong_shader.set_mat4("model", model);	
	sr.phong_shader.set_bool("hasSpecular", false);
	sr.phong_shader.set_mat4("lightSpaceMatrix", lightSpaceMatrix);
	sr.phong_shader.set_int("depth_map", 1);
	sr.phong_shader.set_bool("calculateShadows", true);
	sr.phong_shader.set_vec3("specular_value", glm::vec3(0.1));
	if(g_context.demo_mode == 0){
		sr.phong_shader.set_float("dirLightIntensity", 0.0f);
		sr.phong_shader.set_float("pointLightIntensity", 1.0f);
		sr.phong_shader.set_float("spotLightIntensity", 0.0f);
		sr.phong_shader.set_bool("useSmallerBias", false);
	}
	else if(g_context.demo_mode == 1){
		sr.phong_shader.set_float("dirLightIntensity", 0.0f);
		sr.phong_shader.set_float("pointLightIntensity", 0.0f);
		sr.phong_shader.set_float("spotLightIntensity", 1.0f);
		sr.phong_shader.set_bool("useSmallerBias", false);
	}
	else{
		sr.phong_shader.set_float("dirLightIntensity", 1.0f);
		sr.phong_shader.set_float("pointLightIntensity", 0.0f);
		sr.phong_shader.set_float("spotLightIntensity", 0.0f);
		sr.phong_shader.set_bool("useSmallerBias", true);
	}
	if(g_context.use_blinn)
		sr.phong_shader.set_float("material.shininess", 64.0f);
	else
		sr.phong_shader.set_float("material.shininess", 32.0f);
	sr.scene_floor.Draw(sr.phong_shader);
}

void render_skybox(SceneResources& sr, unsigned int& texture){
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	sr.skybox_shader.use();
	sr.skybox_shader.set_int("skybox", 2);
	sr.skybox.Draw(sr.skybox_shader);
}

void render_gui(){
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowPos(ImVec2(0.0f, 30.0f));
	ImGui::SetNextWindowSize(ImVec2(400.0f, 600.0f));

	ImGui::Begin("Rendering Demo");
	ImGui::Text("Demo mode");
	ImGui::Separator();

	ImGui::RadioButton("Point lighting", &g_context.demo_mode, 0);
	if(g_context.demo_mode == 0){
		ImGui::Indent(20.0f);
		ImGui::RadioButton("Phong", &g_context.use_blinn, 0);
		ImGui::RadioButton("Blinn-Phong", &g_context.use_blinn, 1);
		ImGui::Checkbox("Rotate light", &g_context.rotate_light);
		ImGui::SliderFloat("Rotation speed", &g_context.point_light_rotation_speed, -3.0f, 3.0f, "%.1f");
		ImGui::SliderFloat("Rotation radius", &g_context.point_light_rotation_radius, 0.0f, 5.0f, "%.2f");
		ImGui::Unindent(20.0f);
	}

	ImGui::RadioButton("Spot lighting", &g_context.demo_mode, 1);
	if(g_context.demo_mode == 1){
		ImGui::Indent(20.0f);
		ImGui::RadioButton("Phong", &g_context.use_blinn, 0);
		ImGui::RadioButton("Blinn-Phong", &g_context.use_blinn, 1);
		ImGui::Unindent(20.0f);
	}

	ImGui::RadioButton("Directional lighting", &g_context.demo_mode, 2);
	if(g_context.demo_mode == 2){
		ImGui::Indent(20.0f);
		ImGui::RadioButton("Phong", &g_context.use_blinn, 0);
		ImGui::RadioButton("Blinn-Phong", &g_context.use_blinn, 1);
		ImGui::Unindent(20.0f);
	}

	ImGui::RadioButton("Instancing", &g_context.demo_mode, 3);
	if(g_context.demo_mode == 3){
		ImGui::Indent(20.0f);
		ImGui::SliderInt("Instance count", &g_context.rock_count, 0, g_context.max_rock_count);
		ImGui::Unindent(20.0f);
	}

	ImGui::RadioButton("Particles", &g_context.demo_mode, 4);
	if(g_context.demo_mode == 4){
		ImGui::Indent(20.0f);
		ImGui::SliderInt("Particle count", &g_context.particle_count, 0, g_context.max_particle_count);
		ImGui::SliderFloat("Particle lifetime", &g_context.particle_lifetime, 0, g_context.max_particle_lifetime);
		ImGui::SliderFloat("Cone height", &g_context.cone_height, 0, 1.5f);
		ImGui::SliderFloat("Cone top radius", &g_context.top_radius, 0, 1.0f);
		ImGui::Unindent(20.0f);
	}
	
	ImGui::RadioButton("Procedural", &g_context.demo_mode, 5);
	if(g_context.demo_mode == 5){
		ImGui::Indent(20.0f);
		ImGui::SliderInt("FBM Octaves", &g_context.octaves, 1, 8);
		ImGui::SliderFloat("Scale", &g_context.scale, 1.0, 2.0);
		ImGui::SliderFloat("Cloud speed", &g_context.cloud_speed, 0.01, 1.0, "%.2f");
		ImGui::Unindent(20.0f);
	}
	ImGui::Separator();

	ImGui::NewLine();
	ImGui::Text("Post processing");
	ImGui::Separator();

	ImGui::RadioButton("Inverse", &g_context.post_processing_mode, 1);
	ImGui::RadioButton("Grayscale", &g_context.post_processing_mode, 2);
	ImGui::RadioButton("Gaussian Blur", &g_context.post_processing_mode, 3);

	ImGui::Checkbox("Gamma Correction", &g_context.use_gamma_correction);
	ImGui::SliderFloat("Gamma", &g_context.gamma, 0.0f, 3.0f, "%.1f");
	ImGui::Separator();

	ImGui::NewLine();
	ImGui::Text("Performance");
	ImGui::Separator();
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Separator();

	ImGui::NewLine();
	ImGui::Text("Debugging");
	ImGui::Separator();
	ImGui::Checkbox("Show normals", &g_context.show_normals);
	ImGui::Checkbox("Show depth map", &g_context.show_depth_map);
	ImGui::Separator();

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
