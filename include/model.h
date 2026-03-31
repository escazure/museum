#pragma once
#include "stb_image.h"
#include "core.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

inline unsigned int load_texture_2d(const char* path, bool is_diffuse, bool do_tile = true){
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);	

	if(do_tile){
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, channels;
	unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
	if(channels == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, is_diffuse ? GL_SRGB : GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	else if(channels == 4)
		glTexImage2D(GL_TEXTURE_2D, 0, is_diffuse ? GL_SRGB_ALPHA : GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
		std::cerr << "ERROR:FAILED_TO_LOAD_TEXTURE" << std::endl;
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);
	return texture;
}

struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
};

struct Texture {
	unsigned int id;
	std::string type;
	std::string path;
};

class Mesh {
	public:
		unsigned int VAO, VBO, EBO;
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture> textures;

		Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures){
			this->vertices = vertices;
			this->indices = indices;
			this->textures = textures;

			setupMesh();
		}

		void Draw(Shader &shader){
			unsigned int diffuseNr = 1;	
			unsigned int specularNr= 1;	
			for(unsigned int i = 0; i < textures.size(); i++){	
				glActiveTexture(GL_TEXTURE0 + i);

				std::string number;
				std::string name = textures[i].type;
				if(name == "texture_diffuse")
					number = std::to_string(diffuseNr++);
				else if(name == "texture_specular")
					number = std::to_string(specularNr++);

				shader.set_int(("material." + name + number).c_str(), i);
				glBindTexture(GL_TEXTURE_2D, textures[i].id);
			}

			glActiveTexture(GL_TEXTURE0);

			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}

	private:
		void setupMesh(){
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);
			
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
			glEnableVertexAttribArray(2);

			glBindVertexArray(0);
		}
};

class Model{
	public:
		std::vector<Mesh> meshes;
		Model(const char* path){
			loadModel(path);
		}

		void Draw(Shader &shader){
			for(unsigned int i = 0; i < meshes.size(); i++){
				meshes[i].Draw(shader);
			}
		}

	private:
		std::vector<Texture> textures_loaded;
		std::string directory;

		void loadModel(std::string path){
			Assimp::Importer import;
			const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

			if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
				std::cerr << "ERROR::FAILED_TO_LOAD_SCENE: " << import.GetErrorString() << std::endl;
				return;
			} 

			directory = path.substr(0, path.find_last_of('/'));
			processNode(scene->mRootNode, scene);
		}

		void processNode(aiNode* node, const aiScene* scene){
			for(unsigned int i = 0; i < node->mNumMeshes; i++){
				aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				meshes.push_back(processMesh(mesh, scene));
			}

			for(unsigned int i = 0; i < node->mNumChildren; i++){
				processNode(node->mChildren[i], scene);
			}
		}

		Mesh processMesh(aiMesh* mesh, const aiScene* scene){
			std::vector<Vertex> vertices;	
			std::vector<unsigned int> indices;
			std::vector<Texture> textures;

			for(unsigned int i = 0; i < mesh->mNumVertices; i++){
				Vertex vertex;
				glm::vec3 vector;
				vector.x = mesh->mVertices[i].x;
				vector.y = mesh->mVertices[i].y;
				vector.z = mesh->mVertices[i].z;
				vertex.Position = vector;

				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.Normal = vector;

				if(mesh->mTextureCoords[0]){
					glm::vec2 vec;
					vec.x = mesh->mTextureCoords[0][i].x;
					vec.y = mesh->mTextureCoords[0][i].y;
					vertex.TexCoords = vec;
				}
				else
					vertex.TexCoords = glm::vec2(0.0f, 0.0f);
				vertices.push_back(vertex);
			}

			for(unsigned int i = 0; i < mesh->mNumFaces; i++){
				aiFace face = mesh->mFaces[i];
				for(unsigned int j = 0; j < face.mNumIndices; j++)
					indices.push_back(face.mIndices[j]);
			}

			if(mesh->mMaterialIndex >= 0){
				aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];	
				std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
				textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
				std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
				textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
			}
			return Mesh(vertices, indices, textures);
		}

		std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName){
			std::vector<Texture> textures;
			for(unsigned int i = 0; i < mat->GetTextureCount(type); i++){
				aiString str;
				mat->GetTexture(type, i, &str);
				bool skip = false;
				for(unsigned int j = 0; j < textures_loaded.size(); j++){
					if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0){
						textures.push_back(textures_loaded[j]);
						skip = true;
						break;
					}
				}

				if(!skip){
					Texture texture;
					std::string filename = directory + '/' + str.C_Str();
					if(type == aiTextureType_DIFFUSE)
						texture.id = load_texture_2d(filename.c_str(), true);
					if(type == aiTextureType_SPECULAR)
						texture.id = load_texture_2d(filename.c_str(), false);
					texture.type = typeName;
					texture.path = str.C_Str();
					textures.push_back(texture);
					textures_loaded.push_back(texture);
				}
			}
			return textures;
		}
};
