#pragma once
#include "core.h"

void render_depth_map(SceneResources& sr, glm::mat4& lightSpaceMatrix);
void render_postprocess(SceneResources& sr, unsigned int& vao, unsigned int& texture);
void render_particles(SceneResources& sr, unsigned int& vao, unsigned int& vbo, LightData& light_data);
void render_point_light(SceneResources& sr, LightData& light_data);
void render_spot_light(SceneResources& sr, LightData& light_data);
void render_dir_light(SceneResources& sr, LightData& light_data);
void render_instancing(SceneResources& sr, LightData& light_data);
void render_texturing(SceneResources& sr, LightData& light_data);
void render_pedestal(SceneResources& sr, unsigned int& depth_map, glm::mat4& lightSpaceMatrix);
void render_floor(SceneResources& sr, unsigned int& depth_map, glm::mat4& lightSpaceMatrix);
void render_skybox(SceneResources& sr, unsigned int& texture);
void render_gui();
