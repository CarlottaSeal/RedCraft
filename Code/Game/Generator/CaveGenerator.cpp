#include "CaveGenerator.h"
#include "Game/Game.hpp"
#include "WorldGenPipeline.h"
#include "Game/Block.h"
#include "Game/ChunkUtils.h"
#include "ThirdParty/Noise/SmoothNoise.hpp"

extern Game* g_theGame;

CaveGenerator::CaveGenerator(unsigned int seed)
    : m_cheeseSeed(seed)
    , m_spaghettiSeed(seed + 1000)
    , m_noodleSeed(seed + 2000)
    , m_densitySeed(seed + 3000)
{
}

bool CaveGenerator::IsInCave(const Vec3& worldPos, int distanceToSurface, float terrainHeight, 
                             float* outCaveness, CaveType* outDominantType)
{
    UNUSED(outDominantType);
    UNUSED(terrainHeight)
    //float seaLevel = (float)g_theGame->g_seaLevel;
    
    // Basic bounds checking
    if (worldPos.z < 5 || worldPos.z > 200) return false;
    if (distanceToSurface < 8) return false;
    
    // Calculate depth factor for cave frequency
    float depthFactor = 1.0f;
    if (worldPos.z > 60) {
        // Caves become rarer near surface
        depthFactor = RangeMap(worldPos.z, 60.0f, 80.0f, 1.0f, 0.1f);
    }
    
    // === CHEESE CAVES (Large Rooms) ===
    // Use 3D noise to create large spherical voids
    float cheeseNoise = Compute3dPerlinNoise(
        worldPos.x * 0.014f,  // Scale factor for size
        worldPos.y * 0.014f, 
        worldPos.z * 0.014f,
        1.0f,  // Base frequency
        2,     // Fewer octaves for smoother shapes
        0.5f,  // Persistence
        2.0f,  // Lacunarity
        false, // No ridge
        m_cheeseSeed
    );
    
    // Add a second layer for variation
    float cheeseNoise2 = Compute3dPerlinNoise(
        worldPos.x * 0.01f,
        worldPos.y * 0.01f,
        worldPos.z * 0.008f,  // Stretch vertically
        1.0f, 1, 0.5f, 2.0f, false, 
        m_cheeseSeed + 1337
    );
    
    // Combine cheese noises
    float cheeseDensity = (cheeseNoise * 0.7f + cheeseNoise2 * 0.3f) + 0.05f;
    
    // === SPAGHETTI CAVES (Tunnels) ===
    // Minecraft uses 2D noise sampled at different angles to create tunnels
    float spaghetti1 = Compute3dPerlinNoise(
        worldPos.x * 0.025f,
        worldPos.y * 0.025f,
        0.0f,  // 2D noise in XY plane
        1.0f, 3, 0.4f, 2.0f, false,
        m_spaghettiSeed
    );
    
    float spaghetti2 = Compute3dPerlinNoise(
        worldPos.x * 0.025f,
        0.0f,
        worldPos.z * 0.025f,  // 2D noise in XZ plane
        1.0f, 3, 0.4f, 2.0f, false,
        m_spaghettiSeed + 100
    );
    
    // Create tunnel by finding where BOTH noise values are near zero
    float spaghettiDensity = fabsf(spaghetti1) + fabsf(spaghetti2);
    
    // === NOODLE CAVES (Thin tunnels) ===
    float noodle1 = Compute3dPerlinNoise(
        worldPos.x * 0.035f,
        worldPos.y * 0.035f,
        worldPos.z * 0.035f,
        1.0f, 2, 0.6f, 2.0f, true,  // Ridge noise for more chaotic shapes
        m_noodleSeed
    );
    
    float noodleDensity = fabsf(noodle1);
    
    // === COMBINE ALL CAVE TYPES ===
    bool inCave = false;
    float caveness = 0.0f;
    
    // Cheese caves - large threshold for big rooms
    if (cheeseDensity > 0.65f)
        {
        inCave = true;
        caveness = MaxF(caveness, (cheeseDensity - 0.65f) * 3.0f);
    }
    
    // Spaghetti caves - where both noises are near 0
    if (spaghettiDensity < 0.18f)
        {
        inCave = true;
        caveness = MaxF(caveness, (0.18f - spaghettiDensity) * 5.0f);
    }
    
    // Noodle caves - thin threshold
    if (noodleDensity < 0.08f && worldPos.z < 50) {  // Noodles mainly at depth
        inCave = true;
        caveness = MaxF(caveness, (0.08f - noodleDensity) * 8.0f);
    }
    
    // Apply depth factor
    caveness *= depthFactor;
    
    // Add aquifer caves (flooded caves at certain depths)
    if (worldPos.z < 30 && inCave) {
        // Some caves at depth should be flooded
        float floodNoise = Compute3dPerlinNoise(
            worldPos.x * 0.05f,
            worldPos.y * 0.05f,
            worldPos.z * 0.1f,
            1.0f, 1, 0.5f, 2.0f, false,
            m_densitySeed
        );
        
        if (floodNoise > 0.3f) {
            // This cave section will be water-filled
            // (Handle this in CarveCaves method)
        }
    }
    
    if (outCaveness)
        *outCaveness = caveness;
    
    return inCave;
}
void CaveGenerator::CarveCaves(Block* blocks, const IntVec2& chunkCoords, const ChunkGenData& chunkGenData)
{
    // int caveCount = 0;
    // int airCaveCount = 0;
    // int waterCaveCount = 0;
    // int lavaCaveCount = 0;
    //
    // int cheeseCount = 0;
    // int spaghettiCount = 0;
    // int noodleCount = 0;
    //
    float seaLevel = (float)g_theGame->g_seaLevel;
    
    // Process each block in the chunk
    for (int z = 2; z < CHUNK_SIZE_Z - 1; z++)
    {
        for (int y = 0; y < CHUNK_SIZE_Y; y++)
        {
            for (int x = 0; x < CHUNK_SIZE_X; x++)
            {
                int idx = LocalCoordsToIndex(x, y, z);
                
                // Only carve through solid blocks
                if (blocks[idx].m_typeIndex != BLOCK_TYPE_STONE &&
                    blocks[idx].m_typeIndex != BLOCK_TYPE_DIRT &&
                    blocks[idx].m_typeIndex != BLOCK_TYPE_SAND )
                {
                    continue;
                }
                
                // Calculate world coordinates
                float worldX = (float)(chunkCoords.x * CHUNK_SIZE_X + x);
                float worldY = (float)(chunkCoords.y * CHUNK_SIZE_Y + y);
                float worldZ = (float)z;
                Vec3 worldPos(worldX, worldY, worldZ);
                
                // Get terrain height from chunk gen data
                float terrainHeight = (float)chunkGenData.m_surfaceHeights[x][y];
                
                // Calculate distance to surface
                int distanceToSurface = CalculateDistanceToSurface(blocks, x, y, z);
                
                // Check if this position should be a cave
                float caveness = 0.0f;
                if (!IsInCave(worldPos, distanceToSurface, terrainHeight, &caveness))
                {
                    continue;
                }
                
                //caveCount++;
                
                // Determine what to fill the cave with based on depth and conditions
                uint8_t fillBlock = DetermineCaveFill(worldPos, terrainHeight, seaLevel, caveness);
                
                blocks[idx].SetType(fillBlock);
                
                // // Statistics tracking
                // switch (fillBlock)
                // {
                //     case BLOCK_TYPE_AIR:
                //         airCaveCount++;
                //         break;
                //     case BLOCK_TYPE_WATER:
                //         waterCaveCount++;
                //         break;
                //     case BLOCK_TYPE_LAVA:
                //         lavaCaveCount++;
                //         break;
                // }
                
                // Track cave type statistics
                // if (caveness > 0.7f)
                //     cheeseCount++;
                // else if (caveness > 0.4f)
                //     spaghettiCount++;
                // else
                //     noodleCount++;
            }
        }
    }
    
    // Post-process for better water flow and lava pools TODO
    PostProcessLiquids(blocks, chunkCoords);
    
    // // Debug output
    // if (caveCount > 0)
    // {
    //     DebuggerPrintf("Chunk(%d,%d): %d caves | Air:%d Water:%d Lava:%d | Cheese:%d Spaghetti:%d Noodle:%d\n",
    //                    chunkCoords.x, chunkCoords.y,
    //                    caveCount, airCaveCount, waterCaveCount, lavaCaveCount,
    //                    cheeseCount, spaghettiCount, noodleCount);
    // }
}

uint8_t CaveGenerator::DetermineCaveFill(const Vec3& worldPos, float terrainHeight, float seaLevel, float caveness)
{
    // Lava layer (very deep)
    if (worldPos.z < 15)
    {
        // All caves at lava depth are filled with lava
        return BLOCK_TYPE_LAVA;
    }
    
    // Deep aquifer layer (11-30)
    if (worldPos.z < 30)
    {
        // Use noise to determine water vs air pockets
        float aquiferNoise = Compute3dPerlinNoise(
            worldPos.x * 0.02f,
            worldPos.y * 0.02f,
            worldPos.z * 0.05f,
            1.0f, 2, 0.5f, 2.0f, false,
            m_densitySeed + 5000
        );
        
        // Large caves more likely to have air pockets
        float airChance = caveness * 0.5f;
        
        if (aquiferNoise + airChance > 0.3f)
        {
            return BLOCK_TYPE_AIR;
        }
        else
        {
            return BLOCK_TYPE_WATER;
        }
    }
    
    // Mid-level caves (30-50)
    if (worldPos.z < 50)
    {
        // Occasional water pools in large caves
        if (caveness > 0.8f)
        {
            float poolNoise = Compute3dPerlinNoise(
                worldPos.x * 0.04f,
                worldPos.y * 0.04f,
                worldPos.z * 0.1f,
                1.0f, 1, 0.5f, 2.0f, false,
                m_densitySeed + 7000
            );
            
            if (poolNoise > 0.7f)
            {
                return BLOCK_TYPE_WATER;
            }
        }
        return BLOCK_TYPE_AIR;
    }
    
    // Check if below sea level in ocean areas
    if (worldPos.z < seaLevel && terrainHeight < seaLevel - 5.0f)
    {
        // Underwater caves in ocean biomes
        float drainageNoise = Compute3dPerlinNoise(
            worldPos.x * 0.03f,
            worldPos.y * 0.03f,
            worldPos.z * 0.06f,
            1.0f, 2, 0.5f, 2.0f, false,
            m_densitySeed + 8000
        );
        
        // Some underwater caves can have air pockets
        if (drainageNoise > 0.6f && caveness > 0.6f)
        {
            return BLOCK_TYPE_AIR;
        }
        return BLOCK_TYPE_WATER;
    }
    
    // Normal caves above water table
    return BLOCK_TYPE_AIR;
}

void CaveGenerator::PostProcessLiquids(Block* blocks, const IntVec2& chunkCoords)
{
    UNUSED(chunkCoords)
    // Create more realistic liquid pools by ensuring liquids flow downward
    for (int z = CHUNK_SIZE_Z - 2; z >= 2; z--)
    {
        for (int y = 0; y < CHUNK_SIZE_Y; y++)
        {
            for (int x = 0; x < CHUNK_SIZE_X; x++)
            {
                int idx = LocalCoordsToIndex(x, y, z);
                int idxBelow = LocalCoordsToIndex(x, y, z - 1);

                // 只要下面是空气，就往下掉一格
                if ((blocks[idx].m_typeIndex == BLOCK_TYPE_WATER || 
                     blocks[idx].m_typeIndex == BLOCK_TYPE_LAVA) &&
                    blocks[idxBelow].m_typeIndex == BLOCK_TYPE_AIR)
                {
                    blocks[idxBelow].SetType(blocks[idx].m_typeIndex);
                    blocks[idx].SetType(BLOCK_TYPE_AIR);
                }

                if (blocks[idx].m_typeIndex == BLOCK_TYPE_LAVA)
                {
                    bool touchesWater = false;
                    for (int dz = -1; dz <= 1; dz++)
                    {
                        for (int dy = -1; dy <= 1; dy++)
                        {
                            for (int dx = -1; dx <= 1; dx++)
                            {
                                if (dx == 0 && dy == 0 && dz == 0) continue;

                                int nx = x + dx;
                                int ny = y + dy;
                                int nz = z + dz;

                                if (nx >= 0 && nx < CHUNK_SIZE_X && 
                                    ny >= 0 && ny < CHUNK_SIZE_Y &&
                                    nz >= 2 && nz < CHUNK_SIZE_Z - 1)
                                {
                                    int adjIdx = LocalCoordsToIndex(nx, ny, nz);
                                    if (blocks[adjIdx].m_typeIndex == BLOCK_TYPE_WATER)
                                    {
                                        touchesWater = true;
                                        break;
                                    }
                                }
                            }
                            if (touchesWater) break;
                        }
                        if (touchesWater) break;
                    }

                    if (touchesWater)
                    {
                        blocks[idx].SetType(BLOCK_TYPE_STONE);
                    }
                }
            }
        }
    }
}

int CaveGenerator::CalculateDistanceToSurface(Block* blocks, int x, int y, int z)
{
    int distance = 0;
    
    for (int checkZ = z + 1; checkZ < CHUNK_SIZE_Z; checkZ++)
    {
        int idx = LocalCoordsToIndex(x, y, checkZ);
        uint8_t blockType = blocks[idx].m_typeIndex;
        
        if (blockType == BLOCK_TYPE_AIR || blockType == BLOCK_TYPE_WATER)
        {
            return distance;
        }
        
        distance++;
    }
    
    return distance;
}
