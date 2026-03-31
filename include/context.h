#pragma once
#include "core.h"
#include "structs.h"

class Context {
	public: 
	GLFWwindow* window;
	Camera* camera;
	float win_width;
	float win_height;
	float delta_time;
	float cone_height = 1.3f;
	float top_radius = 0.6f;
	float light_cube_size = 0.05f;
	float gamma = 2.2f;
	float point_light_rotation_speed = 1.2f;
	float point_light_rotation_radius = 3.0f;
	float particle_lifetime = 2.0f;
	const float max_particle_lifetime = 5.0f;
	float cloud_speed = 0.01f;
	float scale = 1.0f;
	float max_bias = 0.05f;
	float min_bias = 0.005f;

	bool show_normals;
	bool show_depth_map;
	bool rotate_light;
	bool use_gamma_correction = true;
	bool capture_mouse = true;
	bool resize_requested = false;
	bool cull_front_faces = false;

	int post_processing_mode;
	int demo_mode;
	int use_blinn = 1;
	int octaves = 5;

	const unsigned int max_rock_count = 50000;
	const unsigned int max_particle_count = 50000;
	int rock_count = 10000;
	int particle_count = 10000;

	const unsigned int shadow_width = 1024;
	const unsigned int shadow_height = 1024;

	std::vector<Particle> particles_list;
	Context(){
		particles_list.resize(max_particle_count);
	}
};

extern Context g_context;
