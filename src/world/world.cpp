#include "world.h"

#include <iostream>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <FastNoiseLite.h>

#include "blocks/blocks.h"

enum BlocksTypeTextures
{
	BLOCK_TEXTURE_DIRT = 0,
	BLOCK_TEXTURE_GRASS_SIDE = 1,
	BLOCK_TEXTURE_GRASS_TOP = 2,
	BLOCK_TEXTURE_BEDROCK = 3,
	BLOCK_TEXTURE_STONE = 4
};

enum BlocksTypeShaders
{
	BLOCK_SHADER_DIRT = 0,
	BLOCK_SHADER_GRASS = 1,
	BLOCK_SHADER_BEDROCK = 2,
	BLOCK_SHADER_STONE = 3
};

World::World(const int& seed, const int& x_max, const int& z_max, const int& y_max)
	: seed(seed), x_max(x_max), z_max(z_max), y_max(y_max)
{
	load_textures();
	load_shaders();
	load_VBOs_VAOs();

	std::vector<std::vector<float>> noiseData(x_max, std::vector<float>(z_max));
	m_noiseData = noiseData;
	load_noise();
}

World::~World()
{
	for (int i = 0; i < 5; i++)
	{
		glDeleteTextures(1, &m_textures[i]);
	}
}

void World::render_world(Camera& camera, const glm::mat4& projection)
{
	m_shaders[BLOCK_SHADER_DIRT].use();
	m_shaders[BLOCK_SHADER_DIRT].setVec3("viewPos", camera.Position);
	m_shaders[BLOCK_SHADER_DIRT].setMat4("projection", projection);
	m_shaders[BLOCK_SHADER_DIRT].setMat4("view", camera.GetViewMatrix());
	glBindVertexArray(dirt_VAO);

	for (int x = 0; x < x_max; x++)
	{
		for (int z = 0; z < z_max; z++)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(x, m_noiseData[x][z], z));
			m_shaders[BLOCK_SHADER_DIRT].setMat4("model", model);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
	}
}

unsigned int World::load_texture(const std::string& path)
{
	unsigned int m_TextureID = 0;
	unsigned char* m_LocalBuffer = NULL;
	int m_Width = 0, m_Height = 0, m_BPP = 0;

	stbi_set_flip_vertically_on_load(1);
	m_LocalBuffer = stbi_load(path.c_str(), &m_Width, &m_Height, &m_BPP, 0);
	
	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);

	// anisotropic filtering
	float maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	
	if(m_LocalBuffer)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LocalBuffer);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}

	if (m_LocalBuffer)
		stbi_image_free(m_LocalBuffer);

	return m_TextureID;
}

void World::load_textures()
{
	m_textures[BLOCK_TEXTURE_DIRT] = load_texture("assets/models/dirt/dirt.png");
	m_textures[BLOCK_TEXTURE_GRASS_SIDE] = load_texture("assets/models/grass/grass_side.png");
	m_textures[BLOCK_TEXTURE_GRASS_TOP] = load_texture("assets/models/grass/grass_top.png");
	m_textures[BLOCK_TEXTURE_BEDROCK] = load_texture("assets/models/bedrock/bedrock.png");
	m_textures[BLOCK_TEXTURE_STONE] = load_texture("assets/models/stone/stone.png");
	glBindTextureUnit(BLOCK_TEXTURE_DIRT, m_textures[BLOCK_TEXTURE_DIRT]);
	glBindTextureUnit(BLOCK_TEXTURE_GRASS_SIDE, m_textures[BLOCK_TEXTURE_GRASS_SIDE]);
	glBindTextureUnit(BLOCK_TEXTURE_GRASS_TOP, m_textures[BLOCK_TEXTURE_GRASS_TOP]);
	glBindTextureUnit(BLOCK_TEXTURE_BEDROCK, m_textures[BLOCK_TEXTURE_BEDROCK]);
	glBindTextureUnit(BLOCK_TEXTURE_STONE, m_textures[BLOCK_TEXTURE_STONE]);
}

void World::load_shaders()
{
	Shader dirt_shader("assets/shaders/dirt_vert.glsl", "assets/shaders/dirt_frag.glsl"),
		   grass_shader("assets/shaders/grass_vert.glsl", "assets/shaders/grass_frag.glsl"),
		   bedrock_shader("assets/shaders/bedrock_vert.glsl", "assets/shaders/bedrock_frag.glsl"),
		   stone_shader("assets/shaders/stone_vert.glsl", "assets/shaders/stone_frag.glsl");
	m_shaders[BLOCK_SHADER_DIRT] = dirt_shader;
	m_shaders[BLOCK_SHADER_GRASS] = grass_shader;
	//memcpy(&m_shaders[BLOCK_SHADER_GRASS], &grass_shader, sizeof(grass_shader));
	m_shaders[BLOCK_SHADER_BEDROCK] = bedrock_shader;
	m_shaders[BLOCK_SHADER_STONE] = stone_shader;

	// BLOCK_TEXTURE_DIRT, BLOCK_TEXTURE_GRASS_SIDE, BLOCK_TEXTURE_GRASS_TOP, BLOCK_TEXTURE_BEDROCK, BLOCK_TEXTURE_STONE
	int samplers[5] = { 0, 1, 2, 3, 4 };

	m_shaders[BLOCK_SHADER_DIRT].use();
	auto loc = glGetUniformLocation(m_shaders[BLOCK_SHADER_DIRT].ID, "diffuseMap");
	glUniform1iv(loc, 5, samplers);
	m_shaders[BLOCK_SHADER_DIRT].setVec3("light.direction", -0.2f, -1.0f, -0.3f);
	m_shaders[BLOCK_SHADER_DIRT].setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
	m_shaders[BLOCK_SHADER_DIRT].setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);

	m_shaders[BLOCK_SHADER_GRASS].use();
	loc = glGetUniformLocation(m_shaders[BLOCK_SHADER_GRASS].ID, "diffuseMap");
	glUniform1iv(loc, 5, samplers);
	m_shaders[BLOCK_SHADER_GRASS].setVec3("light.direction", -0.2f, -1.0f, -0.3f);
	m_shaders[BLOCK_SHADER_GRASS].setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
	m_shaders[BLOCK_SHADER_GRASS].setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);

	m_shaders[BLOCK_SHADER_BEDROCK].use();
	loc = glGetUniformLocation(m_shaders[BLOCK_SHADER_BEDROCK].ID, "diffuseMap");
	glUniform1iv(loc, 5, samplers);
	m_shaders[BLOCK_SHADER_BEDROCK].setVec3("light.direction", -0.2f, -1.0f, -0.3f);
	m_shaders[BLOCK_SHADER_BEDROCK].setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
	m_shaders[BLOCK_SHADER_BEDROCK].setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
	
	m_shaders[BLOCK_SHADER_STONE].use();
	loc = glGetUniformLocation(m_shaders[BLOCK_SHADER_STONE].ID, "diffuseMap");
	glUniform1iv(loc, 5, samplers);
	m_shaders[BLOCK_SHADER_STONE].setVec3("light.direction", -0.2f, -1.0f, -0.3f);
	m_shaders[BLOCK_SHADER_STONE].setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
	m_shaders[BLOCK_SHADER_STONE].setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
}

void World::load_VBOs_VAOs()
{
	// dirt
	glGenVertexArrays(1, &dirt_VAO);
	glGenBuffers(1, &dirt_VBO);
	glBindVertexArray(dirt_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, dirt_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(dirt_vertices), dirt_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);
	
	// grass
	glGenVertexArrays(1, &grass_VAO);
	glGenBuffers(1, &grass_VBO);
	glBindVertexArray(grass_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, grass_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(grass_vertices), grass_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);
	
	// bedrock
	glGenVertexArrays(1, &bedrock_VAO);
	glGenBuffers(1, &bedrock_VBO);
	glBindVertexArray(bedrock_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, bedrock_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bedrock_vertices), bedrock_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);
	
	// stone
	glGenVertexArrays(1, &stone_VAO);
	glGenBuffers(1, &stone_VBO);
	glBindVertexArray(stone_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, stone_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(stone_vertices), stone_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);
}


float World::map_value(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void World::load_noise()
{
	FastNoiseLite noise;
	
	noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	noise.SetFrequency(0.01f);
	noise.SetSeed(seed);
	noise.SetFractalOctaves(5);
	noise.SetFractalLacunarity(2.0f);
	noise.SetFractalGain(0.5f);
	
	for (int x = 0; x < x_max; x++)
	{
		for (int z = 0; z < z_max; z++)
		{
			m_noiseData[x][z] = noise.GetNoise((float)x, (float)z);
			m_noiseData[x][z] = round(map_value(m_noiseData[x][z], -1.0f, 1.0f, 0.0f, y_max));
		}
	}
}