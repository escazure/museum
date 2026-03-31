#include "render.h"

void render_depth_map(SceneResources& sr, glm::mat4& lightSpaceMatrix){
	sr.depth_shader.use();
	sr.depth_shader.set_mat4("lightSpaceMatrix", lightSpaceMatrix);
		
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -2.5f, 0.0f));
	model = glm::scale(model, glm::vec3(3.0f, 1.0f, 3.0f));
	sr.depth_shader.set_mat4("model", model);	
	sr.scene_floor.Draw(sr.depth_shader);

	if(g_context.cull_front_faces)
		glCullFace(GL_FRONT);

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

	if(g_context.cull_front_faces)
		glCullFace(GL_BACK);
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
	sr.light_cube_shader.set_bool("showNormals", g_context.show_normals);
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
	sr.phong_shader.set_float("max_bias", g_context.max_bias);
	sr.phong_shader.set_float("min_bias", g_context.min_bias);
	sr.monkey.Draw(sr.phong_shader);
	
	sr.light_cube_shader.use();
	model = glm::mat4(1.0f);
	model = glm::translate(model, light_data.position);
	model = glm::scale(model, glm::vec3(g_context.light_cube_size));
	sr.light_cube_shader.set_mat4("model", model);
	sr.light_cube_shader.set_vec3("lightColor", light_data.color);
	sr.light_cube_shader.set_bool("showNormals", g_context.show_normals);
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
	sr.phong_shader.set_float("max_bias", g_context.max_bias);
	sr.phong_shader.set_float("min_bias", g_context.min_bias);

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
	sr.light_cube_shader.set_bool("showNormals", g_context.show_normals);
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
	sr.phong_shader.set_float("max_bias", g_context.max_bias);
	sr.phong_shader.set_float("min_bias", g_context.min_bias);

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
	sr.light_cube_shader.set_bool("showNormals", g_context.show_normals);
	sr.light_cube.Draw(sr.light_cube_shader);
}

void render_instancing(SceneResources& sr, LightData& light_data){
	glm::vec3 lightDir = glm::normalize(glm::vec3(6.0f, 0.0f, 6.0f) - light_data.position);
	sr.planet_shader.use();
	sr.planet_shader.set_vec3("viewPos", g_context.camera->position);
	sr.planet_shader.set_bool("showNormals", g_context.show_normals);
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
	sr.rock_shader.set_bool("showNormals", g_context.show_normals);
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
	sr.light_cube_shader.set_bool("showNormals", g_context.show_normals);
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
	sr.procedural_shader.set_bool("showNormals", g_context.show_normals);
	sr.cube.Draw(sr.procedural_shader);

	sr.light_cube_shader.use();
	model = glm::mat4(1.0f);
	model = glm::translate(model, light_data.position);
	model = glm::scale(model, glm::vec3(g_context.light_cube_size));
	sr.light_cube_shader.set_mat4("model", model);
	sr.light_cube_shader.set_vec3("lightColor", light_data.color);
	sr.light_cube_shader.set_bool("showNormals", g_context.show_normals);
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
	sr.phong_shader.set_bool("showNormals", g_context.show_normals);
	sr.phong_shader.set_vec3("specular_value", glm::vec3(0.02f, 0.02f, 0.02f));
	sr.phong_shader.set_int("depth_map", 1);
	sr.phong_shader.set_bool("calculateShadows", true);
	sr.phong_shader.set_mat4("lightSpaceMatrix", lightSpaceMatrix);
	sr.phong_shader.set_float("max_bias", g_context.max_bias);
	sr.phong_shader.set_float("min_bias", g_context.min_bias);
	if(g_context.demo_mode == 0){
		sr.phong_shader.set_float("dirLightIntensity", 0.0f);
		sr.phong_shader.set_float("pointLightIntensity", 1.0f);
		sr.phong_shader.set_float("spotLightIntensity", 0.0f);
	}
	else if(g_context.demo_mode == 1){
		sr.phong_shader.set_float("dirLightIntensity", 0.0f);
		sr.phong_shader.set_float("pointLightIntensity", 0.0f);
		sr.phong_shader.set_float("spotLightIntensity", 1.0f);
	}
	else{
		sr.phong_shader.set_float("dirLightIntensity", 1.0f);
		sr.phong_shader.set_float("pointLightIntensity", 0.0f);
		sr.phong_shader.set_float("spotLightIntensity", 0.0f);
	}
	sr.pedestal.Draw(sr.phong_shader);
}

void render_floor(SceneResources& sr, unsigned int& depth_map, glm::mat4& lightSpaceMatrix){
	sr.phong_shader.use();
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -2.5f, 0.0f));
	model = glm::scale(model, glm::vec3(3.0f, 1.0f, 3.0f));
	sr.phong_shader.set_mat4("model", model);	
	sr.phong_shader.set_bool("showNormals", g_context.show_normals);
	sr.phong_shader.set_bool("hasSpecular", false);
	sr.phong_shader.set_mat4("lightSpaceMatrix", lightSpaceMatrix);
	sr.phong_shader.set_int("depth_map", 1);
	sr.phong_shader.set_bool("calculateShadows", true);
	sr.phong_shader.set_vec3("specular_value", glm::vec3(0.1));
	sr.phong_shader.set_float("max_bias", g_context.max_bias);
	sr.phong_shader.set_float("min_bias", g_context.min_bias);
	if(g_context.demo_mode == 0){
		sr.phong_shader.set_float("dirLightIntensity", 0.0f);
		sr.phong_shader.set_float("pointLightIntensity", 1.0f);
		sr.phong_shader.set_float("spotLightIntensity", 0.0f);
	}
	else if(g_context.demo_mode == 1){
		sr.phong_shader.set_float("dirLightIntensity", 0.0f);
		sr.phong_shader.set_float("pointLightIntensity", 0.0f);
		sr.phong_shader.set_float("spotLightIntensity", 1.0f);
	}
	else{
		sr.phong_shader.set_float("dirLightIntensity", 1.0f);
		sr.phong_shader.set_float("pointLightIntensity", 0.0f);
		sr.phong_shader.set_float("spotLightIntensity", 0.0f);
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
	ImGui::SetNextWindowSize(ImVec2(420.0f, 700.0f));

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
		ImGui::SliderFloat("Rotation radius", &g_context.point_light_rotation_radius, 0.01f, 5.0f, "%.2f");
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
	ImGui::Checkbox("Enable front face culling", &g_context.cull_front_faces);
	ImGui::SliderFloat("Max shadow bias", &g_context.max_bias, 0.001f, 0.05f, "%.3f");
	ImGui::SliderFloat("Min shadow bias", &g_context.min_bias, 0.0001f, 0.005f, "%.4f");
	ImGui::Separator();

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
