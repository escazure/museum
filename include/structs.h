#pragma once
#include "core.h"

struct Particle {
	glm::vec3 position;
	glm::vec3 velocity;
	float life;
	float max_life;
	float scale;
};

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
