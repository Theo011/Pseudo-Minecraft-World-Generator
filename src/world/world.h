#pragma once

#include <vector>

#include "../engine/camera.h"
#include "../engine/shader.h"

class World
{
public:
	// x = width, z = depth, y = height
	World(const int& seed, const int& x_max, const int& z_max, const int& y_max);
	~World();

	void render_world(Camera& camera, const glm::mat4& projection);
	
private:
	int seed;
	int x_max;
	int z_max;
	int y_max;
	unsigned int m_textures[5];
	Shader m_shaders[5];
	std::vector<std::vector<float>> m_noiseData;
	unsigned int dirt_VBO, dirt_VAO;
	unsigned int grass_VBO, grass_VAO;
	unsigned int bedrock_VBO, bedrock_VAO;
	unsigned int stone_VBO, stone_VAO;

private:
	unsigned int load_texture(const std::string& path);
	void load_textures();
	void load_shaders();
	void load_VBOs_VAOs();
	float map_value(float x, float in_min, float in_max, float out_min, float out_max);
	void load_noise();
};