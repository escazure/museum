#pragma once
#include "core.h"

void screen_quad_setup(unsigned int &vao, unsigned int &vbo);
void setup_screen_fbo(unsigned int& fbo, unsigned int& texture, unsigned int& rbo);
void setup_shadow_map(unsigned int& depth_fbo, unsigned int& depth_map);
void setup_rock_instancing(Model& rock, unsigned int& vbo);
void setup_particle_system(unsigned int& vao, unsigned int& vbo);
void setup_lights(PointLight& p1, PointLight& p2, DirLight& dir, SpotLight& spot, LightData& point_light_data, LightData& spot_light_data, LightData& dir_light_data);
void setup_ubos(unsigned int& matrix_ubo, unsigned int& light_ubo, PointLight& p1, PointLight& p2, DirLight& dir, SpotLight& spot);
