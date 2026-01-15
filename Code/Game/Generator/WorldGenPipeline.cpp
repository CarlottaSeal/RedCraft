#include "WorldGenPipeline.h"

#include "Game/Chunk.h"
#include "Game/ChunkUtils.h"
#include "Game/Game.hpp"

extern Game* g_theGame;

WorldGenPipeline::WorldGenPipeline()
    : m_biomeGen(BiomeGenerator(GAME_SEED))
    , m_terrainGen(TerrainGenerator(GAME_SEED))
    , m_caveGen(CaveGenerator(GAME_SEED))
    , m_featurePlacer(FeaturePlacer(GAME_SEED))
    , m_surfaceBuilder(SurfaceBuilder())
{
}

void WorldGenPipeline::GenerateChunk(Chunk* chunk)
{
    if (!chunk)
        return;
    if (m_genMode == WorldGenMode::FLAT_FARM)
    {
        GenerateFlatFarmChunk(chunk);
        return;
    }
    
    ChunkGenData chunkGenData = ChunkGenData();
    ExecuteBiomeStage(chunk, &chunkGenData);
    ExecuteNoiseStage(chunk, &chunkGenData);
    if (g_theGame->g_caveCarvingEnabled)
    {
        ExecuteCaveStage(chunk, &chunkGenData);
    }
    if (g_theGame->g_seaEnabled)
    {
        ExecuteWaterStage(chunk, &chunkGenData);
    }
    if (g_theGame->g_blockReplacementEnabled)
    {
        ExecuteSurfaceStage(chunk, &chunkGenData);
    }
    if (g_theGame->g_treeGenerationEnabled)
    {
        ExecuteFeatureStage(chunk, &chunkGenData);
    }
    //ExecuteCarverStage(chunk, &chunkGenData);

    chunk->m_chunkGenData = chunkGenData;
    
    //delete chunkGenData;
    //chunkGenData = nullptr;
}

void WorldGenPipeline::ExecuteBiomeStage(Chunk* chunk, ChunkGenData* chunkGenData)
{
    IntVec2 chunkCoords = chunk->GetThisChunkCoords();
    
    for (int y = 0; y < CHUNK_SIZE_Y; y++)
    {
        for (int x = 0; x < CHUNK_SIZE_X; x++)
        {
            int worldX = chunkCoords.x * CHUNK_SIZE_X + x;
            int worldY = chunkCoords.y * CHUNK_SIZE_Y + y;
            
            chunkGenData->m_biomeParams[x][y] = m_biomeGen.SampleBiomeParameters(worldX, worldY);
            chunkGenData->m_biomes[x][y] = m_biomeGen.DetermineBiome(chunkGenData->m_biomeParams[x][y]);
            //chunkGenData->m_surfaceHeights[x][y] = 
             //   m_terrainGen.GetSurfaceHeight(worldX, worldY, chunkGenData->m_biomeParams[x][y]);
            chunkGenData->m_surfaceHeights[x][y] = 0;
        }
    }
}

void WorldGenPipeline::ExecuteNoiseStage(Chunk* chunk, ChunkGenData* chunkGenData)
{
    IntVec2 chunkCoords = chunk->GetThisChunkCoords();
    
    for (int z = 0; z < CHUNK_SIZE_Z; z++)
    {
        for (int y = 0; y < CHUNK_SIZE_Y; y++)
        {
            for (int x = 0; x < CHUNK_SIZE_X; x++)
            {
                int idx = LocalCoordsToIndex(x, y , z);
                
                if (z <= 1)
                {
                    //chunk->m_blocks[idx].SetType(BLOCK_TYPE_OBSIDIAN);
                    chunk->m_blocks[idx].SetType(BLOCK_TYPE_OBSIDIAN);
                    continue;
                }
                
                float worldX = (float)(chunkCoords.x * CHUNK_SIZE_X + x);
                float worldY = (float)(chunkCoords.y * CHUNK_SIZE_Y + y);
                float worldZ = (float)z;
                
                Vec3 worldPos(worldX, worldY, worldZ);
                
                BiomeGenerator::BiomeParameters biomeParams = chunkGenData->m_biomeParams[x][y];
                float density = m_terrainGen.Calculate3DDensity(worldPos, biomeParams);
                
                if (density < 0.0f)
                {
                    chunk->m_blocks[idx].SetType(BLOCK_TYPE_STONE);
                    chunkGenData->m_surfaceHeights[x][y] = z;
                }
                else
                {
                    chunk->m_blocks[idx].SetType(BLOCK_TYPE_AIR);
                }
            }
        }
    }
}

void WorldGenPipeline::ExecuteSurfaceStage(Chunk* chunk, ChunkGenData* chunkGenData)
{
    for (int y = 0; y < CHUNK_SIZE_Y; y++)
    {
        for (int x = 0; x < CHUNK_SIZE_X; x++)
        {
            BiomeGenerator::BiomeType biome = chunkGenData->m_biomes[x][y];
            float temperature = chunkGenData->m_biomeParams[x][y].m_temperature;
            float humidity = chunkGenData->m_biomeParams[x][y].m_humidity;
            
            SurfaceBuilder::SurfaceConfig config = 
                m_surfaceBuilder.GetSurfaceConfig(biome, temperature, humidity);
            
            int surfaceZ = -1;
            for (int z = CHUNK_SIZE_Z - 2; z >= 2; --z)
            {
                int idx = LocalCoordsToIndex(x, y, z);
                int idxUp = LocalCoordsToIndex(x, y, z + 1);
                uint8_t t = chunk->m_blocks[idx].m_typeIndex;
                uint8_t tUp = chunk->m_blocks[idxUp].m_typeIndex;

                if (IsSolid(t) && IsNonGroundCover(tUp))
                {
                    surfaceZ = z;
                    break;
                }
            }

            if (surfaceZ >= 2)
            {
                m_surfaceBuilder.BuildSurface(chunk->m_blocks, x, y, surfaceZ, config);
            }
        }
    }
    
    // 2. 温度覆盖（冰）
    float avgTemperature = 0.0f;
    for (int y = 0; y < CHUNK_SIZE_Y; y++)
    {
        for (int x = 0; x < CHUNK_SIZE_X; x++)
        {
            avgTemperature += chunkGenData->m_biomeParams[x][y].m_temperature;
        }
    }
    avgTemperature /= (CHUNK_SIZE_X * CHUNK_SIZE_Y);
    
    m_surfaceBuilder.ApplyTemperatureOverrides(chunk->m_blocks, avgTemperature);
    
    m_surfaceBuilder.GenerateOres(chunk->m_blocks, chunk->GetThisChunkCoords());
}

void WorldGenPipeline::ExecuteCaveStage(Chunk* chunk, ChunkGenData* chunkGenData)
{
    m_caveGen.CarveCaves(chunk->m_blocks, chunk->GetThisChunkCoords(), *chunkGenData);
}

void WorldGenPipeline::ExecuteWaterStage(Chunk* chunk, ChunkGenData* chunkGenData)
{
    UNUSED(chunkGenData)
    for (int y = 0; y < CHUNK_SIZE_Y; y++)
    {
        for (int x = 0; x < CHUNK_SIZE_X; x++)
        {
            int firstSolidZ = -1;
            for (int z = CHUNK_SIZE_Z - 1; z >= 2; --z)
            {
                int idx = LocalCoordsToIndex(x, y, z);
                uint8_t blockType = chunk->m_blocks[idx].m_typeIndex;
                
                if (blockType != BLOCK_TYPE_AIR)
                {
                    firstSolidZ = z;
                    break;
                }
            }
            
            if (firstSolidZ >= 0 && firstSolidZ < g_theGame->g_seaLevel)
            {
                // 从地表+1到海平面-1，填充连续的AIR为水
                for (int z = firstSolidZ + 1; z < g_theGame->g_seaLevel; z++)
                {
                    int idx = LocalCoordsToIndex(x, y, z);
                    
                    // 只填充AIR，不填充已有的其他方块
                    if (chunk->m_blocks[idx].m_typeIndex == BLOCK_TYPE_AIR)
                    {
                        chunk->m_blocks[idx].SetType(BLOCK_TYPE_WATER);
                    }
                    else
                    {
                        // 遇到非AIR（比如树干、洞穴石头），停止向上填水
                        break;
                    }
                }
            }
            // 如果整个柱子都是空气（深海），填满到海平面 
            if (firstSolidZ < 0)
            {
                for (int z = 2; z < g_theGame->g_seaLevel; z++)
                {
                    int idx = LocalCoordsToIndex(x, y, z);
                    if (chunk->m_blocks[idx].m_typeIndex == BLOCK_TYPE_AIR)
                    {
                        chunk->m_blocks[idx].SetType(BLOCK_TYPE_WATER);
                    }
                }
            }
        }
    }
}

void WorldGenPipeline::ExecuteFeatureStage(Chunk* chunk, ChunkGenData* chunkGenData)
{
    IntVec2 chunkCoords = chunk->GetThisChunkCoords();
    
    for (int y = 0; y < CHUNK_SIZE_Y; y++)
    {
        for (int x = 0; x < CHUNK_SIZE_X; x++)
        {
            int worldX = chunkCoords.x * CHUNK_SIZE_X + x;
            int worldY = chunkCoords.y * CHUNK_SIZE_Y + y;
            
            BiomeGenerator::BiomeType biome = chunkGenData->m_biomes[x][y];
            
            if (m_featurePlacer.ShouldPlaceTree(worldX, worldY, biome))
            {
                int surfaceZ = -1;
                for (int z = CHUNK_SIZE_Z - 1; z >= 2; --z)
                {
                    int idx = LocalCoordsToIndex(x, y, z);
                    uint8_t t = chunk->m_blocks[idx].m_typeIndex;

                    if (t == BLOCK_TYPE_GRASS || t == BLOCK_TYPE_DIRT ||
                        t == BLOCK_TYPE_SAND || t == BLOCK_TYPE_SNOW)
                    {
                        surfaceZ = z;
                        break;
                    }
                }
                
                if (surfaceZ >= 2 && surfaceZ + 10 < CHUNK_SIZE_Z)
                {
                    bool hasSpace = true;
                    for (int z = surfaceZ + 1; z <= surfaceZ + 10; ++z)
                    {
                        int id = LocalCoordsToIndex(x, y, z);
                        if (chunk->m_blocks[id].m_typeIndex != BLOCK_TYPE_AIR)
                        {
                            hasSpace = false;
                            break;
                        }
                    }
                    if (hasSpace)
                    {
                        m_featurePlacer.PlaceTree(chunk, x, y, surfaceZ, biome);
                    }
                }
            }
            if (biome == BiomeGenerator::BIOME_OCEAN || 
    biome == BiomeGenerator::BIOME_DEEP_OCEAN || 
    biome == BiomeGenerator::BIOME_FROZEN_OCEAN)
            {
                if (m_featurePlacer.ShouldPlaceUnderwaterPlant(worldX, worldY, biome))
                {
                    int seabedZ = -1;
                    for (int z = CHUNK_SIZE_Z - 1; z >= 0; --z)
                    {
                        int idx = LocalCoordsToIndex(x, y, z);
                        uint8_t blockType = chunk->m_blocks[idx].m_typeIndex;
                        if (blockType != BLOCK_TYPE_AIR && blockType != BLOCK_TYPE_WATER)
                        {
                            seabedZ = z;
                            break;
                        }
                    }
                    if (seabedZ >= 0 && seabedZ < CHUNK_SIZE_Z - 1)
                    {
                        int idxAbove = LocalCoordsToIndex(x, y, seabedZ + 1);
                        if (chunk->m_blocks[idxAbove].m_typeIndex == BLOCK_TYPE_WATER)
                        {
                            float temperature = chunkGenData->m_biomeParams[x][y].m_temperature;  
                            m_featurePlacer.PlaceUnderwaterPlants(chunk, x, y, seabedZ, biome, temperature);
                        }
                    }
                }
            }
        }
    }
}

void WorldGenPipeline::ExecuteCarverStage(Chunk* chunk, ChunkGenData* chunkGenData)
{
    UNUSED(chunkGenData)
    UNUSED(chunk)
}

void WorldGenPipeline::GenerateFlatFarmChunk(Chunk* chunk)
{
    IntVec2 chunkCoords = chunk->GetThisChunkCoords();
    bool isInFarmArea = IsInFarmWorldArea(chunkCoords);
    int worldX = chunkCoords.x * CHUNK_SIZE_X;
    int worldY = chunkCoords.y * CHUNK_SIZE_Y;
	
    for (int localX = 0; localX < CHUNK_SIZE_X; localX++)
    {
        for (int localY = 0; localY < CHUNK_SIZE_Y; localY++)
        {
            for (int localZ = 0; localZ < CHUNK_SIZE_Z; localZ++)
            {
                int blockIndex = chunk->GetBlockLocalIndexFromLocalCoords(localX, localY, localZ);
                Block& block = chunk->m_blocks[blockIndex];
				
                if (isInFarmArea)
                {
                    if (localZ < m_farmWorldHeight - 1)
                    {
                        block.SetType(BLOCK_TYPE_DIRT);
                    }
                    else if (localZ == m_farmWorldHeight - 1)
                    {
                        block.SetType(BLOCK_TYPE_FARMLAND);
                    }
                    else
                    {
                        block.SetType(BLOCK_TYPE_AIR);
                    }
                }
                else
                {
                    // 农场区域外 - 生成空世界或基岩
                    if (localZ == 0)
                    {
                        block.SetType(BLOCK_TYPE_OBSIDIAN);
                    }
                    else
                    {
                        block.SetType(BLOCK_TYPE_AIR);
                    }
                }
            }
        }
    }
}

bool WorldGenPipeline::IsInFarmWorldArea(const IntVec2& chunkCoords) const
{
    int deltaX = chunkCoords.x - m_farmWorldCenter.x;
    int deltaY = chunkCoords.y - m_farmWorldCenter.y;
    return (abs(deltaX) <= m_farmWorldRadius && abs(deltaY) <= m_farmWorldRadius);
}