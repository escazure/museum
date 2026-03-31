#pragma once
#include "gl3w.h"
#include "glfw3.h"
#include "shader_program.h"
#include "camera.h"
#include "model.h"
#include "context.h"
#include "structs.h"
#include "render.h"
#include "setup.h"
#include "stb_image.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <vector>
#include <random>

void init();
void run();
void shutdown();
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_resize_callback(GLFWwindow* window, int width, int height);
void process_input();
void resize_framebuffer(unsigned int& fbo, unsigned int& texture);
void update_ubos(unsigned int& matrix_ubo, unsigned int& light_ubo, glm::vec3& lightPos);
void update_particles();
void respawn(Particle& p);
unsigned int loadCubeMap(std::vector<std::string> faces);
SceneResources load_scene_resources();
