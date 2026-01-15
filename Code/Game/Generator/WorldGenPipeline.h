#pragma once
#include "BiomeGenerator.h"
#include "CaveGenerator.h"
#include "FeaturePlacer.h"
#include "SurfaceBuilder.h"
#include "TerrainGenerator.h"

class Chunk;

struct ChunkGenData
{
	BiomeGenerator::BiomeType m_biomes[CHUNK_SIZE_X][CHUNK_SIZE_Y];
	int m_surfaceHeights[CHUNK_SIZE_X][CHUNK_SIZE_Y];
	BiomeGenerator::BiomeParameters m_biomeParams[CHUNK_SIZE_X][CHUNK_SIZE_Y];
};

class WorldGenPipeline
{
public:
	enum PipelineStage
	{
		STAGE_BIOMES,  
		STAGE_NOISE,   
		STAGE_SURFACE, 
		STAGE_FEATURES,
		STAGE_CAVES,   
		STAGE_CARVERS  
	};

	enum class WorldGenMode
	{
		NORMAL,     
		FLAT_FARM   
	};

public:
    WorldGenPipeline();
    void GenerateChunk(Chunk* chunk);

	void SetWorldGenMode(WorldGenMode mode) { m_genMode = mode; }
	WorldGenMode GetWorldGenMode() const { return m_genMode; }

	void SetFarmWorldCenter(const IntVec2& centerChunk) { m_farmWorldCenter = centerChunk; }
	void SetFarmWorldSize(int chunkRadius) { m_farmWorldRadius = chunkRadius; } // 默认6，生成12x12
	void SetFarmWorldHeight(int height) { m_farmWorldHeight = height; } // 农田所在高度，默认64
    
private:
    void ExecuteBiomeStage(Chunk* chunk, ChunkGenData* chunkGenData);
    void ExecuteNoiseStage(Chunk* chunk, ChunkGenData* chunkGenData);
    void ExecuteSurfaceStage(Chunk* chunk, ChunkGenData* chunkGenData);
    void ExecuteFeatureStage(Chunk* chunk, ChunkGenData* chunkGenData);
    void ExecuteWaterStage(Chunk* chunk, ChunkGenData* chunkGenData);
    void ExecuteCaveStage(Chunk* chunk, ChunkGenData* chunkGenData);
    void ExecuteCarverStage(Chunk* chunk, ChunkGenData* chunkGenData);

	void GenerateFlatFarmChunk(Chunk* chunk);
	bool IsInFarmWorldArea(const IntVec2& chunkCoords) const;

    BiomeGenerator m_biomeGen;
    TerrainGenerator m_terrainGen;
    CaveGenerator m_caveGen;
    SurfaceBuilder m_surfaceBuilder;
    FeaturePlacer m_featurePlacer;

	WorldGenMode m_genMode = WorldGenMode::NORMAL;
	IntVec2 m_farmWorldCenter = IntVec2(0, 0);  // 农场世界中心chunk坐标
	int m_farmWorldRadius = 6;                   // 半径（生成12x12的区域）
	int m_farmWorldHeight = 64;
};
