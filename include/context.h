#pragma once
#include "core.h"

class Context {
	public: 
	GLFWwindow* window;
	Camera* camera;
	float win_width;
	float win_height;
	float delta_time;
	float cone_height = 1.5f;
	float top_radius = 0.7f;
	float light_cube_size = 0.05f;

	bool show_normals;
	bool rotate_light;
	bool use_blinn = true;
	bool use_gamma_correction = true;
	int post_processing_mode;
	int showcase_mode;

	unsigned int rock_amount = 10000;
	unsigned int particle_count = 10000;

	const unsigned int shadow_width = 1024;
	const unsigned int shadow_height = 1024;
	
	Context(){}
};
