#include "FeaturePlacer.h"

#include "Engine/Math/IntVec3.h"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Game/Chunk.h"
#include "Game/Gamecommon.hpp"
#include "Game/ChunkUtils.h"

FeaturePlacer::FeaturePlacer(unsigned int baseSeed)
{
    m_treeSeed = baseSeed + 400;
    m_rng = RandomNumberGenerator();
    
    InitializeTreeStamps();
}

void FeaturePlacer::InitializeTreeStamps()
{
    TreeStamp oakSmall;
    oakSmall.m_logType = BLOCK_TYPE_OAK_LOG;
    oakSmall.m_leafType = BLOCK_TYPE_OAK_LEAVES;
    oakSmall.m_logPositions = MakeColumn(4);
    oakSmall.m_leafPositions = MakeBlobLeaves(IntVec3(0,0,3), 2); 
    m_oakVariants.push_back(oakSmall);
    
    TreeStamp oakMedium;
    oakMedium.m_logType = BLOCK_TYPE_OAK_LOG;
    oakMedium.m_leafType = BLOCK_TYPE_OAK_LEAVES;
    oakMedium.m_logPositions = MakeColumn(5);

    auto leaves1 = MakeBlobLeaves(IntVec3(0,0,3), 2);
    auto leaves2 = MakeBlobLeaves(IntVec3(0,0,4), 2);
    oakMedium.m_leafPositions.insert(oakMedium.m_leafPositions.end(), leaves1.begin(), leaves1.end());
    oakMedium.m_leafPositions.insert(oakMedium.m_leafPositions.end(), leaves2.begin(), leaves2.end());
    oakMedium.m_leafPositions.push_back(IntVec3(0,0,5));  // 顶部
    m_oakVariants.push_back(oakMedium);
    
    TreeStamp oakLarge;
    oakLarge.m_logType = BLOCK_TYPE_OAK_LOG;
    oakLarge.m_leafType = BLOCK_TYPE_OAK_LEAVES;
    oakLarge.m_logPositions = MakeColumn(6);
    auto leavesL1 = MakeBlobLeaves(IntVec3(0,0,3), 2);
    auto leavesL2 = MakeBlobLeaves(IntVec3(0,0,4), 2);
    auto leavesL3 = MakeBlobLeaves(IntVec3(0,0,5), 2);
    oakLarge.m_leafPositions.insert(oakLarge.m_leafPositions.end(), leavesL1.begin(), leavesL1.end());
    oakLarge.m_leafPositions.insert(oakLarge.m_leafPositions.end(), leavesL2.begin(), leavesL2.end());
    oakLarge.m_leafPositions.insert(oakLarge.m_leafPositions.end(), leavesL3.begin(), leavesL3.end());
    oakLarge.m_leafPositions.push_back(IntVec3(0,0,6));
    m_oakVariants.push_back(oakLarge);
    
    TreeStamp birchSmall;
    birchSmall.m_logType = BLOCK_TYPE_BIRCH_LOG;
    birchSmall.m_leafType = BLOCK_TYPE_BIRCH_LEAVES;
    birchSmall.m_logPositions = MakeColumn(5);
    birchSmall.m_leafPositions = MakeBlobLeaves(IntVec3(0,0,3), 2);
    auto birchTop = MakeBlobLeaves(IntVec3(0,0,4), 1);
    birchSmall.m_leafPositions.insert(birchSmall.m_leafPositions.end(), birchTop.begin(), birchTop.end());
    birchSmall.m_leafPositions.push_back(IntVec3(0,0,5));
    m_birchVariants.push_back(birchSmall);
    
    TreeStamp birchTall;
    birchTall.m_logType = BLOCK_TYPE_BIRCH_LOG;
    birchTall.m_leafType = BLOCK_TYPE_BIRCH_LEAVES;
    birchTall.m_logPositions = MakeColumn(6);
    auto birchL1 = MakeBlobLeaves(IntVec3(0,0,4), 2);
    auto birchL2 = MakeBlobLeaves(IntVec3(0,0,5), 1);
    birchTall.m_leafPositions.insert(birchTall.m_leafPositions.end(), birchL1.begin(), birchL1.end());
    birchTall.m_leafPositions.insert(birchTall.m_leafPositions.end(), birchL2.begin(), birchL2.end());
    birchTall.m_leafPositions.push_back(IntVec3(0,0,6));
    m_birchVariants.push_back(birchTall);
    
    TreeStamp spruceNormal;
    spruceNormal.m_logType = BLOCK_TYPE_SPRUCE_LOG;
    spruceNormal.m_leafType = BLOCK_TYPE_SPRUCE_LEAVES;
    spruceNormal.m_logPositions = MakeColumn(6);
    spruceNormal.m_leafPositions = MakeSpruceLeaves(7);  // 锥形
    m_spruceVariants.push_back(spruceNormal);
    
    // 高大Spruce（9-11格高）- 从图片看Spruce非常高
    TreeStamp spruceTall;
    spruceTall.m_logType = BLOCK_TYPE_SPRUCE_LOG;
    spruceTall.m_leafType = BLOCK_TYPE_SPRUCE_LEAVES;
    spruceTall.m_logPositions = MakeColumn(9);
    spruceTall.m_leafPositions = MakeSpruceLeaves(10);
    m_spruceVariants.push_back(spruceTall);
    
    TreeStamp spruceGiant;
    spruceGiant.m_logType = BLOCK_TYPE_SPRUCE_LOG;
    spruceGiant.m_leafType = BLOCK_TYPE_SPRUCE_LEAVES;
    spruceGiant.m_logPositions = MakeColumn(12);
    spruceGiant.m_leafPositions = MakeSpruceLeaves(13);
    m_spruceVariants.push_back(spruceGiant);
    
    for (const auto& spruce : m_spruceVariants)
    {
        TreeStamp snowySpruce = spruce;
        snowySpruce.m_leafType = BLOCK_TYPE_SPRUCE_LEAVES_SNOW;
        m_snowySpruceVariants.push_back(snowySpruce);
    }
    
    TreeStamp jungleSmall;
    jungleSmall.m_logType = BLOCK_TYPE_JUNGLE_LOG;
    jungleSmall.m_leafType = BLOCK_TYPE_JUNGLE_LEAVES;
    jungleSmall.m_logPositions = MakeColumn(6);
    jungleSmall.m_leafPositions = MakeBlobLeaves(IntVec3(0,0,4), 3);
    auto jungleTop = MakeBlobLeaves(IntVec3(0,0,5), 2);
    jungleSmall.m_leafPositions.insert(jungleSmall.m_leafPositions.end(), jungleTop.begin(), jungleTop.end());
    m_jungleVariants.push_back(jungleSmall);
    
    TreeStamp jungleLarge;
    jungleLarge.m_logType = BLOCK_TYPE_JUNGLE_LOG;
    jungleLarge.m_leafType = BLOCK_TYPE_JUNGLE_LEAVES;
    jungleLarge.m_logPositions = MakeColumn(9);
    auto jungL1 = MakeBlobLeaves(IntVec3(0,0,6), 3);
    auto jungL2 = MakeBlobLeaves(IntVec3(0,0,7), 3);
    auto jungL3 = MakeBlobLeaves(IntVec3(0,0,8), 2);
    jungleLarge.m_leafPositions.insert(jungleLarge.m_leafPositions.end(), jungL1.begin(), jungL1.end());
    jungleLarge.m_leafPositions.insert(jungleLarge.m_leafPositions.end(), jungL2.begin(), jungL2.end());
    jungleLarge.m_leafPositions.insert(jungleLarge.m_leafPositions.end(), jungL3.begin(), jungL3.end());
    jungleLarge.m_leafPositions.push_back(IntVec3(0,0,9));
    m_jungleVariants.push_back(jungleLarge);
    
    TreeStamp acacia;
    acacia.m_logType = BLOCK_TYPE_ACACIA_LOG;
    acacia.m_leafType = BLOCK_TYPE_ACACIA_LEAVES;
    acacia.m_logPositions = MakeColumn(5);
    // Acacia特点：扁平、不对称的树冠
    acacia.m_leafPositions = MakeAcaciaLeaves();
    m_acaciaVariants.push_back(acacia);
    
    TreeStamp darkOak;
    darkOak.m_logType = BLOCK_TYPE_OAK_LOG;
    darkOak.m_leafType = BLOCK_TYPE_OAK_LEAVES;
    // 2x2树干
    darkOak.m_logPositions = MakeDarkOakTrunk(7);
    auto darkL1 = MakeBlobLeaves(IntVec3(0,0,5), 3);
    auto darkL2 = MakeBlobLeaves(IntVec3(0,0,6), 3);
    darkOak.m_leafPositions.insert(darkOak.m_leafPositions.end(), darkL1.begin(), darkL1.end());
    darkOak.m_leafPositions.insert(darkOak.m_leafPositions.end(), darkL2.begin(), darkL2.end());
    m_darkOakVariants.push_back(darkOak);
    
    TreeStamp cactusSmall;
    cactusSmall.m_logType = BLOCK_TYPE_CACTUS_LOG;
    cactusSmall.m_leafType = BLOCK_TYPE_AIR;
    cactusSmall.m_logPositions = MakeColumn(3);
    m_cactusVariants.push_back(cactusSmall);
    
    TreeStamp cactusTall;
    cactusTall.m_logType = BLOCK_TYPE_CACTUS_LOG;
    cactusTall.m_leafType = BLOCK_TYPE_AIR;
    cactusTall.m_logPositions = MakeColumn(5);
    m_cactusVariants.push_back(cactusTall);
    
    m_treeStamps[BiomeGenerator::BIOME_FOREST] = m_oakVariants;
    m_treeStamps[BiomeGenerator::BIOME_FOREST].insert(
        m_treeStamps[BiomeGenerator::BIOME_FOREST].end(),
        m_birchVariants.begin(), m_birchVariants.end());
    
    m_treeStamps[BiomeGenerator::BIOME_PLAINS] = m_oakVariants;
    
    m_treeStamps[BiomeGenerator::BIOME_TAIGA] = m_spruceVariants;
    
    m_treeStamps[BiomeGenerator::BIOME_SNOWY_PLAINS] = m_snowySpruceVariants;
    m_treeStamps[BiomeGenerator::BIOME_SNOWY_TAIGA] = m_snowySpruceVariants;
    
    m_treeStamps[BiomeGenerator::BIOME_JUNGLE] = m_jungleVariants;
    
    m_treeStamps[BiomeGenerator::BIOME_SAVANNA] = m_acaciaVariants;
    
    m_treeStamps[BiomeGenerator::BIOME_DESERT] = m_cactusVariants;
}

std::vector<IntVec3> FeaturePlacer::MakeColumn(int height)
{
    std::vector<IntVec3> column;
    column.reserve(height);
    for (int i = 0; i < height; ++i)
    {
        column.emplace_back(0, 0, i);
    }
    return column;
}

std::vector<IntVec3> FeaturePlacer::MakeBlobLeaves(const IntVec3& center, int radius)
{
    std::vector<IntVec3> leaves;
    
    for (int dx = -radius; dx <= radius; ++dx)
    {
        for (int dy = -radius; dy <= radius; ++dy)
        {
            for (int dz = -radius; dz <= radius; ++dz)
            {
                int distSq = dx*dx + dy*dy + dz*dz;
                if (distSq <= radius * radius)
                {
                    if (dx == 0 && dy == 0 && dz <= 0)
                        continue;
                    
                    leaves.emplace_back(
                        center.x + dx, 
                        center.y + dy, 
                        center.z + dz);
                }
            }
        }
    }
    
    return leaves;
}

std::vector<IntVec3> FeaturePlacer::MakeSpruceLeaves(int treeHeight)
{
    std::vector<IntVec3> leaves;
    
    int startZ = treeHeight - 5;
    if (startZ < 2) startZ = 2;
    
    for (int layer = 0; layer < 5 && startZ + layer < treeHeight; ++layer)
    {
        int z = startZ + layer;

        int radius = 2 - (layer / 2);
        if (radius < 0) radius = 0;
        
        for (int dx = -radius; dx <= radius; ++dx)
        {
            for (int dy = -radius; dy <= radius; ++dy)
            {
                if (abs(dx) + abs(dy) <= radius)
                {
                    if (dx == 0 && dy == 0)
                        continue;
                    
                    leaves.emplace_back(dx, dy, z);
                }
            }
        }
    }
    
    if (treeHeight > 0)
    {
        leaves.emplace_back(0, 0, treeHeight - 1);
    }
    
    return leaves;
}

std::vector<IntVec3> FeaturePlacer::MakeAcaciaLeaves()
{
    std::vector<IntVec3> leaves;
    
    // Acacia特点：扁平、偏向一侧
    // 主要在z=4和z=5层
    
    // z=4层：大范围
    for (int dx = -2; dx <= 3; ++dx)
    {
        for (int dy = -2; dy <= 2; ++dy)
        {
            if (abs(dy) + MaxI(0, abs(dx) - 1) <= 3)
            {
                if (dx == 0 && dy == 0) continue;
                leaves.emplace_back(dx, dy, 4);
            }
        }
    }
    
    // z=5层：较小
    for (int dx = -1; dx <= 2; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            if (abs(dy) + abs(dx) <= 2)
            {
                if (dx == 0 && dy == 0) continue;
                leaves.emplace_back(dx, dy, 5);
            }
        }
    }
    
    return leaves;
}

std::vector<IntVec3> FeaturePlacer::MakeDarkOakTrunk(int height)
{
    std::vector<IntVec3> trunk;
    trunk.reserve(height * 4);
    
    for (int z = 0; z < height; ++z)
    {
        // 2x2的树干
        trunk.emplace_back(0, 0, z);
        trunk.emplace_back(1, 0, z);
        trunk.emplace_back(0, 1, z);
        trunk.emplace_back(1, 1, z);
    }
    
    return trunk;
}

bool FeaturePlacer::ShouldPlaceTree(int worldX, int worldY, BiomeGenerator::BiomeType biome)
{
    UNUSED(worldY)
    UNUSED(worldX)
    float treeDensity = 0.0f;
    
    switch (biome) 
    {
        case BiomeGenerator::BIOME_FOREST:      treeDensity = 0.08f; break;
        case BiomeGenerator::BIOME_JUNGLE:      treeDensity = 0.12f; break;
        case BiomeGenerator::BIOME_PLAINS:      treeDensity = 0.005f; break;
        case BiomeGenerator::BIOME_TAIGA:       treeDensity = 0.06f; break;
        case BiomeGenerator::BIOME_SNOWY_PLAINS: treeDensity = 0.02f; break;
        case BiomeGenerator::BIOME_SNOWY_TAIGA:  treeDensity = 0.05f; break;
        case BiomeGenerator::BIOME_SAVANNA:     treeDensity = 0.01f; break;
        case BiomeGenerator::BIOME_DESERT:      treeDensity = 0.003f; break;
        default: return false;
    }
    
    // unsigned int seed = (unsigned int)(worldX * 73856093 ^ worldY * 19349663) ^ m_treeSeed;
    // m_rng.SetSeed(seed);
    
    return m_rng.RollRandomFloatZeroToOne() < treeDensity;
}

void FeaturePlacer::PlaceTree(Chunk* chunk, int localX, int localY, int surfaceZ, 
                              BiomeGenerator::BiomeType biome, float temperature)
{
    UNUSED(temperature)
    
    auto it = m_treeStamps.find(biome);
    if (it == m_treeStamps.end() || it->second.empty())
        return;
    
    const auto& candidates = it->second;
    
    //IntVec2 chunkCoords = chunk->GetThisChunkCoords();
    //int worldX = chunkCoords.x * CHUNK_SIZE_X + localX;
    //int worldY = chunkCoords.y * CHUNK_SIZE_Y + localY;
    
    // unsigned int seed = (unsigned int)(worldX * 73856093 ^ worldY * 19349663) ^ m_treeSeed;
    // m_rng.SetSeed(seed);
    
    int variantIndex = m_rng.RollRandomIntInRange(0, (int)candidates.size() - 1);
    TreeStamp stamp = candidates[variantIndex];
    
    if (!CanPlaceTree(chunk, localX, localY, surfaceZ, stamp))
    {
        return;
    }
    
    for (const IntVec3& offset : stamp.m_logPositions)
    {
        int x = localX + offset.x;
        int y = localY + offset.y;
        int z = surfaceZ + 1 + offset.z;
        
        if (x >= 0 && x < CHUNK_SIZE_X &&
            y >= 0 && y < CHUNK_SIZE_Y &&
            z >= 0 && z < CHUNK_SIZE_Z)
        {
            int idx = LocalCoordsToIndex(x, y, z);
            chunk->m_blocks[idx].SetType(stamp.m_logType);
        }
    }
    
    if (stamp.m_leafType != BLOCK_TYPE_AIR)
    {
        for (const IntVec3& offset : stamp.m_leafPositions)
        {
            int x = localX + offset.x;
            int y = localY + offset.y;
            int z = surfaceZ + 1 + offset.z;
            
            if (x >= 0 && x < CHUNK_SIZE_X &&
                y >= 0 && y < CHUNK_SIZE_Y &&
                z >= 0 && z < CHUNK_SIZE_Z)
            {
                int idx = LocalCoordsToIndex(x, y, z);
                uint8_t currentType = chunk->m_blocks[idx].m_typeIndex;
                
                if (currentType == BLOCK_TYPE_AIR || currentType == BLOCK_TYPE_SNOW)
                {
                    chunk->m_blocks[idx].SetType(stamp.m_leafType);
                }
            }
        }
    }
}

void FeaturePlacer::PlaceTree(Chunk* chunk, int localX, int localY, int surfaceZ, 
                              BiomeGenerator::BiomeType biome)
{
    PlaceTree(chunk, localX, localY, surfaceZ, biome, 0.0f);
}

bool FeaturePlacer::CanPlaceTree(Chunk* chunk, int localX, int localY, int surfaceZ, 
                                 const TreeStamp& stamp)
{
    auto inside = [](int x, int y, int z)
    {
        return (x >= 0 && x < CHUNK_SIZE_X) &&
               (y >= 0 && y < CHUNK_SIZE_Y) &&
               (z >= 0 && z < CHUNK_SIZE_Z);
    };

    for (const IntVec3& o : stamp.m_logPositions)
    {
        int x = localX + o.x;
        int y = localY + o.y;
        int z = surfaceZ + 1 + o.z;
        if (!inside(x, y, z))
            return false; 
        int idx = LocalCoordsToIndex(x, y, z);
        if (chunk->m_blocks[idx].m_typeIndex != BLOCK_TYPE_AIR) return false;
    }
    
    for (const IntVec3& o : stamp.m_leafPositions)
    {
        int x = localX + o.x;
        int y = localY + o.y;
        int z = surfaceZ + 1 + o.z;
        if (!inside(x, y, z)) return false; 
        int idx = LocalCoordsToIndex(x, y, z);
        uint8_t t = chunk->m_blocks[idx].m_typeIndex;
        if (!(t == BLOCK_TYPE_AIR || t == BLOCK_TYPE_SNOW))
            return false;
    }

    return true;
}

FeaturePlacer::TreeStamp FeaturePlacer::GetTreeStamp(BiomeGenerator::BiomeType biome)
{
    auto it = m_treeStamps.find(biome);
    if (it != m_treeStamps.end() && !it->second.empty())
    {
        return it->second[0];
    }
    return TreeStamp();
}

bool FeaturePlacer::ShouldPlaceUnderwaterPlant(int worldX, int worldY, BiomeGenerator::BiomeType biome)
{
    UNUSED(worldX);
    UNUSED(worldY);
    
    float plantDensity = 0.0f;
    
    switch (biome) 
    {
        case BiomeGenerator::BIOME_OCEAN:        plantDensity = 0.03f; break;  // 普通海洋 - 中等密度
        case BiomeGenerator::BIOME_DEEP_OCEAN:   plantDensity = 0.02f; break;  // 深海 - 较少
        case BiomeGenerator::BIOME_FROZEN_OCEAN: plantDensity = 0.01f; break;  // 冰冻海洋 - 稀少
        default: return false;
    }
    
    return m_rng.RollRandomFloatZeroToOne() < plantDensity;
}

void FeaturePlacer::PlaceUnderwaterPlants(Chunk* chunk, int localX, int localY, int seabedZ, 
                                           BiomeGenerator::BiomeType biome, float temperature)
{
    if (seabedZ < 0 || seabedZ >= CHUNK_SIZE_Z - 1)
        return;
    
    int idx = LocalCoordsToIndex(localX, localY, seabedZ);
    uint8_t surfaceBlock = chunk->m_blocks[idx].m_typeIndex;
    
    if (surfaceBlock != BLOCK_TYPE_SAND && 
        surfaceBlock != BLOCK_TYPE_DIRT && 
        surfaceBlock != BLOCK_TYPE_STONE)
        return;
    
    int idxAbove = LocalCoordsToIndex(localX, localY, seabedZ + 1);
    uint8_t blockAbove = chunk->m_blocks[idxAbove].m_typeIndex;
    
    if (blockAbove != BLOCK_TYPE_WATER)
    {
        return;  
    }
    
    int waterDepth = 0;
    for (int z = seabedZ + 1; z < CHUNK_SIZE_Z; z++)
    {
        int waterIdx = LocalCoordsToIndex(localX, localY, z);
        if (chunk->m_blocks[waterIdx].m_typeIndex == BLOCK_TYPE_WATER)
            waterDepth++;
        else
            break;
    }
    if (waterDepth < 2)
        return;
    
    float rand = m_rng.RollRandomFloatZeroToOne();
    
    if (biome == BiomeGenerator::BIOME_FROZEN_OCEAN)
    {
        if (rand < 0.50f)
        {
            PlaceSeagrass(chunk, localX, localY, seabedZ + 1);
        }
    }
    else if (biome == BiomeGenerator::BIOME_DEEP_OCEAN)
    {
        if (rand < 0.40f && waterDepth >= 4)
        {
            PlaceKelp(chunk, localX, localY, seabedZ + 1, waterDepth);
        }
        else if (rand < 0.60f)
        {
            PlaceSeagrass(chunk, localX, localY, seabedZ + 1);
        }
    }
    else if (biome == BiomeGenerator::BIOME_OCEAN)
    {
        if (temperature > 0.5f)  
        {
            if (rand < 0.10f && waterDepth >= 3 && waterDepth <= 8)
            {
                PlaceCoralBlock(chunk, localX, localY, seabedZ + 1);
            }
            else if (rand < 0.30f && waterDepth >= 4)
            {
                PlaceKelp(chunk, localX, localY, seabedZ + 1, waterDepth);
            }
            else if (rand < 0.50f && waterDepth >= 3)
            {
                PlaceTallSeagrass(chunk, localX, localY, seabedZ + 1);
            }
            else if (rand < 0.80f)
            {
                PlaceSeagrass(chunk, localX, localY, seabedZ + 1);
            }
        }
        else 
        {
            if (rand < 0.25f && waterDepth >= 4)
            {
                PlaceKelp(chunk, localX, localY, seabedZ + 1, waterDepth);
            }
            else if (rand < 0.45f && waterDepth >= 3)
            {
                PlaceTallSeagrass(chunk, localX, localY, seabedZ + 1);
            }
            else if (rand < 0.70f)
            {
                PlaceSeagrass(chunk, localX, localY, seabedZ + 1);
            }
        }
    }
}

void FeaturePlacer::PlaceSeagrass(Chunk* chunk, int x, int y, int z)
{
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || 
        z < 0 || z >= CHUNK_SIZE_Z - 1)
        return;
    
    int idx = LocalCoordsToIndex(x, y, z);
    if (chunk->m_blocks[idx].m_typeIndex != BLOCK_TYPE_WATER)
        return;
    
    // 确保上方也是水（不要放在水面边缘）
    int idxAbove = LocalCoordsToIndex(x, y, z + 1);
    if (chunk->m_blocks[idxAbove].m_typeIndex != BLOCK_TYPE_WATER)
        return;
    
    chunk->m_blocks[idx].SetType(BLOCK_TYPE_SEAGRASS);
}

void FeaturePlacer::PlaceTallSeagrass(Chunk* chunk, int x, int y, int z)
{
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || 
        z < 0 || z >= CHUNK_SIZE_Z - 2)
        return;
    
    int idxBottom = LocalCoordsToIndex(x, y, z);
    int idxTop = LocalCoordsToIndex(x, y, z + 1);
    
    if (chunk->m_blocks[idxBottom].m_typeIndex != BLOCK_TYPE_WATER ||
        chunk->m_blocks[idxTop].m_typeIndex != BLOCK_TYPE_WATER)
        return;
    
    // 确保顶部上方也是水（不要放在水面边缘）
    if (z + 2 < CHUNK_SIZE_Z)
    {
        int idxAbove = LocalCoordsToIndex(x, y, z + 2);
        if (chunk->m_blocks[idxAbove].m_typeIndex != BLOCK_TYPE_WATER)
        {
            // 上方不是水，改放普通海草
            PlaceSeagrass(chunk, x, y, z);
            return;
        }
    }
    else
    {
        return;
    }
    
    chunk->m_blocks[idxBottom].SetType(BLOCK_TYPE_TALL_SEAGRASS_BOTTOM);
    chunk->m_blocks[idxTop].SetType(BLOCK_TYPE_TALL_SEAGRASS_TOP);
}

void FeaturePlacer::PlaceKelp(Chunk* chunk, int x, int y, int z, int waterDepth)
{
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || 
        z < 0 || z >= CHUNK_SIZE_Z)
        return;
    
    // 确保顶部至少有 2 格水覆盖（-3 而不是 -2）
    int maxHeight = waterDepth - 3;
    if (maxHeight < 1)
        return;
    
    int kelpHeight = m_rng.RollRandomIntInRange(2, 8);
    if (kelpHeight > maxHeight)
        kelpHeight = maxHeight;
    
    for (int i = 0; i < kelpHeight && (z + i) < CHUNK_SIZE_Z; i++)
    {
        int idx = LocalCoordsToIndex(x, y, z + i);
        
        // 确保当前位置是水
        if (chunk->m_blocks[idx].m_typeIndex != BLOCK_TYPE_WATER)
            break;
        
        // 检查上方是否有水（不要长到水面边缘）
        if (z + i + 1 < CHUNK_SIZE_Z)
        {
            int idxAbove = LocalCoordsToIndex(x, y, z + i + 1);
            if (chunk->m_blocks[idxAbove].m_typeIndex != BLOCK_TYPE_WATER)
            {
                // 上方不是水，停止生长（不放这一格）
                break;
            }
        }
        
        if (i == kelpHeight - 1)
        {
            chunk->m_blocks[idx].SetType(BLOCK_TYPE_KELP_TOP);
        }
        else
        {
            chunk->m_blocks[idx].SetType(BLOCK_TYPE_KELP);
        }
    }
}

void FeaturePlacer::PlaceCoralBlock(Chunk* chunk, int x, int y, int z)
{
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || 
        z < 0 || z >= CHUNK_SIZE_Z - 1)
        return;
    
    int idx = LocalCoordsToIndex(x, y, z);
    if (chunk->m_blocks[idx].m_typeIndex != BLOCK_TYPE_WATER)
        return;
    
    // 确保上方也是水（不要放在水面边缘）
    int idxAbove = LocalCoordsToIndex(x, y, z + 1);
    if (chunk->m_blocks[idxAbove].m_typeIndex != BLOCK_TYPE_WATER)
        return;
    
    float rand = m_rng.RollRandomFloatZeroToOne();
    BlockType coralType;
    
    if (rand < 0.25f)
        coralType = BLOCK_TYPE_CORAL_BLOCK_RED;
    else if (rand < 0.5f)
        coralType = BLOCK_TYPE_CORAL_BLOCK_PURPLE;
    else if (rand < 0.75f)
        coralType = BLOCK_TYPE_CORAL_BLOCK_YELLOW;
    else
        coralType = BLOCK_TYPE_CORAL_BLOCK_FAN_DEAD;
    
    chunk->m_blocks[idx].SetType(coralType);
    
    // 扩展珊瑚
    if (m_rng.RollRandomFloatZeroToOne() < 0.3f)
    {
        int directions[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
        for (int i = 0; i < 4; i++)
        {
            if (m_rng.RollRandomFloatZeroToOne() < 0.4f)
            {
                int nx = x + directions[i][0];
                int ny = y + directions[i][1];
                
                if (nx >= 0 && nx < CHUNK_SIZE_X && ny >= 0 && ny < CHUNK_SIZE_Y)
                {
                    int nIdx = LocalCoordsToIndex(nx, ny, z);
                    if (chunk->m_blocks[nIdx].m_typeIndex == BLOCK_TYPE_WATER)
                    {
                        // 确保扩展位置上方也是水
                        if (z + 1 < CHUNK_SIZE_Z)
                        {
                            int nIdxAbove = LocalCoordsToIndex(nx, ny, z + 1);
                            if (chunk->m_blocks[nIdxAbove].m_typeIndex == BLOCK_TYPE_WATER)
                            {
                                chunk->m_blocks[nIdx].SetType(coralType);
                            }
                        }
                    }
                }
            }
        }
    }
}