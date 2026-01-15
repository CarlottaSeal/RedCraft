#include "Chunk.h"

#include <algorithm>

#include "ChunkUtils.h"
#include "Game.hpp"
#include "Player.hpp"
#include "World.h"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/IntVec3.h"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Save/SaveSystem.h"
#include "Gameplay/RedstoneSimulator.h"
#include "Generator/WorldGenPipeline.h"
#include "ThirdParty/Noise/SmoothNoise.hpp"
#include "ThirdParty/Noise/RawNoise.hpp"
#include "UI/GameUIManager.h"

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
    if (m_transparentVertexBuffer != nullptr)
    {
        delete m_transparentVertexBuffer;
        m_transparentVertexBuffer = nullptr;
    }
    if (m_transparentIndexBuffer != nullptr)
    {
        delete m_transparentIndexBuffer;
        m_transparentIndexBuffer = nullptr;
    }
    if (m_sortedTransparentIndexBuffer != nullptr)
    {
        delete m_sortedTransparentIndexBuffer;
        m_sortedTransparentIndexBuffer = nullptr;
    }
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
        IntVec3 targetLocal = FindDigTarget(g_theGame->m_player->m_position);
        if (targetLocal.z >= 0)
        {
            BlockIterator targetIter(this, targetLocal);
            if (targetIter.IsValid())
            {
                //Block* targetBlock = targetIter.GetBlock();
                
                // 检查是否是可交互的红石组件
                if (HandleBlockInteraction(targetIter))
                {
                    return;  
                }
            }
        }
        
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
        //block->m_typeIndex != BLOCK_TYPE_WATER && TODO:不允许水
        block->m_typeIndex != BLOCK_TYPE_LAVA)
        return;
    
    if (!HasSupport(localCoords, blockType))
    {
        return; 
    }

    bool wasSky = block->IsSky();
    block->SetType(blockType);

    Direction playerFacing = m_world->m_owner->m_player->GetOrthoDirection();
    if (blockType == BLOCK_TYPE_REDSTONE_PISTON ||
        blockType == BLOCK_TYPE_REDSTONE_STICKY_PISTON ||
        blockType == BLOCK_TYPE_REDSTONE_OBSERVER )
    {
        // 这些方块：朝向玩家看的方向（6方向）
        block->SetBlockFacing((uint8_t)playerFacing);
    }
    else if (blockType == BLOCK_TYPE_REPEATER_OFF ||
             blockType == BLOCK_TYPE_REPEATER_ON)
    {
        // 中继器：信号输出方向 = 玩家看的方向（只用水平4方向）
        Direction horizontal = m_world->m_owner->m_player->GetHorizontalDirection();
        block->SetBlockFacing((uint8_t)horizontal);
        block->SetRepeaterDelay(1);  // 默认1档延迟
    }
    // else if (blockType == BLOCK_TYPE_COMPARATOR_OFF ||
    //          blockType == BLOCK_TYPE_COMPARATOR_ON)
    // {
    //     // 比较器：同中继器
    //     Direction horizontal = GetHorizontalDirection(playerFacing);
    //     block->SetBlockFacing((uint8_t)horizontal);
    //     block->SetComparatorMode(0);  // 默认比较模式
    // }
    else if (blockType == BLOCK_TYPE_REDSTONE_LEVER)
    {
        // 拉杆：附着在地面，朝上
        block->SetBlockFacing((uint8_t)DIRECTION_UP);
        block->SetLeverState(false);
    }
    else if (blockType == BLOCK_TYPE_BUTTON_STONE)
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
        BlockIterator globalIter = m_world->GetBlockIterator(iter.GetGlobalCoords());
        m_world->m_redstoneSimulator->UpdateNeighborWireConnections(globalIter);
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
    if (!m_serializer)
        m_serializer = new ChunkSerializer(this);

    // for (int i = 0; i < CHUNK_TOTAL_BLOCKS; i++)
    // {
    //     m_serializer->m_blockData[i] = m_blocks[i].m_typeIndex;
    // }
    memcpy(m_serializer->m_blockData, m_blocks, CHUNK_TOTAL_BLOCKS * sizeof(Block));
    if (m_world)
    {
        std::string worldSaveDir = m_world->GetSaveSubdirectory();
        g_theSaveSystem->PushSaveContext(worldSaveDir);
    }
    g_theSaveSystem->Save(MakeChunkFilename(m_chunkCoords), m_serializer, SaveFormat::BINARY);
    if (m_world)
    {
        g_theSaveSystem->PopSaveContext();
    }
    m_needsSaving = false;
}

bool Chunk::Load()
{
    if (!m_serializer)
        m_serializer = new ChunkSerializer(this);
    
    const std::string fn = MakeChunkFilename(m_chunkCoords);
    if (m_world)
    {
        std::string worldSaveDir = m_world->GetSaveSubdirectory();
        g_theSaveSystem->PushSaveContext(worldSaveDir);
    }
    if (!g_theSaveSystem->FileExists(fn))
    {
        if (m_world)
            g_theSaveSystem->PopSaveContext();
        return false;
    }

    bool success = g_theSaveSystem->Load(fn, m_serializer, SaveFormat::BINARY);
    if (m_world)
    {
        g_theSaveSystem->PopSaveContext();
    }
    if (!success)
        return false;

    //解压结果回写到 Chunk 的方块数组
    size_t totalBytes = CHUNK_TOTAL_BLOCKS * sizeof(Block);
    memcpy(m_blocks, m_serializer->m_blockData, totalBytes);

    RefillWaterAboveUnderwaterPlants();

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

void Chunk::RenderOpaque() const
{
    if (m_vertexBuffer && m_indexBuffer && !m_vertices.empty())
    {
        // g_theRenderer->BindShader(m_world->m_worldShader);
        // g_theRenderer->SetModelConstants();
        // g_theRenderer->BindTexture(&m_world->m_owner->m_spriteSheet->GetTexture());
        g_theRenderer->BindVertexBuffer(m_vertexBuffer);
        g_theRenderer->BindIndexBuffer(m_indexBuffer);
        g_theRenderer->DrawIndexBuffer(m_vertexBuffer, m_indexBuffer, (unsigned int)m_indices.size());
    }
}

void Chunk::RenderTransparent(const Vec3& cameraPos) const
{
    if (m_transparentVertexBuffer == nullptr || 
        m_transparentIndexBuffer == nullptr || 
        m_transparentIndices.empty())
    {
        return;
    }
    // 计算透明面片数量
    size_t numFaces = m_transparentIndices.size() / 6;  // 每个四边形 = 6 个索引
    // 安全检查：如果面片中心数量不匹配，降级到无排序渲染
    if (m_transparentFaceCenters.size() != numFaces)
    {
        // 直接渲染，不排序
        g_theRenderer->BindVertexBuffer(m_transparentVertexBuffer);
        g_theRenderer->BindIndexBuffer(m_transparentIndexBuffer);
        g_theRenderer->DrawIndexBuffer(m_transparentVertexBuffer, 
                                       m_transparentIndexBuffer, 
                                       (unsigned int)m_transparentIndices.size());
        return;
    }
    // 【排序逻辑】
    // 创建面片索引数组
    std::vector<size_t> faceOrder(numFaces);
    for (size_t i = 0; i < numFaces; i++)
    {
        faceOrder[i] = i;
    }
    // 按照距离排序（从远到近）
    std::sort(faceOrder.begin(), faceOrder.end(), 
        [this, &cameraPos](size_t idxA, size_t idxB) -> bool 
        {
            float distSqA = (m_transparentFaceCenters[idxA] - cameraPos).GetLengthSquared();
            float distSqB = (m_transparentFaceCenters[idxB] - cameraPos).GetLengthSquared();
            return distSqA > distSqB;  // 从远到近
        });
    // 根据排序后的顺序重建索引数组
    m_sortedTransparentIndices.clear();
    m_sortedTransparentIndices.reserve(m_transparentIndices.size());
    for (size_t faceIdx : faceOrder)
    {
        // 每个四边形有 6 个索引
        size_t baseIdx = faceIdx * 6;
        for (size_t i = 0; i < 6; i++)
        {
            m_sortedTransparentIndices.push_back(m_transparentIndices[baseIdx + i]);
        }
    }
    // 创建或更新排序后的索引缓冲
    if (m_sortedTransparentIndexBuffer == nullptr)
    {
        m_sortedTransparentIndexBuffer = g_theRenderer->CreateIndexBuffer(
            (unsigned int)(m_sortedTransparentIndices.size() * sizeof(unsigned int)),
            sizeof(unsigned int));
    }
    // 上传排序后的索引到 GPU
    g_theRenderer->CopyCPUToGPU(
        m_sortedTransparentIndices.data(),
        (unsigned int)(m_sortedTransparentIndices.size() * sizeof(unsigned int)),
        m_sortedTransparentIndexBuffer);
    // 使用排序后的索引缓冲渲染
    g_theRenderer->BindVertexBuffer(m_transparentVertexBuffer);
    g_theRenderer->BindIndexBuffer(m_sortedTransparentIndexBuffer);
    g_theRenderer->DrawIndexBuffer(m_transparentVertexBuffer, 
                                   m_sortedTransparentIndexBuffer,
                                   (unsigned int)m_sortedTransparentIndices.size());
}

void Chunk::GenerateBlocks()
{
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
        return false;
    }
    
    m_vertices.clear();
    m_indices.clear();
    m_transparentVertices.clear();
    m_transparentIndices.clear();
    m_transparentFaces.clear();
    m_transparentFaceCenters.clear();  

    BlockIterator iter(this, 0);
    for (int blockIndex = 0; blockIndex < CHUNK_TOTAL_BLOCKS; blockIndex++)
    {
        iter = BlockIterator(this, blockIndex);
        if (!iter.IsValid() || iter.GetBlockType() == BLOCK_TYPE_AIR)
            continue;
        
        const BlockDefinition& blockDef = BlockDefinition::GetBlockDef(iter.GetBlockType());
        if (!blockDef.m_isVisible)
            continue;

        bool isTransparent = blockDef.m_isTransparent;
        std::vector<Vertex_PCUTBN>& targetVerts = isTransparent ? m_transparentVertices : m_vertices;
        std::vector<unsigned int>& targetIndices = isTransparent ? m_transparentIndices : m_indices;
        
        if (IsPlantBillboard(iter.GetBlockType()))
        {
            AddCropToMesh(iter, targetVerts, targetIndices);
            continue;
        }
        if (NeedsSpecialRenderingAsRedstoneComp(iter.GetBlockType()))
        {
            AddRedstoneComponentToMesh(iter, m_vertices, m_indices);
            continue; 
        }
        for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
        {
            Direction direction = (Direction)dir;
            
            if (ShouldRenderFace(iter, direction))
            {
                // 记录添加面片前的顶点数量
                size_t vertCountBefore = targetVerts.size();
                // 添加面片
                AddFaceToMesh(iter.GetLocalCoords(), blockDef, direction, targetVerts, targetIndices);
                // 如果是透明面片，计算并记录中心位置
                if (isTransparent)
                {
                    // 计算新添加的顶点的平均位置
                    Vec3 faceCenter = Vec3();
                    size_t vertCountAfter = targetVerts.size();
                    
                    for (size_t i = vertCountBefore; i < vertCountAfter; i++)
                    {
                        faceCenter += targetVerts[i].m_position;
                    }
                    
                    float numNewVerts = (float)(vertCountAfter - vertCountBefore);
                    if (numNewVerts > 0.0f)
                    {
                        faceCenter /= numNewVerts;
                        m_transparentFaceCenters.push_back(faceCenter);
                    }
                }
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
    {
        IntVec3 localCoords = iter.GetLocalCoords();
        bool onBoundary = IsOnBoundary(localCoords, direction);
        if (onBoundary && !GetNeighbor(direction))
            return false;  // Farm 边界，不渲染
        return true;
    }
    
    Block* neighborBlock = neighbor.GetBlock();
    if (!neighborBlock)
        return true;
    if (neighborBlock->m_typeIndex == BLOCK_TYPE_AIR)
        return true;
    
    if (neighborBlock->IsOpaque())
        return false;
    
    uint8_t currentType = iter.GetBlockType();
    uint8_t neighborType = neighborBlock->m_typeIndex;
    
    if (currentType == neighborType)
        return false;
    if (IsUnderwaterProduct(currentType) && neighborType == BLOCK_TYPE_WATER)
        return false;
    if (currentType == BLOCK_TYPE_WATER && IsUnderwaterProduct(neighborType))
        return false;
    
    return true;
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

void Chunk::AddFaceToMesh(const IntVec3& localCoords, const BlockDefinition& blockDef, Direction direction,
                          std::vector<Vertex_PCUTBN>& targetVerts, std::vector<unsigned int>& targetIndices)
{
    BlockIterator iter(this, GetBlockLocalIndexFromLocalCoords(localCoords.x, localCoords.y, localCoords.z));
    BlockIterator neighbor = iter.GetNeighborCrossBoundary(direction);
    
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
    size_t startVertIndex = targetVerts.size(); 
    
    for (int i = 0; i < 4; i++)
    {
        int vertIndex = faceIndices[i];
        Vertex_PCUTBN vert = blockDef.m_verts[vertIndex];
        
        vert.m_position += blockWorldPos;

        vert.m_color = Rgba8(
    (unsigned char)(outdoorInfluence * 255.0f),      
    (unsigned char)(indoorInfluence * 255.0f),       
    (unsigned char)(directionGrayscale * 255.0f),    
    255
);
        targetVerts.push_back(vert); 
    }
    targetIndices.push_back((unsigned int)(startVertIndex + 0)); 
    targetIndices.push_back((unsigned int)(startVertIndex + 1)); 
    targetIndices.push_back((unsigned int)(startVertIndex + 2)); 
    targetIndices.push_back((unsigned int)(startVertIndex + 0));
    targetIndices.push_back((unsigned int)(startVertIndex + 2)); 
    targetIndices.push_back((unsigned int)(startVertIndex + 3)); 
}

bool Chunk::IsBlockTransparent(uint8_t blockType) const
{
    const BlockDefinition& def = BlockDefinition::GetBlockDef(blockType);
    return def.m_isTransparent;
}

void Chunk::AddRedstoneWireToMesh(const BlockIterator& iter)
{
    Block* wire = iter.GetBlock();
    if (!wire || !IsRedstoneWire(wire->m_typeIndex))
        return;
    
    IntVec3 globalCoords = iter.GetGlobalCoords();
    Vec3 worldPos((float)globalCoords.x, (float)globalCoords.y, (float)globalCoords.z);
    
    uint8_t power = wire->GetRedstonePower();
    Rgba8 color = GetRedstoneWireColor(power);
    
    const BlockDefinition& def = BlockDefinition::GetBlockDef(wire->m_typeIndex);
    AABB2 uvs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_topSpriteCoords);
    
    float wireHeight = 0.02f;
    
    Vec3 p0 = worldPos + Vec3(0.0f, 0.0f, wireHeight);
    Vec3 p1 = worldPos + Vec3(1.0f, 0.0f, wireHeight);
    Vec3 p2 = worldPos + Vec3(1.0f, 1.0f, wireHeight);
    Vec3 p3 = worldPos + Vec3(0.0f, 1.0f, wireHeight);
    if (wire->m_typeIndex == BLOCK_TYPE_REDSTONE_WIRE_CORNER)
    {
        // 0: NE, 1: SE, 2: SW, 3: NW（外角方向）:contentReference[oaicite:2]{index=2}
        // 假设 sprite 画的是 “NE 角”（即 facing == 0 的情况），
        // 那么 facing=1/2/3 时分别顺时针旋转 90/180/270 度。
        uint8_t facing = wire->GetBlockFacing();
        int rotateTimes = static_cast<int>(facing) & 0x03;  // 0~3
        RotateQuadAroundCenterCW90(p0, p1, p2, p3, rotateTimes);
    }
    AddVertsForIndexQuad3D(m_vertices, m_indices, p0, p1, p2, p3, color, uvs);
}

void Chunk::AddRedstoneTorchToMesh(const BlockIterator& iter)
{
    Block* torch = iter.GetBlock();
    if (!torch) return;
    
    IntVec3 globalCoords = iter.GetGlobalCoords();
    Vec3 worldPos((float)globalCoords.x, (float)globalCoords.y, (float)globalCoords.z);
    
    const BlockDefinition& def = BlockDefinition::GetBlockDef(torch->m_typeIndex);
    AABB2 uvs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_sideSpriteCoords);
    
    Rgba8 color = Rgba8::WHITE;
    
    float torchWidth = 8.0f / 16.0f;
    float torchHeight = 12.0f / 16.0f;
    float halfW = torchWidth / 2.0f;
    
    Direction facing = (Direction)torch->GetBlockFacing();
    Vec3 center = worldPos + Vec3(0.5f, 0.5f, 0.0f);
    
    // 定义局部坐标系下的火把顶点（朝上时）
    Vec3 localVerts[8];
    localVerts[0] = Vec3(-halfW, 0, 0);          // X面底左
    localVerts[1] = Vec3(halfW, 0, 0);           // X面底右
    localVerts[2] = Vec3(halfW, 0, torchHeight); // X面顶右
    localVerts[3] = Vec3(-halfW, 0, torchHeight);// X面顶左
    localVerts[4] = Vec3(0, -halfW, 0);          // Y面底左
    localVerts[5] = Vec3(0, halfW, 0);           // Y面底右
    localVerts[6] = Vec3(0, halfW, torchHeight); // Y面顶右
    localVerts[7] = Vec3(0, -halfW, torchHeight);// Y面顶左
    
    // 根据朝向变换顶点
    if (facing != DIRECTION_UP)
    {
        float tiltAngle = 22.0f; // 倾斜角度（度）
        Vec3 offset;
        
        switch (facing)
        {
        case DIRECTION_NORTH: 
            offset = Vec3(0.0f, 0.35f, 0.15f);
            break;
        case DIRECTION_SOUTH: 
            offset = Vec3(0.0f, -0.35f, 0.15f);
            tiltAngle = -tiltAngle;
            break;
        case DIRECTION_EAST:  
            offset = Vec3(-0.35f, 0.0f, 0.15f);
            break;
        case DIRECTION_WEST:  
            offset = Vec3(0.35f, 0.0f, 0.15f);
            tiltAngle = -tiltAngle;
            break;
        default: 
            offset = Vec3(0, 0, 0);
            break;
        }
        
        center += offset;
        
        // 应用倾斜旋转
        float radians = tiltAngle * (3.14159265f / 180.0f);
        float cosA = cosf(radians);
        float sinA = sinf(radians);
        
        for (int i = 0; i < 8; i++)
        {
            Vec3 v = localVerts[i];
            
            // 根据附着方向选择旋转轴
            if (facing == DIRECTION_NORTH || facing == DIRECTION_SOUTH)
            {
                // 绕X轴旋转（向北或向南倾斜）
                float newY = v.y * cosA - v.z * sinA;
                float newZ = v.y * sinA + v.z * cosA;
                localVerts[i] = Vec3(v.x, newY, newZ);
            }
            else if (facing == DIRECTION_EAST || facing == DIRECTION_WEST)
            {
                // 绕Y轴旋转（向东或向西倾斜）
                float newX = v.x * cosA + v.z * sinA;
                float newZ = -v.x * sinA + v.z * cosA;
                localVerts[i] = Vec3(newX, v.y, newZ);
            }
        }
    }
    
    // 添加X方向面（双面）
    AddVertsForIndexQuad3D(m_vertices, m_indices,
        center + localVerts[0], center + localVerts[1],
        center + localVerts[2], center + localVerts[3],
        color, uvs);
    AddVertsForIndexQuad3D(m_vertices, m_indices,
        center + localVerts[1], center + localVerts[0],
        center + localVerts[3], center + localVerts[2],
        color, uvs);
    
    // 添加Y方向面（双面）
    AddVertsForIndexQuad3D(m_vertices, m_indices,
        center + localVerts[4], center + localVerts[5],
        center + localVerts[6], center + localVerts[7],
        color, uvs);
    AddVertsForIndexQuad3D(m_vertices, m_indices,
        center + localVerts[5], center + localVerts[4],
        center + localVerts[7], center + localVerts[6],
        color, uvs);
}

void Chunk::AddRepeaterToMesh(const BlockIterator& iter)
{
    Block* repeater = iter.GetBlock();
    if (!repeater) return;
    
    IntVec3 globalCoords = iter.GetGlobalCoords();
    Vec3 worldPos((float)globalCoords.x, (float)globalCoords.y, (float)globalCoords.z);
    
    const BlockDefinition& def = BlockDefinition::GetBlockDef(repeater->m_typeIndex);
    AABB2 topUVs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_topSpriteCoords);
    AABB2 sideUVs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_sideSpriteCoords);
    
    Rgba8 color = Rgba8::WHITE;
    float baseHeight = 2.0f / 16.0f;
    
    // 获取朝向
    Direction facing = (Direction)repeater->GetBlockFacing();
    
    // 计算旋转角度（以度为单位）
    float yawDegrees = 0.0f;
    switch (facing)
    {
    case DIRECTION_NORTH: yawDegrees = 0.0f;   break;
    case DIRECTION_SOUTH: yawDegrees = 180.0f; break;
    case DIRECTION_EAST:  yawDegrees = 270.0f; break;
    case DIRECTION_WEST:  yawDegrees = 90.0f;  break;
    default: yawDegrees = 0.0f; break;
    }
    
    float yawRadians = yawDegrees * (3.14159265f / 180.0f);
    float cosYaw = cosf(yawRadians);
    float sinYaw = sinf(yawRadians);
    Vec3 blockCenter = worldPos + Vec3(0.5f, 0.5f, 0.0f);
    
    // Lambda函数：旋转点绕Z轴（垂直轴）
    auto rotatePoint = [&](const Vec3& point) -> Vec3 {
        Vec3 relative = point - blockCenter;
        float newX = relative.x * cosYaw - relative.y * sinYaw;
        float newY = relative.x * sinYaw + relative.y * cosYaw;
        return blockCenter + Vec3(newX, newY, relative.z);
    };
    
    // 底座（平板）
    Vec3 baseCorners[8];
    baseCorners[0] = worldPos + Vec3(0, 0, 0);
    baseCorners[1] = worldPos + Vec3(1, 0, 0);
    baseCorners[2] = worldPos + Vec3(1, 1, 0);
    baseCorners[3] = worldPos + Vec3(0, 1, 0);
    baseCorners[4] = worldPos + Vec3(0, 0, baseHeight);
    baseCorners[5] = worldPos + Vec3(1, 0, baseHeight);
    baseCorners[6] = worldPos + Vec3(1, 1, baseHeight);
    baseCorners[7] = worldPos + Vec3(0, 1, baseHeight);
    
    // 旋转所有角点
    for (int i = 0; i < 8; i++)
    {
        baseCorners[i] = rotatePoint(baseCorners[i]);
    }
    
    // 底座顶面
    AddVertsForIndexQuad3D(m_vertices, m_indices,
        baseCorners[4], baseCorners[5], baseCorners[6], baseCorners[7],
        color, topUVs);
    
    // 底座四个侧面
    AddVertsForIndexQuad3D(m_vertices, m_indices,
        baseCorners[0], baseCorners[1], baseCorners[5], baseCorners[4],
        color, sideUVs);
    AddVertsForIndexQuad3D(m_vertices, m_indices,
        baseCorners[2], baseCorners[3], baseCorners[7], baseCorners[6],
        color, sideUVs);
    AddVertsForIndexQuad3D(m_vertices, m_indices,
        baseCorners[3], baseCorners[0], baseCorners[4], baseCorners[7],
        color, sideUVs);
    AddVertsForIndexQuad3D(m_vertices, m_indices,
        baseCorners[1], baseCorners[2], baseCorners[6], baseCorners[5],
        color, sideUVs);
    
    bool isOn = (repeater->m_typeIndex == BLOCK_TYPE_REPEATER_ON);
    uint8_t torchType = isOn ? BLOCK_TYPE_REDSTONE_TORCH : BLOCK_TYPE_REDSTONE_TORCH_OFF;
    const BlockDefinition& torchDef = BlockDefinition::GetBlockDef(torchType);
    AABB2 torchUVs = g_theGame->m_spriteSheet->GetSpriteUVs(torchDef.m_sideSpriteCoords);
    
    float torchWidth = 8.0f / 16.0f;
    float torchHeight = 10.0f / 16.0f;
    float halfW = torchWidth / 2.0f;
    
    uint8_t delay = repeater->GetRepeaterDelay();
    
    // 前火把位置（输出端，靠近前方）
    float frontY = 3.0f / 16.0f;
    // 后火把位置（输入端，随delay变化）
    float backY = 11.0f / 16.0f - (delay - 1) * 2.0f / 16.0f;
    
    auto addCrossTorch = [&](float yPos) {
        // 火把中心（未旋转的局部坐标）
        Vec3 localCenter = worldPos + Vec3(0.5f, yPos, baseHeight + torchHeight / 2.0f);
        Vec3 torchCenter = rotatePoint(localCenter);
        
        // 定义局部顶点（相对于火把中心）
        Vec3 localVerts[8];
        localVerts[0] = Vec3(-halfW, 0, -torchHeight/2);  // X面底左
        localVerts[1] = Vec3(halfW, 0, -torchHeight/2);   // X面底右
        localVerts[2] = Vec3(halfW, 0, torchHeight/2);    // X面顶右
        localVerts[3] = Vec3(-halfW, 0, torchHeight/2);   // X面顶左
        localVerts[4] = Vec3(0, -halfW, -torchHeight/2);  // Y面底左
        localVerts[5] = Vec3(0, halfW, -torchHeight/2);   // Y面底右
        localVerts[6] = Vec3(0, halfW, torchHeight/2);    // Y面顶右
        localVerts[7] = Vec3(0, -halfW, torchHeight/2);   // Y面顶左
        
        // 旋转每个顶点（绕Z轴）
        for (int i = 0; i < 8; i++)
        {
            float x = localVerts[i].x;
            float y = localVerts[i].y;
            float newX = x * cosYaw - y * sinYaw;
            float newY = x * sinYaw + y * cosYaw;
            localVerts[i] = Vec3(newX, newY, localVerts[i].z);
        }
        
        // 添加X方向面（双面）
        AddVertsForIndexQuad3D(m_vertices, m_indices,
            torchCenter + localVerts[0], torchCenter + localVerts[1],
            torchCenter + localVerts[2], torchCenter + localVerts[3],
            color, torchUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices,
            torchCenter + localVerts[1], torchCenter + localVerts[0],
            torchCenter + localVerts[3], torchCenter + localVerts[2],
            color, torchUVs);
        
        // 添加Y方向面（双面）
        AddVertsForIndexQuad3D(m_vertices, m_indices,
            torchCenter + localVerts[4], torchCenter + localVerts[5],
            torchCenter + localVerts[6], torchCenter + localVerts[7],
            color, torchUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices,
            torchCenter + localVerts[5], torchCenter + localVerts[4],
            torchCenter + localVerts[7], torchCenter + localVerts[6],
            color, torchUVs);
    };
    addCrossTorch(frontY);
    addCrossTorch(backY);
}

void Chunk::AddLeverToMesh(const BlockIterator& iter)
{
    Block* lever = iter.GetBlock();
    if (!lever) return;
    
    IntVec3 globalCoords = iter.GetGlobalCoords();
    Vec3 worldPos((float)globalCoords.x, (float)globalCoords.y, (float)globalCoords.z);
    
    const BlockDefinition& def = BlockDefinition::GetBlockDef(lever->m_typeIndex);
    AABB2 baseUVs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_bottomSpriteCoords);
    AABB2 leverUVs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_sideSpriteCoords);
    
    Rgba8 color = Rgba8::WHITE;
    bool isOn = lever->GetSpecialState();
    
    // 底座
    float bw = 10.0f / 16.0f, bd = 8.0f / 16.0f, bh = 6.0f / 16.0f;
    Vec3 baseCenter = worldPos + Vec3(0.5f, 0.5f, 0.0f);
    AABB3 base(baseCenter + Vec3(-bw/2, -bd/2, 0), baseCenter + Vec3(bw/2, bd/2, bh));
    AddVertsForIndexAABB3D(m_vertices, m_indices, base, color, baseUVs);
    
    // 拉杆柄 (倾斜)
    float lw = 8.0f / 16.0f, ll = 8.0f / 16.0f;
    float halfW = lw / 2.0f;
    Vec3 leverBase = baseCenter + Vec3(0, 0, bh);
    float tilt = ll * 0.7f;
    Vec3 leverEnd = isOn ? (leverBase + Vec3(0, tilt, tilt)) : (leverBase + Vec3(0, -tilt, tilt));
    
    // 两个面 (双面渲染)
    AddVertsForIndexQuad3D(m_vertices, m_indices,
        leverBase + Vec3(-halfW, 0, 0), leverBase + Vec3(halfW, 0, 0),
        leverEnd + Vec3(halfW, 0, 0), leverEnd + Vec3(-halfW, 0, 0),
        color, leverUVs);
    AddVertsForIndexQuad3D(m_vertices, m_indices,
        leverBase + Vec3(halfW, 0, 0), leverBase + Vec3(-halfW, 0, 0),
        leverEnd + Vec3(-halfW, 0, 0), leverEnd + Vec3(halfW, 0, 0),
        color, leverUVs);
}

void Chunk::AddButtonToMesh(const BlockIterator& iter)
{
    Block* button = iter.GetBlock();
    if (!button) return;
    
    IntVec3 globalCoords = iter.GetGlobalCoords();
    Vec3 worldPos((float)globalCoords.x, (float)globalCoords.y, (float)globalCoords.z);
    
    const BlockDefinition& def = BlockDefinition::GetBlockDef(button->m_typeIndex);
    AABB2 uvs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_sideSpriteCoords);
    Rgba8 color = Rgba8::WHITE;
    bool isPressed = button->GetSpecialState();
    Direction facing = (Direction)button->GetBlockFacing();
    float bw = 6.0f / 16.0f, bh = 4.0f / 16.0f;
    float bd = isPressed ? 1.0f / 16.0f : 2.0f / 16.0f;
    Vec3 mins, maxs;
    switch (facing)
    {
    case DIRECTION_UP:
        mins = worldPos + Vec3(0.5f - bw/2, 0.5f - bh/2, 0);
        maxs = mins + Vec3(bw, bh, bd);
        break;
    case DIRECTION_DOWN:
        maxs = worldPos + Vec3(0.5f + bw/2, 0.5f + bh/2, 1);
        mins = maxs - Vec3(bw, bh, bd);
        break;
    case DIRECTION_NORTH:
        mins = worldPos + Vec3(0.5f - bw/2, 1 - bd, 0.5f - bh/2);
        maxs = mins + Vec3(bw, bd, bh);
        break;
    case DIRECTION_SOUTH:
        mins = worldPos + Vec3(0.5f - bw/2, 0, 0.5f - bh/2);
        maxs = mins + Vec3(bw, bd, bh);
        break;
    case DIRECTION_EAST:
        mins = worldPos + Vec3(1 - bd, 0.5f - bw/2, 0.5f - bh/2);
        maxs = mins + Vec3(bd, bw, bh);
        break;
    case DIRECTION_WEST:
        mins = worldPos + Vec3(0, 0.5f - bw/2, 0.5f - bh/2);
        maxs = mins + Vec3(bd, bw, bh);
        break;
    default:
        mins = worldPos + Vec3(0.5f - bw/2, 0.5f - bh/2, 0);
        maxs = mins + Vec3(bw, bh, bd);
    }
    AddVertsForIndexAABB3D(m_vertices, m_indices, AABB3(mins, maxs), color, uvs);
}

void Chunk::AddPistonToMesh(const BlockIterator& iter)
{
    Block* piston = iter.GetBlock();
    if (!piston) return;
    
    IntVec3 globalCoords = iter.GetGlobalCoords();
    Vec3 pos((float)globalCoords.x, (float)globalCoords.y, (float)globalCoords.z);
    
    const BlockDefinition& def = BlockDefinition::GetBlockDef(piston->m_typeIndex);
    
    AABB2 topUVs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_topSpriteCoords);
    AABB2 bottomUVs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_bottomSpriteCoords);
    AABB2 sideUVs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_sideSpriteCoords);
    
    Rgba8 color = Rgba8::WHITE;
    
    // 获取活塞朝向
    Direction facing = (Direction)piston->GetBlockFacing();
    
    // 8个顶点
    Vec3 p000 = pos;
    Vec3 p100 = pos + Vec3(1, 0, 0);
    Vec3 p010 = pos + Vec3(0, 1, 0);
    Vec3 p110 = pos + Vec3(1, 1, 0);
    Vec3 p001 = pos + Vec3(0, 0, 1);
    Vec3 p101 = pos + Vec3(1, 0, 1);
    Vec3 p011 = pos + Vec3(0, 1, 1);
    Vec3 p111 = pos + Vec3(1, 1, 1);
    
    // 根据朝向渲染6个面 (topUVs = 活塞头)
    switch (facing)
    {
    case DIRECTION_NORTH:  // 活塞头朝 +Y
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, topUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, bottomUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, sideUVs);
        break;
        
    case DIRECTION_SOUTH:  // 活塞头朝 -Y
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, topUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, bottomUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, sideUVs);
        break;
        
    case DIRECTION_EAST:   // 活塞头朝 +X
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, topUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, bottomUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, sideUVs);
        break;
        
    case DIRECTION_WEST:   // 活塞头朝 -X
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, topUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, bottomUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, sideUVs);
        break;
        
    case DIRECTION_UP:     // 活塞头朝 +Z
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, topUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, bottomUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, sideUVs);
        break;
        
    case DIRECTION_DOWN:   // 活塞头朝 -Z
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, topUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, bottomUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, sideUVs);
        break;
        
    default:
        // 默认朝北
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, topUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, bottomUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, sideUVs);
    }
}

void Chunk::AddPistonHeadToMesh(const BlockIterator& iter)
{
    Block* head = iter.GetBlock();
    if (!head) return;

    IntVec3 globalCoords = iter.GetGlobalCoords();
    Vec3 worldPos((float)globalCoords.x, (float)globalCoords.y, (float)globalCoords.z);

    const BlockDefinition& def = BlockDefinition::GetBlockDef(head->m_typeIndex);
    AABB2 topUVs  = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_topSpriteCoords);
    AABB2 sideUVs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_sideSpriteCoords);

    Rgba8 color = Rgba8::WHITE;

    float headThick = 4.0f / 16.0f;   // 活塞头厚度
    float armW      = 4.0f / 16.0f;   // 中间那根小方柱的宽度
    float armOff    = (1.0f - armW) * 0.5f;

    Direction facing = (Direction)head->GetBlockFacing();

    AABB3 headBox;
    AABB3 armBox;

    switch (facing)
    {
    case DIRECTION_UP: // 头在方块上面，+Z
        headBox = AABB3(
            worldPos + Vec3(0.f, 0.f, 1.f - headThick),
            worldPos + Vec3(1.f, 1.f, 1.f));
        armBox = AABB3(
            worldPos + Vec3(armOff, armOff, 0.f),
            worldPos + Vec3(armOff + armW, armOff + armW, 1.f - headThick));
        break;

    case DIRECTION_DOWN: // 头在下面，-Z
        headBox = AABB3(
            worldPos + Vec3(0.f, 0.f, 0.f),
            worldPos + Vec3(1.f, 1.f, headThick));
        armBox = AABB3(
            worldPos + Vec3(armOff, armOff, headThick),
            worldPos + Vec3(armOff + armW, armOff + armW, 1.f));
        break;

    case DIRECTION_NORTH: // +Y
        headBox = AABB3(
            worldPos + Vec3(0.f, 1.f - headThick, 0.f),
            worldPos + Vec3(1.f, 1.f, 1.f));
        armBox = AABB3(
            worldPos + Vec3(armOff, 0.f, armOff),
            worldPos + Vec3(armOff + armW, 1.f - headThick, armOff + armW));
        break;

    case DIRECTION_SOUTH: // -Y
        headBox = AABB3(
            worldPos + Vec3(0.f, 0.f, 0.f),
            worldPos + Vec3(1.f, headThick, 1.f));
        armBox = AABB3(
            worldPos + Vec3(armOff, headThick, armOff),
            worldPos + Vec3(armOff + armW, 1.f, armOff + armW));
        break;

    case DIRECTION_EAST: // +X
        headBox = AABB3(
            worldPos + Vec3(1.f - headThick, 0.f, 0.f),
            worldPos + Vec3(1.f, 1.f, 1.f));
        armBox = AABB3(
            worldPos + Vec3(0.f, armOff, armOff),
            worldPos + Vec3(1.f - headThick, armOff + armW, armOff + armW));
        break;

    case DIRECTION_WEST: // -X
        headBox = AABB3(
            worldPos + Vec3(0.f, 0.f, 0.f),
            worldPos + Vec3(headThick, 1.f, 1.f));
        armBox = AABB3(
            worldPos + Vec3(headThick, armOff, armOff),
            worldPos + Vec3(1.f, armOff + armW, armOff + armW));
        break;

    default:
        // 默认按向上处理，避免奇怪情况
        headBox = AABB3(
            worldPos + Vec3(0.f, 0.f, 1.f - headThick),
            worldPos + Vec3(1.f, 1.f, 1.f));
        armBox = AABB3(
            worldPos + Vec3(armOff, armOff, 0.f),
            worldPos + Vec3(armOff + armW, armOff + armW, 1.f - headThick));
        break;
    }
    // 头板：用 topUVs（正面那块带十字的木板）
    AddVertsForIndexAABB3D(m_vertices, m_indices, headBox, color, topUVs);
    // 中间那根小柱：用 sideUVs
    AddVertsForIndexAABB3D(m_vertices, m_indices, armBox, color, sideUVs);
}

void Chunk::AddObserverToMesh(const BlockIterator& iter)
{
    Block* observer = iter.GetBlock();
    if (!observer) return;
    
    IntVec3 globalCoords = iter.GetGlobalCoords();
    Vec3 pos((float)globalCoords.x, (float)globalCoords.y, (float)globalCoords.z);
    
    const BlockDefinition& def = BlockDefinition::GetBlockDef(observer->m_typeIndex);
    
    // top = 检测面, bottom = 输出面, side = 侧面
    AABB2 frontUVs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_topSpriteCoords);
    
    AABB2 backOffUVs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_bottomSpriteCoords);
    IntVec2 backOnCoords = def.m_bottomSpriteCoords + IntVec2(1, 0);   
    AABB2 backOnUVs = g_theGame->m_spriteSheet->GetSpriteUVs(backOnCoords);
    bool isOn = observer->GetSpecialState();
    AABB2 backUVs = isOn ? backOnUVs : backOffUVs;
    
    AABB2 sideUVs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_sideSpriteCoords);
    
    Rgba8 color = Rgba8::WHITE;
    Direction facing = (Direction)observer->GetBlockFacing();
    
    // 8个顶点
    Vec3 p000 = pos;
    Vec3 p100 = pos + Vec3(1, 0, 0);
    Vec3 p010 = pos + Vec3(0, 1, 0);
    Vec3 p110 = pos + Vec3(1, 1, 0);
    Vec3 p001 = pos + Vec3(0, 0, 1);
    Vec3 p101 = pos + Vec3(1, 0, 1);
    Vec3 p011 = pos + Vec3(0, 1, 1);
    Vec3 p111 = pos + Vec3(1, 1, 1);
    
    // 根据朝向渲染6个面
    switch (facing)
    {
    case DIRECTION_NORTH:  // 检测面朝 +Y
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, frontUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, backUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, sideUVs);
        break;
    case DIRECTION_SOUTH:  // 检测面朝 -Y
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, frontUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, backUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, sideUVs);
        break;
    case DIRECTION_EAST:   // 检测面朝 +X
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, frontUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, backUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, sideUVs);
        break;
    case DIRECTION_WEST:   // 检测面朝 -X
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, frontUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, backUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, sideUVs);
        break;
    case DIRECTION_UP:     // 检测面朝 +Z
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, frontUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, backUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, sideUVs);
        break;
    case DIRECTION_DOWN:   // 检测面朝 -Z
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, frontUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, backUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, sideUVs);
        break;
    default:
        // 默认朝 +Y
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p010, p011, p111, color, frontUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p000, p100, p101, p001, color, backUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p100, p110, p111, p101, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p010, p000, p001, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p001, p101, p111, p011, color, sideUVs);
        AddVertsForIndexQuad3D(m_vertices, m_indices, p110, p100, p000, p010, color, sideUVs);
    }
}

// void Chunk::AddWireClimbingFace(std::vector<Vertex_PCU>& verts, const Vec3& basePos, Direction climbDir,
//                                 const Rgba8& color, const Vec2& uvMin, const Vec2& uvMax)
// {
//     float offset = 0.02f;  // 避免z-fighting
//     Vec3 p0, p1, p2, p3;
//     
//     switch (climbDir)
//     {
//     case DIRECTION_NORTH:
//         p0 = basePos + Vec3(0, 0, offset);
//         p1 = basePos + Vec3(1, 0, offset);
//         p2 = basePos + Vec3(1, 0, 1 + offset);
//         p3 = basePos + Vec3(0, 0, 1 + offset);
//         break;
//         
//     case DIRECTION_SOUTH:
//         p0 = basePos + Vec3(0, 1, offset);
//         p1 = basePos + Vec3(1, 1, offset);
//         p2 = basePos + Vec3(1, 1, 1 + offset);
//         p3 = basePos + Vec3(0, 1, 1 + offset);
//         break;
//         
//     case DIRECTION_EAST:
//         p0 = basePos + Vec3(1, 0, offset);
//         p1 = basePos + Vec3(1, 1, offset);
//         p2 = basePos + Vec3(1, 1, 1 + offset);
//         p3 = basePos + Vec3(1, 0, 1 + offset);
//         break;
//         
//     case DIRECTION_WEST:
//         p0 = basePos + Vec3(0, 0, offset);
//         p1 = basePos + Vec3(0, 1, offset);
//         p2 = basePos + Vec3(0, 1, 1 + offset);
//         p3 = basePos + Vec3(0, 0, 1 + offset);
//         break;
//         
//     default:
//         return;
//     }
//     verts.push_back(Vertex_PCU(p0, color, Vec2(uvMin.x, uvMin.y)));
//     verts.push_back(Vertex_PCU(p1, color, Vec2(uvMax.x, uvMin.y)));
//     verts.push_back(Vertex_PCU(p2, color, Vec2(uvMax.x, uvMax.y)));
//     
//     verts.push_back(Vertex_PCU(p0, color, Vec2(uvMin.x, uvMin.y)));
//     verts.push_back(Vertex_PCU(p2, color, Vec2(uvMax.x, uvMax.y)));
//     verts.push_back(Vertex_PCU(p3, color, Vec2(uvMin.x, uvMax.y)));
// }

// Rgba8 Chunk::GetRedstoneWireTint(Block* block)
// {
//     if (!IsRedstoneWire(block->m_typeIndex))
//         return Rgba8::WHITE;
//     
//     uint8_t power = block->GetRedstonePower();
//     float t = (float)power / 15.0f;
//     
//     // 从暗红(76,0,0)到亮红(255,25,25)
//     return Rgba8(
//         (uint8_t)(76 + t * 179),   // R: 76 → 255
//         (uint8_t)(t * 25),          // G: 0 → 25
//         (uint8_t)(t * 25),          // B: 0 → 25
//         255
//     );
// }

bool Chunk::NeedsSpecialRenderingAsRedstoneComp(uint8_t blockType)
{
    if (IsRedstoneWire(blockType))
        return true;
    switch (blockType)
    {
    case BLOCK_TYPE_REDSTONE_TORCH:
    case BLOCK_TYPE_REDSTONE_TORCH_OFF:
    case BLOCK_TYPE_REPEATER_OFF:
    case BLOCK_TYPE_REPEATER_ON:
    case BLOCK_TYPE_REDSTONE_LEVER:
    case BLOCK_TYPE_BUTTON_STONE:
    case BLOCK_TYPE_REDSTONE_PISTON:
    case BLOCK_TYPE_PISTON_HEAD:
    case BLOCK_TYPE_REDSTONE_OBSERVER:
        return true;
    default:
        return false;
    }
}

void Chunk::AddCropToMesh(const BlockIterator& iter, std::vector<Vertex_PCUTBN>& targetVerts, std::vector<unsigned int>& targetIndices)
{
    Block* crop = iter.GetBlock();
    if (!crop) return;
    
    IntVec3 globalCoords = iter.GetGlobalCoords();
    Vec3 worldPos((float)globalCoords.x, (float)globalCoords.y, (float)globalCoords.z);
    
    const BlockDefinition& def = BlockDefinition::GetBlockDef(crop->m_typeIndex);
    AABB2 uvs = g_theGame->m_spriteSheet->GetSpriteUVs(def.m_topSpriteCoords);
    Rgba8 color = GetCropGrowthColor(crop->m_typeIndex);
    float height = 1.0f;
    // 十字模型：X方向面
    AddVertsForIndexQuad3D(targetVerts, targetIndices,
        worldPos + Vec3(0, 0.2f, 0), worldPos + Vec3(1, 0.8f, 0),
        worldPos + Vec3(1, 0.8f, height), worldPos + Vec3(0, 0.2f, height),
        color, uvs);
    AddVertsForIndexQuad3D(targetVerts, targetIndices,
        worldPos + Vec3(1, 0.8f, 0), worldPos + Vec3(0, 0.2f, 0),
        worldPos + Vec3(0, 0.2f, height), worldPos + Vec3(1, 0.8f, height),
        color, uvs);
    // 十字模型：Y方向面
    AddVertsForIndexQuad3D(targetVerts, targetIndices,
        worldPos + Vec3(0.2f, 0, 0), worldPos + Vec3(0.8f, 1, 0),
        worldPos + Vec3(0.8f, 1, height), worldPos + Vec3(0.2f, 0, height),
        color, uvs);
    AddVertsForIndexQuad3D(targetVerts, targetIndices,
        worldPos + Vec3(0.8f, 1, 0), worldPos + Vec3(0.2f, 0, 0),
        worldPos + Vec3(0.2f, 0, height), worldPos + Vec3(0.8f, 1, height),
        color, uvs);
}

void Chunk::AddRedstoneComponentToMesh(const BlockIterator& iter, std::vector<Vertex_PCUTBN>& targetVerts, std::vector<unsigned int>& targetIndices)
{
    Block* block = iter.GetBlock();
    if (!block) return;
    
    uint8_t type = block->m_typeIndex;
    if (IsRedstoneWire(type))
    {
        AddRedstoneWireToMesh(iter);
        return;
    }
    switch (type)
    {
    case BLOCK_TYPE_REDSTONE_TORCH:
    case BLOCK_TYPE_REDSTONE_TORCH_OFF:
        AddRedstoneTorchToMesh(iter);
        break;
    case BLOCK_TYPE_REPEATER_OFF:
    case BLOCK_TYPE_REPEATER_ON:
        AddRepeaterToMesh(iter);
        break;
    case BLOCK_TYPE_REDSTONE_LEVER:
        AddLeverToMesh(iter);
        break;
    case BLOCK_TYPE_BUTTON_STONE:
    //case BLOCK_TYPE_BUTTON_WOOD:
        AddButtonToMesh(iter);
        break;
    case BLOCK_TYPE_REDSTONE_PISTON:
        AddPistonToMesh(iter);
        break;
    case BLOCK_TYPE_PISTON_HEAD:
        AddPistonHeadToMesh(iter);
        break;
    case BLOCK_TYPE_REDSTONE_OBSERVER:
        AddObserverToMesh(iter);
        break;
    }
}

bool Chunk::HandleBlockInteraction(const BlockIterator& block)
{
    if (!block.IsValid())
        return false;
    Block* b = block.GetBlock();
    if (!b)
        return false;

   /* if (g_theApp->WasKeyJustPressed(KEYCODE_F2))
    {
        g_theGameUIManager->OpenRedstoneConfig(m_world->m_currentRaycast.m_hitGlobalCoords);
    }*/
    
    switch (b->m_typeIndex)
    {
    case BLOCK_TYPE_REDSTONE_LEVER:
        m_world->m_redstoneSimulator->ToggleLever(block);
        return true;
        
    case BLOCK_TYPE_BUTTON_STONE:
        m_world->m_redstoneSimulator->PressButton(block);
        return true;
        
    case BLOCK_TYPE_REPEATER_OFF:
    case BLOCK_TYPE_REPEATER_ON:
        m_world->m_redstoneSimulator->CycleRepeaterDelay(block);
        return true;
    default:
        break;
    }
    return false;
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
    if (!m_vertices.empty())
    {
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
    if (!m_transparentVertices.empty())
    {
        if (m_transparentVertexBuffer)
        {
            delete m_transparentVertexBuffer;
            m_transparentVertexBuffer = nullptr;
        }
        if (m_transparentIndexBuffer)
        {
            delete m_transparentIndexBuffer;
            m_transparentIndexBuffer = nullptr;
        }

        m_transparentVertexBuffer = g_theRenderer->CreateVertexBuffer((unsigned int)(m_transparentVertices.size() * sizeof(Vertex_PCUTBN)),
                                                                      sizeof(Vertex_PCUTBN));
        m_transparentIndexBuffer = g_theRenderer->CreateIndexBuffer((unsigned int)(m_transparentIndices.size() * sizeof(unsigned int)),
                                                                    sizeof(unsigned int));
        
        g_theRenderer->CopyCPUToGPU(m_transparentVertices.data(),(unsigned int)(m_transparentVertices.size() * sizeof(Vertex_PCUTBN)),m_transparentVertexBuffer);
        g_theRenderer->CopyCPUToGPU(m_transparentIndices.data(), (unsigned int)(m_transparentIndices.size() * sizeof(unsigned int)),m_transparentIndexBuffer);
    }
}
bool Chunk::AreAllNeighborsActive() const
{
    return (m_eastNeighbor && m_eastNeighbor->GetState() == ChunkState::ACTIVE) &&
          (m_westNeighbor && m_westNeighbor->GetState() == ChunkState::ACTIVE) &&
          (m_northNeighbor && m_northNeighbor->GetState() == ChunkState::ACTIVE) &&
          (m_southNeighbor && m_southNeighbor->GetState() == ChunkState::ACTIVE);
}

void Chunk::RefillWaterAboveUnderwaterPlants()
{
    for (int y = 0; y < CHUNK_SIZE_Y; y++)
    {
        for (int x = 0; x < CHUNK_SIZE_X; x++)
        {
            for (int z = 2; z < CHUNK_SIZE_Z - 1; z++)
            {
                int idx = LocalCoordsToIndex(x, y, z);
                uint8_t blockType = m_blocks[idx].m_typeIndex;
                
                if (IsUnderwaterProduct(blockType))
                {
                    // 检查上方是否是 AIR（应该是 WATER）
                    int idxAbove = LocalCoordsToIndex(x, y, z + 1);
                    if (m_blocks[idxAbove].m_typeIndex == BLOCK_TYPE_AIR)
                    {
                        // 填充水直到遇到非AIR或到达海平面
                        for (int zAbove = z + 1; zAbove < g_theGame->g_seaLevel && zAbove < CHUNK_SIZE_Z; zAbove++)
                        {
                            int idxFill = LocalCoordsToIndex(x, y, zAbove);
                            if (m_blocks[idxFill].m_typeIndex == BLOCK_TYPE_AIR)
                            {
                                m_blocks[idxFill].SetType(BLOCK_TYPE_WATER);
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

void Chunk::PrepareSortedIndices(const Vec3& cameraPos)
{
    size_t numFaces = m_transparentIndices.size() / 6;
    if (m_transparentFaceCenters.size() != numFaces || numFaces == 0)
        return;
    // 检查缓存
    float cameraMoveDist = (cameraPos - m_lastSortCameraPos).GetLength();
    float distToChunk = (cameraPos - m_bounds.GetCenter()).GetLength();
    
    float resortThreshold;
    if (distToChunk < 30.0f)
        resortThreshold = 3.0f;
    else if (distToChunk < 60.0f)
        resortThreshold = 8.0f;
    else
        resortThreshold = 15.0f;
    {
        std::lock_guard<std::mutex> lock(m_sortMutex);
        if (m_sortedIndicesValid && cameraMoveDist < resortThreshold)
            return;
    }
    // 排序
    std::vector<size_t> faceOrder(numFaces);
    for (size_t i = 0; i < numFaces; i++)
        faceOrder[i] = i;
    
    std::sort(faceOrder.begin(), faceOrder.end(), 
        [this, &cameraPos](size_t a, size_t b) -> bool 
        {
            return (m_transparentFaceCenters[a] - cameraPos).GetLengthSquared() > 
                   (m_transparentFaceCenters[b] - cameraPos).GetLengthSquared();
        });
    
    // 重建索引
    std::vector<unsigned int> newSortedIndices;
    newSortedIndices.reserve(m_transparentIndices.size());
    
    for (size_t faceIdx : faceOrder)
    {
        size_t baseIdx = faceIdx * 6;
        for (size_t i = 0; i < 6; i++)
        {
            newSortedIndices.push_back(m_transparentIndices[baseIdx + i]);
        }
    }
    // 更新共享数据
    {
        std::lock_guard<std::mutex> lock(m_sortMutex);
        m_sortedTransparentIndices.swap(newSortedIndices);
        m_lastSortCameraPos = cameraPos;
        m_sortedIndicesValid = true;
    }
}

void Chunk::RenderTransparentPreSorted(const Vec3& cameraPos) const
{
    if (m_transparentVertexBuffer == nullptr || 
        m_transparentIndexBuffer == nullptr || 
        m_transparentIndices.empty())
    {
        return;
    }
    // 检查排序数据
    bool hasValidSort = false;
    {
        std::lock_guard<std::mutex> lock(m_sortMutex);
        hasValidSort = m_sortedIndicesValid && !m_sortedTransparentIndices.empty();
    }
    if (!hasValidSort)
    {
        // 降级：直接渲染
        g_theRenderer->BindVertexBuffer(m_transparentVertexBuffer);
        g_theRenderer->BindIndexBuffer(m_transparentIndexBuffer);
        g_theRenderer->DrawIndexBuffer(m_transparentVertexBuffer, 
                                       m_transparentIndexBuffer, 
                                       (unsigned int)m_transparentIndices.size());
        return;
    }
    // 更新 GPU 缓冲
    {
        std::lock_guard<std::mutex> lock(m_sortMutex);
        
        if (m_sortedTransparentIndexBuffer == nullptr)
        {
            m_sortedTransparentIndexBuffer = g_theRenderer->CreateIndexBuffer(
                (unsigned int)(m_sortedTransparentIndices.size() * sizeof(unsigned int)),
                sizeof(unsigned int));
        }
        
        g_theRenderer->CopyCPUToGPU(
            m_sortedTransparentIndices.data(),
            (unsigned int)(m_sortedTransparentIndices.size() * sizeof(unsigned int)),
            m_sortedTransparentIndexBuffer);
    }
    
    g_theRenderer->BindVertexBuffer(m_transparentVertexBuffer);
    g_theRenderer->BindIndexBuffer(m_sortedTransparentIndexBuffer);
    g_theRenderer->DrawIndexBuffer(m_transparentVertexBuffer, 
                                   m_sortedTransparentIndexBuffer,
                                   (unsigned int)m_sortedTransparentIndices.size());

}

void Chunk::ReportDirty()
{
    m_world->m_hasDirtyChunk = true;
}
