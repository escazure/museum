#pragma once
#include "gl3w.h"
#include "glfw3.h"
#include "shader_program.h"
#include "camera.h"
#include "model.h"
#include "context.h"
#include "stb_image.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <vector>
#include <random>

struct PointLight {
	glm::vec4 position;
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;
};

struct DirLight {
	glm::vec4 direction;
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;
};

struct SpotLight {
	glm::vec4 position;
	glm::vec4 direction;
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;
};

struct LightData {
	glm::vec3 position;
	glm::vec3 color;
};

struct SceneResources {
    Model monkey;
    Model pedestal;
    Model light_cube;
    Model scene_floor;
    Model skybox;
    Model cube;
    Model planet;
    Model rock;

    Shader phong_shader;
    Shader light_cube_shader;
    Shader skybox_shader;
    Shader procedural_shader;
    Shader planet_shader;
    Shader rock_shader;
    Shader particle_shader;
    Shader depth_shader;
    Shader screen_shader;

	SceneResources():
		monkey("models/Monkey/monkey.obj"),
        pedestal("models/Pillar/pillar.obj"),
        light_cube("models/Cube/cube.obj"),
        scene_floor("models/MuseumSceneFloor/scene_floor.obj"),
        skybox("models/Skybox/cube.obj"),
        cube("models/Cube/cube.obj"),
		planet("models/planet/planet.obj"),
        rock("models/rock/rock.obj"),

		depth_shader("shaders/depth_mapping/vertex.glsl", "shaders/depth_mapping/fragment.glsl"),
        screen_shader("shaders/screen/vertex.glsl", "shaders/screen/fragment.glsl"),
        phong_shader("shaders/phong/vertex.glsl", "shaders/phong/fragment.glsl"),
        light_cube_shader("shaders/light_cube/vertex.glsl", "shaders/light_cube/fragment.glsl"),
        skybox_shader("shaders/skybox/vertex.glsl", "shaders/skybox/fragment.glsl"),
        procedural_shader("shaders/procedural/vertex.glsl", "shaders/procedural/fragment.glsl"),
        planet_shader("shaders/planet/vertex.glsl", "shaders/planet/fragment.glsl"),
        rock_shader("shaders/rock/vertex.glsl", "shaders/rock/fragment.glsl"),
        particle_shader("shaders/particles/vertex.glsl", "shaders/particles/fragment.glsl", "shaders/particles/geometry.glsl")
	{
		
		phong_shader.bind_uniform_block("Matrices", 0);
        phong_shader.bind_uniform_block("Light", 1);
        light_cube_shader.bind_uniform_block("Matrices", 0);
        skybox_shader.bind_uniform_block("Matrices", 0);
        procedural_shader.bind_uniform_block("Matrices", 0);	
	}
};

void init();
void run();
void shutdown();
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_resize_callback(GLFWwindow* window, int width, int height);
void process_input();
void resize_framebuffer(unsigned int& fbo, unsigned int& texture);
void screen_quad_setup(unsigned int &vao, unsigned int &vbo);
void setup_screen_fbo(unsigned int& fbo, unsigned int& texture, unsigned int& rbo);
void setup_shadow_map(unsigned int& depth_fbo, unsigned int& depth_map);
void setup_rock_instancing(Model& rock, unsigned int& vbo);
void setup_particle_system(unsigned int& vao, unsigned int& vbo);
void setup_lights(PointLight& p1, PointLight& p2, DirLight& dir, SpotLight& spot);
void setup_ubos(unsigned int& matrix_ubo, unsigned int& light_ubo, PointLight& p1, PointLight& p2, DirLight& dir, SpotLight& spot);
void update_ubos(unsigned int& matrix_ubo, unsigned int& light_ubo, glm::vec3& lightPos);
void render_depth_map(SceneResources& sr, glm::mat4& lightSpaceMatrix);
void render_postprocess(SceneResources& sr, unsigned int& vao, unsigned int& texture);
void render_particles(SceneResources& sr, unsigned int& vao, unsigned int& vbo);
void render_point_light(SceneResources& sr, LightData& light_data);
void render_spot_light(SceneResources& sr, LightData& light_data);
void render_dir_light(SceneResources& sr, LightData& light_data);
void render_instancing(SceneResources& sr, LightData& light_data);
void render_texturing(SceneResources& sr);
void render_pedestal(SceneResources& sr, unsigned int& depth_map);
void render_floor(SceneResources& sr, unsigned int& depth_map, glm::mat4& lightSpaceMatrix);
void render_skybox(SceneResources& sr, unsigned int& texture);
void render_gui();

void update_particles();
void respawn(Particle& p);
unsigned int loadCubeMap(std::vector<std::string> faces);
SceneResources load_scene_resources();
