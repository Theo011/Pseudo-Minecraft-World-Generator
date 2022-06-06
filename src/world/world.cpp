#include "world.h"

#include <iostream>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <FastNoiseLite.h>

#include "../engine/filesystem.h"

World::World(const int& seed, const int& x_max, const int& z_max, const int& y_max)
	: m_seed(seed), m_x_max(x_max), m_z_max(z_max), m_y_max(y_max),
	  m_dirt_amount(0), m_stone_amount(0), m_bedrock_amount(0), m_grass_amount(0)
{
	std::vector<std::vector<float>> noiseData(x_max, std::vector<float>(z_max));
	m_noiseData = noiseData;
	load_noise();
	load_models();
	calculate_blocks();
	setup_world();
}

World::~World()
{
	delete[] m_dirt_matrices;
	delete[] m_stone_matrices;
	delete[] m_bedrock_matrices;
	delete[] m_grass_matrices;
}

void World::render_world(Camera& camera, const glm::mat4& projection)
{
	m_general_block_shader.use();
	m_general_block_shader.setInt("diffuseMap", 0);
	m_general_block_shader.setMat4("projection", projection);
	m_general_block_shader.setMat4("view", camera.GetViewMatrix());
	m_general_block_shader.setVec3("viewPos", camera.Position);
	m_general_block_shader.setVec3("light.direction", -0.2f, -1.0f, -0.3f);
	m_general_block_shader.setVec3("light.ambient", 0.3f, 0.3f, 0.3f);
	m_general_block_shader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_block_models[GRASS].textures_loaded[0].id);
	
	for (unsigned int i = 0; i < m_block_models[GRASS].meshes.size(); ++i)
	{
		glBindVertexArray(m_block_models[GRASS].meshes[i].VAO);
		glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(m_block_models[GRASS].meshes[i].indices.size()), GL_UNSIGNED_INT, 0, m_grass_amount);
		glBindVertexArray(0);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_block_models[BEDROCK].textures_loaded[0].id);

	for (unsigned int i = 0; i < m_block_models[BEDROCK].meshes.size(); ++i)
	{
		glBindVertexArray(m_block_models[BEDROCK].meshes[i].VAO);
		glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(m_block_models[BEDROCK].meshes[i].indices.size()), GL_UNSIGNED_INT, 0, m_bedrock_amount);
		glBindVertexArray(0);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_block_models[DIRT].textures_loaded[0].id);

	for (unsigned int i = 0; i < m_block_models[DIRT].meshes.size(); ++i)
	{
		glBindVertexArray(m_block_models[DIRT].meshes[i].VAO);
		glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(m_block_models[DIRT].meshes[i].indices.size()), GL_UNSIGNED_INT, 0, m_dirt_amount);
		glBindVertexArray(0);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_block_models[STONE].textures_loaded[0].id);

	for (unsigned int i = 0; i < m_block_models[STONE].meshes.size(); ++i)
	{
		glBindVertexArray(m_block_models[STONE].meshes[i].VAO);
		glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(m_block_models[STONE].meshes[i].indices.size()), GL_UNSIGNED_INT, 0, m_stone_amount);
		glBindVertexArray(0);
	}
}

float World::map_value(const float& x, const float& in_min, const float& in_max, const float& out_min, const float& out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void World::load_noise()
{
	FastNoiseLite noise;
	
	noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	noise.SetFrequency(0.01f);
	noise.SetSeed(m_seed);
	noise.SetFractalOctaves(5);
	noise.SetFractalLacunarity(2.0f);
	noise.SetFractalGain(0.5f);
	
	for (int x = 0; x < m_x_max; ++x)
	{
		for (int z = 0; z < m_z_max; ++z)
		{
			m_noiseData[x][z] = noise.GetNoise((float)x, (float)z);			 // ensures that height 0 will be bedrock
			m_noiseData[x][z] = round(map_value(m_noiseData[x][z], -1.0f, 1.0f, 1.0f, m_y_max));
			++m_grass_amount;
			++m_bedrock_amount;
			individual_cubes += 2;
		}
	}
}

void World::load_models()
{
	Shader temp_shader("assets/shaders/general_block_vert.glsl", "assets/shaders/general_block_frag.glsl");
	
	m_general_block_shader = temp_shader;

	Model temp_dirt_model(FileSystem::getPath("assets/models/dirt/dirt.obj"));
	Model temp_stone_model(FileSystem::getPath("assets/models/stone/stone.obj"));
	Model temp_bedrock_model(FileSystem::getPath("assets/models/bedrock/bedrock.obj"));
	Model temp_grass_model(FileSystem::getPath("assets/models/grass/grass.obj"));

	m_block_models[DIRT] = temp_dirt_model;
	m_block_models[STONE] = temp_stone_model;
	m_block_models[BEDROCK] = temp_bedrock_model;
	m_block_models[GRASS] = temp_grass_model;
}

void World::setup_world()
{
	int gb = 0;
	int d = 0;
	int s = 0;

	m_grass_matrices = new glm::mat4[m_grass_amount];
	m_bedrock_matrices = new glm::mat4[m_bedrock_amount];
	m_dirt_matrices = new glm::mat4[m_dirt_amount];
	m_stone_matrices = new glm::mat4[m_stone_amount];

	for (int x = 0; x < m_x_max; ++x)
	{
		for (int z = 0; z < m_z_max; ++z)
		{
			glm::mat4 grass_model = glm::mat4(1.0f);
			glm::mat4 bedrock_model = glm::mat4(1.0f);

			grass_model = glm::translate(grass_model, glm::vec3(x, m_noiseData[x][z], z));
			grass_model = glm::scale(grass_model, glm::vec3(0.5f));
			m_grass_matrices[gb] = grass_model;

			bedrock_model = glm::translate(bedrock_model, glm::vec3(x, 0, z));
			bedrock_model = glm::scale(bedrock_model, glm::vec3(0.5f));
			m_bedrock_matrices[gb] = bedrock_model;

			int y = m_noiseData[x][z];
			int h = y;

			while (h > 0)
			{
				if (y - h <= 8)
				{
					glm::mat4 dirt_model = glm::mat4(1.0f);

					dirt_model = glm::translate(dirt_model, glm::vec3(x, h, z));
					dirt_model = glm::scale(dirt_model, glm::vec3(0.5f));
					m_dirt_matrices[d] = dirt_model;
					++d;
				}
				else
				{
					glm::mat4 stone_model = glm::mat4(1.0f);

					stone_model = glm::translate(stone_model, glm::vec3(x, h, z));
					stone_model = glm::scale(stone_model, glm::vec3(0.5f));
					m_stone_matrices[s] = stone_model;
					++s;
				}

				--h;
			}

			++gb;
		}
	}
	
	glGenBuffers(1, &m_grass_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_grass_buffer);
	glBufferData(GL_ARRAY_BUFFER, m_grass_amount * sizeof(glm::mat4), &m_grass_matrices[0], GL_STATIC_DRAW);

	for (int i = 0; i < m_block_models[GRASS].meshes.size(); ++i)
	{
		glBindVertexArray(m_block_models[GRASS].meshes[i].VAO);

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);

		glBindVertexArray(0);
	}

	glGenBuffers(1, &m_bedrock_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_bedrock_buffer);
	glBufferData(GL_ARRAY_BUFFER, m_bedrock_amount * sizeof(glm::mat4), &m_bedrock_matrices[0], GL_STATIC_DRAW);

	for (int i = 0; i < m_block_models[BEDROCK].meshes.size(); ++i)
	{
		glBindVertexArray(m_block_models[BEDROCK].meshes[i].VAO);

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);

		glBindVertexArray(0);
	}

	glGenBuffers(1, &m_dirt_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_dirt_buffer);
	glBufferData(GL_ARRAY_BUFFER, m_dirt_amount * sizeof(glm::mat4), &m_dirt_matrices[0], GL_STATIC_DRAW);

	for (int i = 0; i < m_block_models[DIRT].meshes.size(); ++i)
	{
		glBindVertexArray(m_block_models[DIRT].meshes[i].VAO);

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);

		glBindVertexArray(0);
	}

	glGenBuffers(1, &m_stone_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_stone_buffer);
	glBufferData(GL_ARRAY_BUFFER, m_stone_amount * sizeof(glm::mat4), &m_stone_matrices[0], GL_STATIC_DRAW);

	for (int i = 0; i < m_block_models[STONE].meshes.size(); ++i)
	{
		glBindVertexArray(m_block_models[STONE].meshes[i].VAO);

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);

		glBindVertexArray(0);
	}
}

void World::calculate_blocks()
{
	for (int x = 0; x < m_x_max; ++x)
	{
		for (int z = 0; z < m_z_max; ++z)
		{
			int y = m_noiseData[x][z];
			int h = y;
			
			while (h > 0)
			{
				if (y - h <= 8)
				{
					++m_dirt_amount;
					++individual_cubes;
				}
				else
				{
					++m_stone_amount;
					++individual_cubes;
				}

				--h;
			}
		}
	}
}