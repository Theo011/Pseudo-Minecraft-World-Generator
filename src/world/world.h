#pragma once

#include <vector>

#include "../engine/camera.h"
#include "../engine/model.h"

enum Blocks
{
	DIRT,
	STONE,
	BEDROCK,
	GRASS,
	BLOCKS_AMOUNT // HAS TO ALWAYS BE LAST
};

class World
{
private:
	int m_seed;
	int m_x_max;
	int m_z_max;
	int m_y_max;
	std::vector<std::vector<float>> m_noiseData;
	Shader m_general_block_shader;
	Model m_block_models[BLOCKS_AMOUNT];
	int m_dirt_amount, m_stone_amount, m_bedrock_amount, m_grass_amount;
	glm::mat4 *m_dirt_matrices, *m_stone_matrices, *m_bedrock_matrices, *m_grass_matrices;
	unsigned int m_dirt_buffer, m_stone_buffer, m_bedrock_buffer, m_grass_buffer;
		
public:
	// x = width, z = depth, y = height
	World(const int& seed, const int& x_max, const int& z_max, const int& y_max);
	~World();

	void render_world(Camera& camera, const glm::mat4& projection);
	
private:
	float map_value(const float& x, const float& in_min, const float& in_max, const float& out_min, const float& out_max);
	void load_noise();
	void load_models();
	void setup_world();
};