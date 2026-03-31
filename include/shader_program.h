#pragma once 

#include "glfw3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

struct Shader {
	unsigned int id;

	Shader(const char* vertex_path,const char* fragment_path,const char* geometry_path = nullptr){
		std::string vertex_code;
		std::string geometry_code;
		std::string fragment_code;
		std::ifstream v_shader_file;
		std::ifstream g_shader_file;
		std::ifstream f_shader_file;

		v_shader_file.exceptions (std::ifstream::failbit | std::ifstream::badbit);
		g_shader_file.exceptions (std::ifstream::failbit | std::ifstream::badbit);
		f_shader_file.exceptions (std::ifstream::failbit | std::ifstream::badbit);

		try{
			v_shader_file.open(vertex_path);
			f_shader_file.open(fragment_path);
			std::stringstream v_shader_stream, f_shader_stream;

			v_shader_stream << v_shader_file.rdbuf();
			f_shader_stream << f_shader_file.rdbuf();

			v_shader_file.close();
			f_shader_file.close();

			vertex_code = v_shader_stream.str();
			fragment_code = f_shader_stream.str();

			if(geometry_path){
				g_shader_file.open(geometry_path);
				std::stringstream g_shader_stream;
				g_shader_stream << g_shader_file.rdbuf();
				g_shader_file.close();
				geometry_code = g_shader_stream.str();
			}
		}
		catch(std::ifstream::failure e){
			std::cerr << "SHADER: File not read" << std::endl;
		}
		const char* v_shader_code = vertex_code.c_str();
		const char* f_shader_code = fragment_code.c_str();

		unsigned int vertex, geometry, fragment;
		int success;
		char info_log[512];

		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &v_shader_code, NULL);
		glCompileShader(vertex);
		
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if(!success){
			glGetShaderInfoLog(vertex, 512, NULL, info_log);	
			std::cerr << "SHADER: Vertex shader compilation failed:\n" << info_log << std::endl;
		}

		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &f_shader_code, NULL);
		glCompileShader(fragment);
		
		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if(!success){
			glGetShaderInfoLog(fragment, 512, NULL, info_log);	
			std::cerr << "SHADER: Fragment shader compilation failed:\n" << info_log << std::endl;
		}
		
		id = glCreateProgram();
		glAttachShader(id, vertex);

		if(geometry_path){
			const char* g_shader_code = geometry_code.c_str();
			geometry = glCreateShader(GL_GEOMETRY_SHADER);
			glShaderSource(geometry, 1, &g_shader_code, NULL);
			glCompileShader(geometry);
			
			glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
			if(!success){
				glGetShaderInfoLog(geometry, 512, NULL, info_log);	
				std::cerr << "SHADER: Geometry shader compilation failed:\n" << info_log << std::endl;
			}
			glAttachShader(id, geometry);
		}

		glAttachShader(id, fragment);
		glLinkProgram(id);

		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if(!success){
			glGetProgramInfoLog(id, 512, NULL, info_log);
			std::cerr << "SHADER: Failed to link shaders:\n" << info_log << std::endl;
		}

		glDeleteShader(vertex);
		if(geometry_path)
			glDeleteShader(geometry);
		glDeleteShader(fragment);
	}

	void use(){
		glUseProgram(id);
	}

	void bind_uniform_block(const std::string &name, unsigned int value){
		glUniformBlockBinding(id, glGetUniformBlockIndex(id, name.c_str()), value);
	}

	void set_bool(const std::string &name, bool value) const {
		glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
	}
	void set_int(const std::string &name, int value) const {
		glUniform1i(glGetUniformLocation(id, name.c_str()), value);
	}
	void set_float(const std::string &name, float value) const {
		glUniform1f(glGetUniformLocation(id, name.c_str()), value);
	}
	void set_mat4(const std::string &name, glm::mat4 &mat) const {
		glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	void set_vec3(const std::string &name, float x, float y, float z) const {
		glUniform3f(glGetUniformLocation(id, name.c_str()), x, y, z);
	}
	void set_vec3(const std::string &name, glm::vec3 vec) const {
		glUniform3f(glGetUniformLocation(id, name.c_str()), vec.x, vec.y, vec.z);
	}
};
