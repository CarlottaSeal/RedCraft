#include "Chunk.h"

#include "ChunkUtils.h"
#include "Game.hpp"
#include "Player.hpp"
#include "World.h"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/IntVec3.h"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Save/SaveSystem.h"
#include "Gameplay/RedstoneSimulator.h"
#include "Generator/WorldGenPipeline.h"
#include "ThirdParty/Noise/SmoothNoise.hpp"
#include "ThirdParty/Noise/RawNoise.hpp"

extern Game* g_theGame;
extern RandomNumberGenerator* g_theRNG;

Chunk::Chunk(World* owner, IntVec2 chunkCoords)
    :m_chunkCoords(chunkCoords),m_world(owner)
{
    float minX = (float)(chunkCoords.x * CHUNK_SIZE_X);
    float minY = (float)(chunkCoords.y * CHUNK_SIZE_Y);
    m_bounds.m_mins = Vec3(minX, minY, 0.f);
    m_bounds.m_maxs = Vec3(minX + CHUNK_SIZE_X, minY + CHUNK_SIZE_Y, (float)CHUNK_SIZE_Z);
    
    // GenerateBlocks();
    // m_serializer = new ChunkSerializer(this); 
    // GenerateDebug();
    m_vertices.reserve(40000);
    m_indices.reserve(60000);

    ReportDirty();
}

Chunk::~Chunk()
{
    if (m_vertexBuffer != nullptr)
    {
        delete m_vertexBuffer;
        m_vertexBuffer = nullptr;

        delete m_vertexBufferDebug;
        m_vertexBufferDebug = nullptr;
    }
    if (m_indexBuffer != nullptr)
    {
        delete m_indexBuffer;
        m_indexBuffer = nullptr;

        delete m_indexBufferDebug;
        m_indexBufferDebug = nullptr;
    }
    if (m_serializer)
    {
        delete m_serializer;
        m_serializer = nullptr;
    }
}

void Chunk::InitializeLighting()
{
     // 标记与相邻 Chunk 接触的边界非不透明方块为脏 
    if (m_eastNeighbor && m_eastNeighbor->GetState() == ChunkState::ACTIVE)
    {
        for (int y = 0; y < CHUNK_SIZE_Y; y++)
        {
            for (int z = 0; z < CHUNK_SIZE_Z; z++)
            {
                int idx = LocalCoordsToIndex(CHUNK_MAX_X, y, z);
                if (!m_blocks[idx].IsOpaque())
                {
                    BlockIterator iter(this, idx);
                    m_world->MarkLightingDirty(iter);
                }
            }
        }
    }
    
    if (m_westNeighbor && m_westNeighbor->GetState() == ChunkState::ACTIVE)
    {
        for (int y = 0; y < CHUNK_SIZE_Y; y++)
        {
            for (int z = 0; z < CHUNK_SIZE_Z; z++)
            {
                int idx = LocalCoordsToIndex(0, y, z);
                if (!m_blocks[idx].IsOpaque())
                {
                    BlockIterator iter(this, idx);
                    m_world->MarkLightingDirty(iter);
                }
            }
        }
    }
    
    if (m_northNeighbor && m_northNeighbor->GetState() == ChunkState::ACTIVE)
    {
        for (int x = 0; x < CHUNK_SIZE_X; x++)
        {
            for (int z = 0; z < CHUNK_SIZE_Z; z++)
            {
                int idx = LocalCoordsToIndex(x, CHUNK_MAX_Y, z);
                if (!m_blocks[idx].IsOpaque())
                {
                    BlockIterator iter(this, idx);
                    m_world->MarkLightingDirty(iter);
                }
            }
        }
    }
    
    if (m_southNeighbor && m_southNeighbor->GetState() == ChunkState::ACTIVE)
    {
        for (int x = 0; x < CHUNK_SIZE_X; x++)
        {
            for (int z = 0; z < CHUNK_SIZE_Z; z++)
            {
                int idx = LocalCoordsToIndex(x, 0, z);
                if (!m_blocks[idx].IsOpaque())
                {
                    BlockIterator iter(this, idx);
                    m_world->MarkLightingDirty(iter);
                }
            }
        }
    }
    
    // ===== 步骤2 & 3：标记天空方块并设置室外光 =====
    for (int y = 0; y < CHUNK_SIZE_Y; y++)
    {
        for (int x = 0; x < CHUNK_SIZE_X; x++)
        {
            // 从上往下找第一个不透明方块
            for (int z = CHUNK_SIZE_Z - 1; z >= 0; z--)
            {
                int idx = LocalCoordsToIndex(x, y, z);
                Block& block = m_blocks[idx];
                
                if (block.IsOpaque())
                {
                    // 遇到不透明方块，停止
                    break;
                }
                else
                {
                    block.SetIsSky(true);
                }
            }
        }
    }
    
    // ===== 步骤4：为天空方块设置室外光，并标记其水平邻居为脏 =====
    for (int z = 0; z < CHUNK_SIZE_Z; z++)
    {
        for (int y = 0; y < CHUNK_SIZE_Y; y++)
        {
            for (int x = 0; x < CHUNK_SIZE_X; x++)
            {
                int idx = LocalCoordsToIndex(x, y, z);
                Block& block = m_blocks[idx];
                
                if (block.IsSky())
                {
                    // 设置室外光 = 15
                    block.SetOutdoorLight(15);
                    
                    // 标记四个水平方向的非天空非不透明邻居为脏
                    Direction horizontalDirs[] = {
                        DIRECTION_EAST, DIRECTION_WEST, 
                        DIRECTION_NORTH, DIRECTION_SOUTH
                    };
                    
                    for (Direction dir : horizontalDirs)
                    {
                        BlockIterator iter(this, idx);
                        BlockIterator neighbor = iter.GetNeighborCrossBoundary(dir);
                        
                        if (neighbor.IsValid())
                        {
                            Block* neighborBlock = neighbor.GetBlock();
                            if (neighborBlock && 
                                !neighborBlock->IsSky() && 
                                !neighborBlock->IsOpaque())
                            {
                                m_world->MarkLightingDirty(neighbor);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 步骤5：标记所有发光方块为脏
    for (int idx = 0; idx < CHUNK_TOTAL_BLOCKS; idx++)
    {
        Block& block = m_blocks[idx];
        const BlockDefinition& def = BlockDefinition::GetBlockDef(block.m_typeIndex);
        
        if (def.m_indoorLightInfluence > 0 || def.m_outdoorLightInfluence > 0)
        {
            BlockIterator iter(this, idx);
            m_world->MarkLightingDirty(iter);
        }
    }
}

int Chunk::GetBlockLocalIndexFromLocalCoords(int x, int y, int z) const
{
    return x | (y << 4) | (z << 8);
}

void Chunk::GetLocalCoordsFromIndex(int index, int& x, int& y, int& z) const
{
    x = index & CHUNK_MASK_X;                          // x = index & 0x0F
    y = (index & CHUNK_MASK_Y) >> CHUNK_BITS_X;        // y = (index & 0xF0) >> 4  
    z = (index & CHUNK_MASK_Z) >> CHUNK_BITS_XY;       // z = (index & 0x7F00) >> 8
    // const int XY = CHUNK_SIZE_X * CHUNK_SIZE_Y;
    // z = index / XY;
    // int rem = index % XY;
    // y = rem / CHUNK_SIZE_X;
    // x = rem % CHUNK_SIZE_X;
}

IntVec3 Chunk::GetLocalCoordsFromIndex(int index) const
{
    int x = index & CHUNK_MASK_X;                          // x = index & 0x0F
    int y = (index & CHUNK_MASK_Y) >> CHUNK_BITS_X;        // y = (index & 0xF0) >> 4  
    int z = (index & CHUNK_MASK_Z) >> CHUNK_BITS_XY;       // z = (index & 0x7F00) >> 8
    return IntVec3(x, y, z);
}

Block Chunk::GetBlock(int localX, int localY, int localZ) const
{
    return m_blocks[GetBlockLocalIndexFromLocalCoords(localX, localY, localZ)];
}

Vec3 Chunk::GetBlockWorldPosition(int blockIndex) const
{
    int localX = blockIndex & CHUNK_MASK_X;                     
    int localY = (blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X;   
    int localZ = (blockIndex & CHUNK_MASK_Z) >> CHUNK_BITS_XY;  
    
	float worldX = (float)((m_chunkCoords.x * CHUNK_SIZE_X) + localX);
	float worldY = (float)((m_chunkCoords.y * CHUNK_SIZE_Y) + localY);
	float worldZ = (float)localZ;  

	return Vec3(worldX, worldY, worldZ);
}

void Chunk::Update(float deltaSeconds)
{
    UNUSED(deltaSeconds)
    UpdateInputForDigAndPlace();
}

void Chunk::Render() const
{
    if (m_vertexBuffer)
    {
		g_theRenderer->BindShader(m_world->m_worldShader);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);

		// Mat44 mat;
		// mat.SetTranslation2D(Vec2((float)(m_chunkCoords.x * CHUNK_SIZE_X),
		// 	(float)(m_chunkCoords.y * CHUNK_SIZE_Y)));
		// g_theRenderer->SetModelConstants(mat);
        g_theRenderer->SetModelConstants();
		g_theRenderer->BindTexture(&m_world->m_owner->m_spriteSheet->GetTexture());
		g_theRenderer->DrawIndexBuffer(m_vertexBuffer, m_indexBuffer, (unsigned int)m_indices.size());

		if (m_world->IsDebugging())
		{
		    g_theRenderer->SetModelConstants();
		    if (g_theGame->g_showChunkBounds)
		    {
		        g_theRenderer->BindTexture(nullptr);
		        g_theRenderer->DrawIndexBuffer(m_vertexBufferDebug, m_indexBufferDebug, (unsigned int)m_indicesDebug.size(), PrimitiveTopology::PRIMITIVE_LINES);
		    }

		    if (g_theGame->g_debugVisualizationMode == 0)
		        return;
		    
		    const float eps = 0.01f;
            const float cellW = (m_bounds.m_maxs.x - m_bounds.m_mins.x)/CHUNK_SIZE_X;
            const float cellH = (m_bounds.m_maxs.y - m_bounds.m_mins.y)/CHUNK_SIZE_Y;
            const float zTop = m_bounds.m_maxs.z + eps;
        
            std::vector<Vertex_PCU> debugVerts;
            for (int j = 0; j < 16; ++j)
            {
                for (int i = 0; i < 16; ++i)
                {
                    float c = 0.f;
                    if (g_theGame->g_debugVisualizationMode == 1)
                        c = m_chunkGenData.m_biomeParams[i][j].m_continentalness;
                    if (g_theGame->g_debugVisualizationMode == 2)
                        c = m_chunkGenData.m_biomeParams[i][j].m_erosion;
                    if (g_theGame->g_debugVisualizationMode == 3)
                        c = m_chunkGenData.m_biomeParams[i][j].m_peaksAndValleys;
                    if (g_theGame->g_debugVisualizationMode == 4)
                        c = m_chunkGenData.m_biomeParams[i][j].m_temperature;
                    if (g_theGame->g_debugVisualizationMode == 5)
                        c = m_chunkGenData.m_biomeParams[i][j].m_humidity;
                    if (g_theGame->g_debugVisualizationMode == 6) 
                        c =((float)(m_chunkGenData.m_biomes[i][j]/BiomeGenerator::BIOME_UNKNOWN))*2.f - 1.f;
                    if (g_theGame->g_debugVisualizationMode == 7)
                        c = m_chunkGenData.m_biomeParams[i][j].m_weirdness;
                    
                    float gray = RangeMap(c, -1.2f, 1.f, 0.f, 1.f);
                    Rgba8 color = Rgba8((unsigned char)gray * 255, (unsigned char)gray * 255, (unsigned char)gray * 255);
        
                    float x0 = m_bounds.m_mins.x + i * cellW;
                    float y0 = m_bounds.m_mins.y + j * cellH;
                    float x1 = x0 + cellW;
                    float y1 = y0 + cellH;
        
                    AddVertsForAABB3D(debugVerts, 
                        AABB3(Vec3(x0, y0, zTop), Vec3(x1, y1, zTop + eps)), 
                        color);
                }
            }
		    g_theRenderer->DrawVertexArray(debugVerts);
	    }
    
	    g_theRenderer->SetModelConstants();
    }
}

void Chunk::SetNeighbor(Direction dir, Chunk* neighbor)
{
    switch (dir)
    {
    case DIRECTION_NORTH: m_northNeighbor = neighbor; break;
    case DIRECTION_SOUTH: m_southNeighbor = neighbor; break;
    case DIRECTION_EAST:  m_eastNeighbor = neighbor; break;
    case DIRECTION_WEST:  m_westNeighbor = neighbor; break;

    case DIRECTION_UP:
    case DIRECTION_DOWN:
        break;
    }
}

Chunk* Chunk::GetNeighbor(Direction dir) const
{
    switch (dir)
    {
    case DIRECTION_NORTH: return m_northNeighbor;
    case DIRECTION_SOUTH: return m_southNeighbor;
    case DIRECTION_EAST:  return m_eastNeighbor;
    case DIRECTION_WEST:  return m_westNeighbor;
    default: return nullptr;
    }
}

void Chunk::MarkSelfDirty()
{
    m_isDirty = true;
}

void Chunk::MarkNeighborChunkDirty(Direction dir)
{
    if (dir == DIRECTION_UP || dir == DIRECTION_DOWN)
        return;
    
    Chunk* neighbor = GetNeighbor(dir);
    if (neighbor)
    {
        neighbor->m_isDirty = true;
        neighbor->m_needsImmediateRebuild = true;
    }
}

bool Chunk::IsOnBoundary(const IntVec3& localCoords, Direction dir) const
{
    switch (dir)
    {
    case DIRECTION_EAST:
        return localCoords.x == CHUNK_SIZE_X - 1;
    case DIRECTION_WEST:
        return localCoords.x == 0;
    case DIRECTION_NORTH:
        return localCoords.y == CHUNK_SIZE_Y - 1;
    case DIRECTION_SOUTH:
        return localCoords.y == 0;
    case DIRECTION_UP:
        return localCoords.z == CHUNK_SIZE_Z - 1;
    case DIRECTION_DOWN:
        return localCoords.z == 0;
    default: return false;
    }
}

void Chunk::UpdateInputForDigAndPlace()
{
    if (g_theApp->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
    {
        IntVec3 digPlace = FindDigTarget(g_theGame->m_player->m_position);
        DigBlock(digPlace);
    }
    if (g_theApp->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
    {
        IntVec3 placePlace = FindPlaceTarget(g_theGame->m_player->m_position);

        PlaceBlock(placePlace, m_world->m_typeToPlace);
    }
}

void Chunk::DigBlock(const IntVec3& localCoords)
{
    BlockIterator iter(this, localCoords);
    if (!iter.IsValid())
        return;

    Block* block = iter.GetBlock();
    if (!CanDigBlock(block->m_typeIndex))
        return;

    uint8_t oldType = block->m_typeIndex;
    
    block->SetType(BLOCK_TYPE_AIR);
    m_world->MarkLightingDirty(iter);
    
    BlockIterator above = iter.GetNeighborCrossBoundary(DIRECTION_UP);
    if (above.IsValid() && above.IsSky())
    {
        BlockIterator current = iter;
        while (current.IsValid())
        {
            Block* currentBlock = current.GetBlock();
            if (!currentBlock || currentBlock->IsOpaque())
                break;
            
            currentBlock->SetIsSky(true);
            currentBlock->SetOutdoorLight(15);
            m_world->MarkLightingDirty(current);
            
            current = current.GetNeighborCrossBoundary(DIRECTION_DOWN);
        }
    }
    
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = iter.GetNeighborCrossBoundary((Direction)dir);
        if (neighbor.IsValid())
        {
            m_world->MarkLightingDirtyIfNotOpaque(neighbor);
            
            // 如果邻居在不同 Chunk，标记那个 Chunk 为脏
            if (neighbor.GetChunk() != this)
            {
                neighbor.GetChunk()->m_isDirty = true;
                m_world->m_hasDirtyChunk = true;
            }
        }
    }
    
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        if (IsOnBoundary(localCoords, (Direction)dir))
        {
            Chunk* neighbor = GetNeighbor((Direction)dir);
            if (neighbor)
            {
                neighbor->m_needsImmediateRebuild = true;
                m_world->m_hasDirtyChunk = true;
            }
        }
    }
    
    GenerateMesh();
    m_needsSaving = true;

    m_world->OnBlockRemoved(iter, oldType);
}

void Chunk::PlaceBlock(const IntVec3& localCoords, uint8_t blockType)
{
    BlockIterator iter(this, localCoords);
    if (!iter.IsValid())
        return;

    Block* block = iter.GetBlock();
    if (block->m_typeIndex != BLOCK_TYPE_AIR && 
        block->m_typeIndex != BLOCK_TYPE_WATER &&
        block->m_typeIndex != BLOCK_TYPE_LAVA)
        return;
    
    if (!HasSupport(localCoords, blockType))
    {
        return; 
    }

    bool wasSky = block->IsSky();
    block->SetType(blockType);

    Direction playerFacing = m_game->m_player->GetOrthoDirection();
    if (blockType == BLOCK_TYPE_PISTON ||
        blockType == BLOCK_TYPE_STICKY_PISTON ||
        blockType == BLOCK_TYPE_OBSERVER ||
        blockType == BLOCK_TYPE_DISPENSER ||
        blockType == BLOCK_TYPE_DROPPER)
    {
        // 这些方块：朝向玩家看的方向（6方向）
        block->SetBlockFacing((uint8_t)playerFacing);
    }
    else if (blockType == BLOCK_TYPE_REPEATER_OFF ||
             blockType == BLOCK_TYPE_REPEATER_ON)
    {
        // 中继器：信号输出方向 = 玩家看的方向（只用水平4方向）
        Direction horizontal = m_game->m_player->GetHorizontalDirection();
        block->SetBlockFacing((uint8_t)horizontal);
        block->SetRepeaterDelay(1);  // 默认1档延迟
    }
    else if (blockType == BLOCK_TYPE_COMPARATOR_OFF ||
             blockType == BLOCK_TYPE_COMPARATOR_ON)
    {
        // 比较器：同中继器
        Direction horizontal = GetHorizontalDirection(playerFacing);
        block->SetBlockFacing((uint8_t)horizontal);
        block->SetComparatorMode(0);  // 默认比较模式
    }
    else if (blockType == BLOCK_TYPE_LEVER)
    {
        // 拉杆：附着在地面，朝上
        block->SetBlockFacing((uint8_t)DIRECTION_UP);
        block->SetLeverState(false);
    }
    else if (blockType == BLOCK_TYPE_BUTTON_STONE ||
             blockType == BLOCK_TYPE_BUTTON_WOOD)
    {
        // 按钮：附着在地面
        block->SetBlockFacing((uint8_t)DIRECTION_UP);
        block->SetButtonPressed(false);
    }
    else if (blockType == BLOCK_TYPE_REDSTONE_TORCH)
    {
        // 红石火把：放在地上
        block->SetBlockFacing((uint8_t)DIRECTION_UP);
    }
    else if (IsRedstoneWire(blockType))
    {
        // 红石线：初始化为DOT，后续会自动更新连接
        block->SetType(BLOCK_TYPE_REDSTONE_WIRE_DOT);
        block->SetRedstonePower(0);
    }
    
    //light deal
    // 1. 标记该方块光照为脏
    m_world->MarkLightingDirty(iter);
    
    // 2. 如果被替换的方块是天空 且 新方块不透明，向下清除天空标记
    if (wasSky && block->IsOpaque())
    {
        // 清除当前方块的天空标记
        block->SetIsSky(false);
        block->SetOutdoorLight(0);
        
        // 向下遍历，清除所有天空标记
        BlockIterator current = iter.GetNeighborCrossBoundary(DIRECTION_DOWN);
        while (current.IsValid())
        {
            Block* currentBlock = current.GetBlock();
            if (!currentBlock)
                break;
            
            if (!currentBlock->IsSky())
            {
                // 已经不是天空了，停止
                break;
            }
            
            // 清除天空标记
            currentBlock->SetIsSky(false);
            currentBlock->SetOutdoorLight(0);
            
            // 标记光照为脏
            m_world->MarkLightingDirty(current);
            
            // 向下移动
            current = current.GetNeighborCrossBoundary(DIRECTION_DOWN);
        }
    }
    
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = iter.GetNeighborCrossBoundary((Direction)dir);
        if (neighbor.IsValid())
        {
            m_world->MarkLightingDirtyIfNotOpaque(neighbor);
            
            if (neighbor.GetChunk() != this)
            {
                neighbor.GetChunk()->m_isDirty = true;
                m_world->m_hasDirtyChunk = true;
            }
        }
    }
    
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        if (IsOnBoundary(localCoords, (Direction)dir))
        {
            Chunk* neighbor = GetNeighbor((Direction)dir);
            if (neighbor)
            {
                neighbor->m_needsImmediateRebuild = true;
                m_world->m_hasDirtyChunk = true;
            }
        }
    }
    
    GenerateMesh();
    m_needsSaving = true;
    
    m_world->OnBlockPlaced(iter);
    if (IsRedstoneWire(blockType))
    {
        UpdateRedstoneWireConnections(globalCoords);
    }
}

IntVec3 Chunk::FindDigTarget(const Vec3& worldPos)
{
    int wx = FloorToInt(worldPos.x);
    int wy = FloorToInt(worldPos.y);
    int wz = FloorToInt(worldPos.z);

    if (wz < 0)
        return IntVec3(0, 0, -1); 
    if (wz >= CHUNK_SIZE_Z)
        wz = CHUNK_SIZE_Z - 1;

    for (int z = wz; z >= 0; --z)
    {
        Block b = m_world->GetBlockAtWorldCoords(wx, wy, z);
        
        if (b.m_typeIndex == BLOCK_TYPE_AIR || 
            b.m_typeIndex == BLOCK_TYPE_WATER ||
            b.m_typeIndex == BLOCK_TYPE_LAVA)
            {
            continue;
            }
        
        IntVec2 chunkOfTarget = GetChunkCoords(IntVec3(wx, wy, z));
        if (chunkOfTarget != m_chunkCoords)
            return IntVec3(0, 0, -1);

        IntVec3 local = GlobalCoordsToLocalCoords(IntVec3(wx, wy, z));
        return local;
    }
    return IntVec3(0, 0, -1); 
}

IntVec3 Chunk::FindPlaceTarget(const Vec3& worldPos)
{
    IntVec3 digLocal = FindDigTarget(worldPos);
    if (digLocal.z == -1)
        return IntVec3(0, 0, -1); 
    if (digLocal.z + 1 >= CHUNK_SIZE_Z)
        return IntVec3(0, 0, -1); 

    IntVec3 placeLocal(digLocal.x, digLocal.y, digLocal.z + 1);
    
    if (placeLocal.z >= CHUNK_SIZE_Z)
        return IntVec3(0, 0, -1); 

    IntVec3 placeGlobal = GetGlobalCoords(m_chunkCoords, placeLocal);
    Block above = m_world->GetBlockAtWorldCoords(placeGlobal.x, placeGlobal.y, placeGlobal.z);
    
    if (above.m_typeIndex != BLOCK_TYPE_AIR && 
        above.m_typeIndex != BLOCK_TYPE_WATER)
        {
        return IntVec3(0, 0, -1);
        }

    return placeLocal;
}

bool Chunk::CanDigBlock(uint8_t blockType)
{
    switch (blockType)
    {
    case BLOCK_TYPE_AIR:
    case BLOCK_TYPE_WATER:
    case BLOCK_TYPE_LAVA:
    case BLOCK_TYPE_OBSIDIAN: 
        return false;
    default:
        return true;
    }
}

bool Chunk::HasSupport(const IntVec3& localCoords, uint8_t blockType)
{
    if (blockType == BLOCK_TYPE_SAND)
    {
        if (localCoords.z > 0)
        {
            int belowIdx = LocalCoordsToIndex(localCoords);
            if (m_blocks[belowIdx].m_typeIndex == BLOCK_TYPE_AIR ||
                m_blocks[belowIdx].m_typeIndex == BLOCK_TYPE_WATER)
            {
                return false;
            }
        }
    }
    
    if (IsFoliage(blockType))
    {
        // 简化版：检查周围3x3x3范围内是否有木头
        for (int dx = -1; dx <= 1; dx++)
        {
            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dz = -1; dz <= 1; dz++)
                {
                    int x = localCoords.x + dx;
                    int y = localCoords.y + dy;
                    int z = localCoords.z + dz;
                    
                    if (x >= 0 && x < CHUNK_SIZE_X && 
                        y >= 0 && y < CHUNK_SIZE_Y && 
                        z >= 0 && z < CHUNK_SIZE_Z)
                    {
                        int idx = LocalCoordsToIndex(x, y, z);
                        if (IsLog(m_blocks[idx].m_typeIndex))
                        {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    if (IsSnow(blockType))
    {
		int belowZ = localCoords.z - 1;
		if (belowZ < 0)
			return false;

		uint8_t belowType = m_blocks[LocalCoordsToIndex(localCoords.x, localCoords.y, belowZ)].m_typeIndex;
		return IsSolid(belowType);
    }
    return true; 
}

void Chunk::Save()
{
    //if (!m_needsSaving)
    //    return;
    
    if (!m_serializer)
        m_serializer = new ChunkSerializer(this);

    // for (int i = 0; i < CHUNK_TOTAL_BLOCKS; i++)
    // {
    //     m_serializer->m_blockData[i] = m_blocks[i].m_typeIndex;
    // }
    memcpy(m_serializer->m_blockData, m_blocks, CHUNK_TOTAL_BLOCKS * sizeof(Block));
    g_theSaveSystem->Save(MakeChunkFilename(m_chunkCoords), m_serializer, SaveFormat::BINARY);
    m_needsSaving = false;
}

bool Chunk::Load()
{
    if (!m_serializer)
        m_serializer = new ChunkSerializer(this);
    
    const std::string fn = MakeChunkFilename(m_chunkCoords);
    if (!g_theSaveSystem->FileExists(fn))
        return false;
    
    if (!g_theSaveSystem->FileExists(fn))
        return false;

    // 调用 ISerializable::Load → 解压到 serializer 的缓冲
    if (!g_theSaveSystem->Load(fn, m_serializer, SaveFormat::BINARY))
        return false;

    //解压结果回写到 Chunk 的方块数组
    size_t totalBytes = CHUNK_TOTAL_BLOCKS * sizeof(Block);
    memcpy(m_blocks, m_serializer->m_blockData, totalBytes);
    // for (int i = 0; i < CHUNK_TOTAL_BLOCKS; ++i)
    // {
    //     m_blocks[i].m_typeIndex = m_serializer->m_blockData[i];
    //     BlockDefinition const& blockDef = BlockDefinition::GetBlockDef(m_blocks[i].m_typeIndex);
    //     m_blocks[i].SetIsOpaque(blockDef.m_isOpaque);
    //     m_blocks[i].SetIsSolid(blockDef.m_isSolid);
    //     m_blocks[i].SetIsVisible(blockDef.m_isVisible);
    // }

    m_isDirty = true;
    m_needsSaving = false;
    return true;
}

std::string Chunk::MakeChunkFilename(const IntVec2& chunkCoords)
{
    char name[64];
    std::snprintf(name, sizeof(name), "Chunk(%d,%d).chunk", chunkCoords.x, chunkCoords.y);
    return std::string(name);
}

void Chunk::GenerateBlocks()
{
    // m_vertices.clear();
    // m_indices.clear();
    // if (m_vertexBuffer != nullptr)
    // {
    //     delete m_vertexBuffer;
    //     m_vertexBuffer = nullptr;
    // }
    // if (m_indexBuffer != nullptr)
    // {
    //     delete m_indexBuffer;
    //     m_indexBuffer = nullptr;
    // }
    if (!m_world->m_worldGenPipeline)
    {
        m_world->m_worldGenPipeline = new WorldGenPipeline();
    }
    
    m_world->m_worldGenPipeline->GenerateChunk(this);

    //InitializeLighting();
    
    m_isDirty = true;
    m_needsSaving = true;
}

bool Chunk::GenerateMesh()
{
    bool allNeighborsActive = true;
    
    if (!m_eastNeighbor || m_eastNeighbor->GetState() != ChunkState::ACTIVE)
        allNeighborsActive = false;
    if (!m_westNeighbor || m_westNeighbor->GetState() != ChunkState::ACTIVE)
        allNeighborsActive = false;
    if (!m_northNeighbor || m_northNeighbor->GetState() != ChunkState::ACTIVE)
        allNeighborsActive = false;
    if (!m_southNeighbor || m_southNeighbor->GetState() != ChunkState::ACTIVE)
        allNeighborsActive = false;
    
    if (!allNeighborsActive)
    {
        //DebuggerPrintf("  Chunk (%d, %d) cannot generate mesh - neighbors not ready\n", 
          //             m_chunkCoords.x, m_chunkCoords.y);
        return false;
    }
    
    //DebuggerPrintf("  Chunk (%d, %d) generating mesh - SUCCESS\n", 
      //             m_chunkCoords.x, m_chunkCoords.y);
    
    m_vertices.clear();
    m_indices.clear();

    BlockIterator iter(this, 0);
    for (int blockIndex = 0; blockIndex < CHUNK_TOTAL_BLOCKS; blockIndex++)
    {
        iter = BlockIterator(this, blockIndex);
        
        if (!iter.IsValid() || iter.GetBlockType() == BLOCK_TYPE_AIR)
            continue;
        
        const BlockDefinition& blockDef = BlockDefinition::GetBlockDef(iter.GetBlockType());
        
        if (!blockDef.m_isVisible)
            continue;
        
        for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
        {
            Direction direction = (Direction)dir;
            
            if (ShouldRenderFace(iter, direction))
            {
                AddFaceToMesh(iter.GetLocalCoords(), blockDef, direction);
            }
        }
    }
    UpdateVBOIBO();
    m_isDirty = false;
    m_needsImmediateRebuild = false;

    GenerateDebug();
    m_needsSaving = true;
    return true;
}

void Chunk::GenerateDebug()
{
    m_verticesDebug.clear();
    m_indicesDebug.clear();
    if (m_vertexBufferDebug)
        delete m_vertexBufferDebug;
    if (m_indexBufferDebug)
        delete m_indexBufferDebug;
    m_vertexBufferDebug = nullptr;
    m_indexBufferDebug = nullptr;
    
    AddVertsForIndexAABBZWireframe3D(m_verticesDebug, m_indicesDebug, m_bounds);
    //AddVertsForIndexAABB3D(m_verticesDebug, m_indicesDebug, m_bounds);
    m_vertexBufferDebug = g_theRenderer->CreateVertexBuffer((unsigned int)m_verticesDebug.size() * sizeof(Vertex_PCU), sizeof(Vertex_PCU));
    m_indexBufferDebug = g_theRenderer->CreateIndexBuffer((unsigned int)m_indicesDebug.size()* sizeof(unsigned int), sizeof(unsigned int));

    g_theRenderer->CopyCPUToGPU(m_verticesDebug.data(), (unsigned int)(m_verticesDebug.size() * sizeof(Vertex_PCU)), m_vertexBufferDebug);
    g_theRenderer->CopyCPUToGPU(m_indicesDebug.data(), (unsigned int)(m_indicesDebug.size() * sizeof(unsigned int)), m_indexBufferDebug);
}

bool Chunk::ShouldRenderFace(const BlockIterator& iter, Direction direction)
{
    BlockIterator neighbor = iter.GetNeighborCrossBoundary(direction);
    
    if (!neighbor.IsValid())
        return true;
    
    Block* neighborBlock = neighbor.GetBlock();
    if (!neighborBlock)
        return true;
    if (neighborBlock->m_typeIndex == BLOCK_TYPE_AIR)
        return true;
    
    if (neighborBlock->IsOpaque())
        return false;
    
    BlockDefinition const& neighborDef = BlockDefinition::GetBlockDef(neighborBlock->m_typeIndex);
    if (!neighborDef.m_isOpaque)
        return true;
    
    return false;

    // IntVec3 localCoords = iter.GetLocalCoords();
    //
    // BlockIterator neighbor = iter.GetNeighbor(direction);
    //
    // if (neighbor.IsValid())
    // {
    //     return neighbor.IsTransparent();
    // }
    // if (IsOnBoundary(localCoords, direction))
    // {
    //     Chunk* neighborChunk = GetNeighbor(direction);
    //     if (neighborChunk)
    //     {
    //         IntVec3 neighborLocalCoords = GetNeighborBlockCoords(localCoords, direction);
    //         BlockIterator neighborIter(neighborChunk, neighborLocalCoords);
    //         
    //         if (neighborIter.IsValid())
    //         {
    //             return neighborIter.IsTransparent();
    //         }
    //         else
    //         {
    //             return true;
    //         }
    //     }
    //     else
    //     {
    //         return true;
    //     }
    // }
    //
    // return true;
}

IntVec3 Chunk::GetNeighborBlockCoords(const IntVec3& localCoords, Direction dir)
{
    switch (dir)
    {
    case DIRECTION_EAST:  return IntVec3(0, localCoords.y, localCoords.z);
    case DIRECTION_WEST:  return IntVec3(CHUNK_SIZE_X - 1, localCoords.y, localCoords.z);
    case DIRECTION_NORTH: return IntVec3(localCoords.x, 0, localCoords.z);
    case DIRECTION_SOUTH: return IntVec3(localCoords.x, CHUNK_SIZE_Y - 1, localCoords.z);
    default:
            return localCoords;
    }
}

void Chunk::AddFaceToMesh(const IntVec3& localCoords, const BlockDefinition& blockDef, Direction direction)
{
    BlockIterator iter(this, GetBlockLocalIndexFromLocalCoords(localCoords.x, localCoords.y, localCoords.z));
    BlockIterator neighbor = iter.GetNeighborCrossBoundary(direction);
    
    // 获取邻居的光照值（如果没有邻居，使用默认值）
    uint8_t neighborOutdoorLight = 15;  
    uint8_t neighborIndoorLight = 0;
    
    if (neighbor.IsValid())
    {
        neighborOutdoorLight = neighbor.GetOutdoorLight();
        neighborIndoorLight = neighbor.GetIndoorLight();
    }
    
    float outdoorInfluence = (float)neighborOutdoorLight / 15.0f;
    float indoorInfluence = (float)neighborIndoorLight / 15.0f;
    
    float directionGrayscale = 1.0f;
    switch (direction)
    {
        case DIRECTION_EAST:  directionGrayscale = 0.9f;  break;
        case DIRECTION_WEST:  directionGrayscale = 0.8f;  break;
        case DIRECTION_NORTH: directionGrayscale = 0.85f; break;
        case DIRECTION_SOUTH: directionGrayscale = 0.75f; break;
        case DIRECTION_UP:    directionGrayscale = 1.0f;  break;
        case DIRECTION_DOWN:  directionGrayscale = 0.7f;  break;
    }
    
    Vec3 blockWorldPos(
        (float)(m_chunkCoords.x * CHUNK_SIZE_X + localCoords.x),
        (float)(m_chunkCoords.y * CHUNK_SIZE_Y + localCoords.y),
        (float)localCoords.z
    );
    
    const int* faceIndices = GetFaceIndices(direction);
    size_t startVertIndex = m_vertices.size();
    
    for (int i = 0; i < 4; i++)
    {
        int vertIndex = faceIndices[i];
        Vertex_PCUTBN vert = blockDef.m_verts[vertIndex];
        
        // 更新位置到世界坐标
        vert.m_position += blockWorldPos;
        
        // 设置顶点颜色：R=室外光，G=室内光，B=方向灰度
        vert.m_color = Rgba8(
            (unsigned char)(outdoorInfluence * 255.0f),
            (unsigned char)(indoorInfluence * 255.0f),
            (unsigned char)(directionGrayscale * 255.0f),
            255
        );
        
        m_vertices.push_back(vert);
    }
    
    m_indices.push_back((unsigned int)(startVertIndex + 0));
    m_indices.push_back((unsigned int)(startVertIndex + 1));
    m_indices.push_back((unsigned int)(startVertIndex + 2));
    
    m_indices.push_back((unsigned int)(startVertIndex + 0));
    m_indices.push_back((unsigned int)(startVertIndex + 2));
    m_indices.push_back((unsigned int)(startVertIndex + 3));
}

void Chunk::AddRedstoneWireToMesh(const BlockIterator& block,std::vector<Vertex_PCU>& verts)
{
    Block* wire = block.GetBlock();
    if (!wire || wire->m_typeIndex != BLOCK_TYPE_REDSTONE_WIRE)
        return;
    
    // 获取世界坐标
    Vec3 worldPos = Vec3(
        (float)block.GetGlobalCoords().x,
        (float)block.GetGlobalCoords().y,
        (float)block.GetGlobalCoords().z
    );
    
    // 获取颜色
    uint8_t power = wire->GetRedstonePower();
    Rgba8 color = GetRedstoneWireColor(power);
    
    // 红石线高度偏移（避免z-fighting）
    float wireHeight = 0.02f;
    
    // 获取连接状态
    WireConnections conn = m_world->GetRedstoneSimulator()->GetWireConnections(block);
    
    // 根据连接状态选择纹理
    uint8_t texIndex = conn.GetTextureIndex();
    Vec2 uvMin, uvMax;
    GetRedstoneWireUVs(texIndex, uvMin, uvMax);
    
    // ===== 渲染主体（顶面） =====
    float x0 = worldPos.x;
    float y0 = worldPos.y;
    float z0 = worldPos.z + wireHeight;
    float x1 = worldPos.x + 1.0f;
    float y1 = worldPos.y + 1.0f;
    
    // 顶面四个顶点
    verts.push_back(Vertex_PCU(Vec3(x0, y0, z0), color, Vec2(uvMin.x, uvMin.y)));
    verts.push_back(Vertex_PCU(Vec3(x1, y0, z0), color, Vec2(uvMax.x, uvMin.y)));
    verts.push_back(Vertex_PCU(Vec3(x1, y1, z0), color, Vec2(uvMax.x, uvMax.y)));
    
    verts.push_back(Vertex_PCU(Vec3(x0, y0, z0), color, Vec2(uvMin.x, uvMin.y)));
    verts.push_back(Vertex_PCU(Vec3(x1, y1, z0), color, Vec2(uvMax.x, uvMax.y)));
    verts.push_back(Vertex_PCU(Vec3(x0, y1, z0), color, Vec2(uvMin.x, uvMax.y)));
    
    // ===== 渲染爬升部分 =====
    // 北边向上
    if (conn.north == WireConnection::UP)
    {
        AddWireClimbingFace(verts, worldPos, DIRECTION_NORTH, color, uvMin, uvMax);
    }
    // 南边向上
    if (conn.south == WireConnection::UP)
    {
        AddWireClimbingFace(verts, worldPos, DIRECTION_SOUTH, color, uvMin, uvMax);
    }
    // 东边向上
    if (conn.east == WireConnection::UP)
    {
        AddWireClimbingFace(verts, worldPos, DIRECTION_EAST, color, uvMin, uvMax);
    }
    // 西边向上
    if (conn.west == WireConnection::UP)
    {
        AddWireClimbingFace(verts, worldPos, DIRECTION_WEST, color, uvMin, uvMax);
    }
}

void Chunk::AddWireClimbingFace(std::vector<Vertex_PCU>& verts, const Vec3& basePos, Direction climbDir,
    const Rgba8& color, const Vec2& uvMin, const Vec2& uvMax)
{
    float offset = 0.02f;  // 避免z-fighting
    Vec3 p0, p1, p2, p3;
    
    switch (climbDir)
    {
    case DIRECTION_NORTH:
        p0 = basePos + Vec3(0, 0, offset);
        p1 = basePos + Vec3(1, 0, offset);
        p2 = basePos + Vec3(1, 0, 1 + offset);
        p3 = basePos + Vec3(0, 0, 1 + offset);
        break;
        
    case DIRECTION_SOUTH:
        p0 = basePos + Vec3(0, 1, offset);
        p1 = basePos + Vec3(1, 1, offset);
        p2 = basePos + Vec3(1, 1, 1 + offset);
        p3 = basePos + Vec3(0, 1, 1 + offset);
        break;
        
    case DIRECTION_EAST:
        p0 = basePos + Vec3(1, 0, offset);
        p1 = basePos + Vec3(1, 1, offset);
        p2 = basePos + Vec3(1, 1, 1 + offset);
        p3 = basePos + Vec3(1, 0, 1 + offset);
        break;
        
    case DIRECTION_WEST:
        p0 = basePos + Vec3(0, 0, offset);
        p1 = basePos + Vec3(0, 1, offset);
        p2 = basePos + Vec3(0, 1, 1 + offset);
        p3 = basePos + Vec3(0, 0, 1 + offset);
        break;
        
    default:
        return;
    }
    verts.push_back(Vertex_PCU(p0, color, Vec2(uvMin.x, uvMin.y)));
    verts.push_back(Vertex_PCU(p1, color, Vec2(uvMax.x, uvMin.y)));
    verts.push_back(Vertex_PCU(p2, color, Vec2(uvMax.x, uvMax.y)));
    
    verts.push_back(Vertex_PCU(p0, color, Vec2(uvMin.x, uvMin.y)));
    verts.push_back(Vertex_PCU(p2, color, Vec2(uvMax.x, uvMax.y)));
    verts.push_back(Vertex_PCU(p3, color, Vec2(uvMin.x, uvMax.y)));
}

const int* Chunk::GetFaceIndices(Direction direction)
{
    //in block def
    static const int southFace[4] = {0, 1, 2, 3};   
    static const int eastFace[4]  = {4, 5, 6, 7};   
    static const int northFace[4] = {8, 9, 10, 11}; 
    static const int westFace[4]  = {12, 13, 14, 15};
    static const int upFace[4]    = {16, 17, 18, 19};
    static const int downFace[4]  = {20, 21, 22, 23};

    
    switch (direction)
    {
    case DIRECTION_EAST:  return eastFace;
    case DIRECTION_WEST:  return westFace;
    case DIRECTION_NORTH: return northFace;
    case DIRECTION_SOUTH: return southFace;
    case DIRECTION_UP:    return upFace;
    case DIRECTION_DOWN:  return downFace;
    default: return nullptr;
    }
}

void Chunk::UpdateVBOIBO()
{
    if (m_vertices.empty())
    {
        DebuggerPrintf("Has no vertices!\n");
        return;
    }
       
    if (m_vertexBuffer)
    {
        delete m_vertexBuffer;
        m_vertexBuffer = nullptr;
    }
    if (m_indexBuffer)
    {
        delete m_indexBuffer;
        m_indexBuffer = nullptr;
    }

    m_vertexBuffer = g_theRenderer->CreateVertexBuffer((unsigned int)(m_vertices.size() * sizeof(Vertex_PCUTBN)),
                                                       sizeof(Vertex_PCUTBN));
    m_indexBuffer = g_theRenderer->CreateIndexBuffer((unsigned int)(m_indices.size() * sizeof(unsigned int)),
                                                     sizeof(unsigned int));
    
    g_theRenderer->CopyCPUToGPU(m_vertices.data(),(unsigned int)(m_vertices.size() * sizeof(Vertex_PCUTBN)),m_vertexBuffer);
    g_theRenderer->CopyCPUToGPU(m_indices.data(), (unsigned int)(m_indices.size() * sizeof(unsigned int)),m_indexBuffer);
}

bool Chunk::AreAllNeighborsActive() const
{
    return (m_eastNeighbor && m_eastNeighbor->GetState() == ChunkState::ACTIVE) &&
          (m_westNeighbor && m_westNeighbor->GetState() == ChunkState::ACTIVE) &&
          (m_northNeighbor && m_northNeighbor->GetState() == ChunkState::ACTIVE) &&
          (m_southNeighbor && m_southNeighbor->GetState() == ChunkState::ACTIVE);
}

void Chunk::ReportDirty()
{
    m_world->m_hasDirtyChunk = true;
}
