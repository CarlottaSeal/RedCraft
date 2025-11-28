#include "World.h"

#include <algorithm>
#include <chrono>

#include "Game.hpp"

#include "Chunk.h"
#include "ChunkJob.h"
#include "ChunkUtils.h"
#include "Player.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Gameplay/RedstoneSimulator.h"
#include "ThirdParty/Noise/SmoothNoise.hpp"

World::World(Game* owner)
    :m_owner(owner)
{
    m_worldGenPipeline = new WorldGenPipeline();

    m_worldConstantBuffer = g_theRenderer->CreateConstantBuffer(sizeof(WorldConstants));
    m_worldShader = g_theRenderer->CreateOrGetShader("Data/Shaders/WorldShader", VertexType::VERTEX_PCUTBN);

    m_redstoneSimulator = new RedstoneSimulator(this);
}

World::~World()
{
    delete m_worldConstantBuffer;
    m_worldConstantBuffer = nullptr;
    
	for (auto pair : m_activeChunks)
	{
		Chunk* chunk = pair.second;
		delete chunk;
		chunk = nullptr;
	}
    delete m_worldGenPipeline;

    if (m_redstoneSimulator)
    {
        delete m_redstoneSimulator;
        m_redstoneSimulator = nullptr;
    }
}

void World::Update(float deltaSeconds)
{
    //UpdateVisibleChunks();
    
    UpdateAccelerateTime();
    UpdateTypeToPlace();
    UpdateDiggingAndPlacing(deltaSeconds);
    // Chunk* chunkToUpdate = GetChunkFromPlayerCameraPosition(m_owner->m_player->m_position);
    // if(chunkToUpdate)
    //     chunkToUpdate->Update(deltaSeconds);
    
    ProcessCompletedJobs();
    
    // if ((int)m_activeChunks.size() < MAX_ACTIVE_CHUNKS)
    // {
    //     ActivateSingleNearestMissingChunkWithinRange();
    // }
    if ((int)m_activeChunks.size() < MAX_ACTIVE_CHUNKS)
    {
        SubmitNewActivateJobs(); 
    }
    DeactivateSingleFarthestOutsideRangeIfAny();
    
    m_debugTimer += deltaSeconds;
	if (m_debugTimer >= 2.0f)
	{
		m_debugTimer = 0.0f;
	    if (g_theJobSystem->GetPendingJobCount() != 0 && g_theJobSystem->GetExecutingJobCount() !=0)
	    {
	        if (g_theJobSystem)
	        {
	            g_theJobSystem->PrintDebugInfo();
	        }
	        g_theDevConsole->AddLine(Rgba8::MAGENTA,
                Stringf("Active Chunks: %d, Processing: %d, Rendering: %d",
                    (int)m_activeChunks.size(),
                    (int)m_processingChunks.size(),
                    (int)m_visibleChunks.size()));
	    }
	}

    UpdateDayNightCycle(deltaSeconds);
    UpdateWorldConstants();
    ProcessDirtyLighting();

    if (m_redstoneSimulator)
    {
        m_redstoneSimulator->Update(deltaSeconds);
    }
    
    RebuildDirtyMeshes(2);
}

void World::Render() const
{
    BindWorldConstansBuffer();
    for (auto& chunkPair : m_activeChunks)
    {
        chunkPair.second->Render();
    }
    if (m_highlightedBlock.m_isValid)
    {
        RenderBlockHighlight();
    }
	// for (Chunk* chunk : m_visibleChunks)
	// {
	// 	chunk->Render();
	//     DebuggerPrintf("Visible Chunks: %d\n", (int)m_visibleChunks.size());
	// }
}

void World::RenderBlockHighlight() const
{
     if (!m_highlightedBlock.m_isValid)
        return;
    
    Vec3 blockWorldPos = Vec3(
        (float)m_highlightedBlock.m_worldCoords.x,
        (float)m_highlightedBlock.m_worldCoords.y,
        (float)m_highlightedBlock.m_worldCoords.z
    );

    constexpr float EXPAND = 0.002f;
    constexpr float BORDER_WIDTH = 0.05f;
    
    std::vector<Vertex_PCU> faceVertices;
    Rgba8 borderColor = Rgba8(0, 0, 0, 150);
    Rgba8 centerColor = Rgba8(255, 255, 255, 70);
    AABB2 UVs(Vec2(0, 0), Vec2(1, 1));
    
    Direction face = m_highlightedBlock.m_hitFace;
    
    switch(face)
    {
    case Direction::DIRECTION_NORTH:
        { 
            float y = blockWorldPos.y + 1.0f + EXPAND;
           
            AddVertsForQuad3D(faceVertices, 
                Vec3(blockWorldPos.x, y, blockWorldPos.z),
                Vec3(blockWorldPos.x, y, blockWorldPos.z + 1.0f),
                Vec3(blockWorldPos.x + 1.0f, y, blockWorldPos.z + 1.0f),
                Vec3(blockWorldPos.x + 1.0f, y, blockWorldPos.z),
                borderColor, UVs);
          
            float inset = BORDER_WIDTH;
            AddVertsForQuad3D(faceVertices,
                Vec3(blockWorldPos.x + inset, y, blockWorldPos.z + inset),
                Vec3(blockWorldPos.x + inset, y, blockWorldPos.z + 1.0f - inset),
                Vec3(blockWorldPos.x + 1.0f - inset, y, blockWorldPos.z + 1.0f - inset),
                Vec3(blockWorldPos.x + 1.0f - inset, y, blockWorldPos.z + inset),
                centerColor, UVs);
            break;
    }
        
    case Direction::DIRECTION_SOUTH:{ // -Y
            float y = blockWorldPos.y - EXPAND;
            AddVertsForQuad3D(faceVertices,
                Vec3(blockWorldPos.x, y, blockWorldPos.z),
                Vec3(blockWorldPos.x + 1.0f, y, blockWorldPos.z),
                Vec3(blockWorldPos.x + 1.0f, y, blockWorldPos.z + 1.0f),
                Vec3(blockWorldPos.x, y, blockWorldPos.z + 1.0f),
                borderColor, UVs);
            
            float inset = BORDER_WIDTH;
            AddVertsForQuad3D(faceVertices,
                Vec3(blockWorldPos.x + inset, y, blockWorldPos.z + inset),
                Vec3(blockWorldPos.x + 1.0f - inset, y, blockWorldPos.z + inset),
                Vec3(blockWorldPos.x + 1.0f - inset, y, blockWorldPos.z + 1.0f - inset),
                Vec3(blockWorldPos.x + inset, y, blockWorldPos.z + 1.0f - inset),
                centerColor, UVs);
            break;
    }
        
        case Direction::DIRECTION_EAST: { // +X
            float x = blockWorldPos.x + 1.0f + EXPAND;
          
            AddVertsForQuad3D(faceVertices,
                Vec3(x, blockWorldPos.y, blockWorldPos.z),
                Vec3(x, blockWorldPos.y + 1.0f, blockWorldPos.z),
                Vec3(x, blockWorldPos.y + 1.0f, blockWorldPos.z + 1.0f),
                Vec3(x, blockWorldPos.y, blockWorldPos.z + 1.0f),
                borderColor, UVs);
            
            float inset = BORDER_WIDTH;
            AddVertsForQuad3D(faceVertices,
                Vec3(x, blockWorldPos.y + inset, blockWorldPos.z + inset),
                Vec3(x, blockWorldPos.y + 1.0f - inset, blockWorldPos.z + inset),
                Vec3(x, blockWorldPos.y + 1.0f - inset, blockWorldPos.z + 1.0f - inset),
                Vec3(x, blockWorldPos.y + inset, blockWorldPos.z + 1.0f - inset),
                centerColor, UVs);
            break;
        }
        
        case Direction::DIRECTION_WEST: { // -X
            float x = blockWorldPos.x - EXPAND;
     
            AddVertsForQuad3D(faceVertices,
                Vec3(x, blockWorldPos.y + 1.0f, blockWorldPos.z),
                Vec3(x, blockWorldPos.y, blockWorldPos.z),
                Vec3(x, blockWorldPos.y, blockWorldPos.z + 1.0f),
                Vec3(x, blockWorldPos.y + 1.0f, blockWorldPos.z + 1.0f),
                borderColor, UVs);

            float inset = BORDER_WIDTH;
            AddVertsForQuad3D(faceVertices,
                Vec3(x, blockWorldPos.y + 1.0f - inset, blockWorldPos.z + inset),
                Vec3(x, blockWorldPos.y + inset, blockWorldPos.z + inset),
                Vec3(x, blockWorldPos.y + inset, blockWorldPos.z + 1.0f - inset),
                Vec3(x, blockWorldPos.y + 1.0f - inset, blockWorldPos.z + 1.0f - inset),
                centerColor, UVs);
            break;
        }
        
    case Direction::DIRECTION_UP: { 
            float z = blockWorldPos.z + 1.0f + EXPAND;

            AddVertsForQuad3D(faceVertices,
                Vec3(blockWorldPos.x, blockWorldPos.y, z),
                Vec3(blockWorldPos.x + 1.0f, blockWorldPos.y, z),
                Vec3(blockWorldPos.x + 1.0f, blockWorldPos.y + 1.0f, z),
                Vec3(blockWorldPos.x, blockWorldPos.y + 1.0f, z),
                borderColor, UVs);
 
            float inset = BORDER_WIDTH;
            AddVertsForQuad3D(faceVertices,
                Vec3(blockWorldPos.x + inset, blockWorldPos.y + inset, z),
                Vec3(blockWorldPos.x + 1.0f - inset, blockWorldPos.y + inset, z),
                Vec3(blockWorldPos.x + 1.0f - inset, blockWorldPos.y + 1.0f - inset, z),
                Vec3(blockWorldPos.x + inset, blockWorldPos.y + 1.0f - inset, z),
                centerColor, UVs);
            break;
        }
        
        case Direction::DIRECTION_DOWN: { 
            float z = blockWorldPos.z - EXPAND;
        
            AddVertsForQuad3D(faceVertices,
                Vec3(blockWorldPos.x, blockWorldPos.y + 1.0f, z),
                Vec3(blockWorldPos.x + 1.0f, blockWorldPos.y + 1.0f, z),
                Vec3(blockWorldPos.x + 1.0f, blockWorldPos.y, z),
                Vec3(blockWorldPos.x, blockWorldPos.y, z),
                borderColor, UVs);
          
            float inset = BORDER_WIDTH;
            AddVertsForQuad3D(faceVertices,
                Vec3(blockWorldPos.x + inset, blockWorldPos.y + 1.0f - inset, z),
                Vec3(blockWorldPos.x + 1.0f - inset, blockWorldPos.y + 1.0f - inset, z),
                Vec3(blockWorldPos.x + 1.0f - inset, blockWorldPos.y + inset, z),
                Vec3(blockWorldPos.x + inset, blockWorldPos.y + inset, z),
                centerColor, UVs);
            break;
        }
    }
    
    g_theRenderer->SetModelConstants(Mat44());
    g_theRenderer->SetBlendMode(BlendMode::ALPHA);
    g_theRenderer->SetDepthMode(DepthMode::DISABLED); 
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->BindShader(nullptr);
    g_theRenderer->DrawVertexArray((int)faceVertices.size(), faceVertices.data());
}

Chunk* World::GetChunkFromPlayerCameraPosition(Vec3 cameraPos)
{
    const int chunkX = FloorDivision(FloorToInt(cameraPos.x), CHUNK_SIZE_X);
    const int chunkY = FloorDivision(FloorToInt(cameraPos.y), CHUNK_SIZE_Y);
    return GetChunk(chunkX, chunkY);
}

Chunk* World::GetChunk(int chunkX, int chunkY)
{
    for (auto pair : m_activeChunks)
    {
        Chunk* ch = pair.second;
        const IntVec2& c = ch->m_chunkCoords; 
        if (c.x == chunkX && c.y == chunkY)
        {
            return ch;
        }
    }
    return nullptr;
}

Block World::GetBlockAtWorldCoords(int worldX, int worldY, int worldZ)
{
    if (worldZ < 0 || worldZ >= CHUNK_SIZE_Z)
    {
        return Block();
    }

    const int chunkX = FloorDivision(worldX, CHUNK_SIZE_X);
    const int chunkY = FloorDivision(worldY, CHUNK_SIZE_Y);

    // 0-15
    const int localX = GetEuclideanMod(worldX, CHUNK_SIZE_X);
    const int localY = GetEuclideanMod(worldY, CHUNK_SIZE_Y);
    const int localZ = worldZ; 

    Chunk* ch = GetChunk(chunkX, chunkY);
    if (!ch)
    {
        return Block();
    }

    return ch->GetBlock(localX, localY, localZ);
}

BlockIterator World::GetBlockIterator(const IntVec3& globalCoords)
{
    if (globalCoords.z < 0 || globalCoords.z >= CHUNK_SIZE_Z)
    {
        return BlockIterator();
    }

    IntVec2 chunkCoords = GetChunkCoords(globalCoords);
    Chunk* chunk = GetChunk(chunkCoords.x, chunkCoords.y);
    if (!chunk || chunk->GetState() != ChunkState::ACTIVE)
    {
        return BlockIterator();
    }

    IntVec3 localCoords = GlobalCoordsToLocalCoords(globalCoords);

    return BlockIterator(chunk, localCoords);
}

bool World::CanConnectToRedstone(const IntVec3& globalPos)
{
    Block block = GetBlockAtWorldCoords(globalPos.x, globalPos.y, globalPos.z);
    
    uint8_t type = block.m_typeIndex;
    
    // 红石线可以连接到：
    // 1. 其他红石线
    // 2. 红石组件（火把、中继器、比较器等）
    // 3. 被充能的方块（可选，简化版可以不要）
    
    return IsRedstoneWire(type) ||
           IsRedstoneComponent(type) ||
           type == BLOCK_TYPE_REDSTONE_BLOCK;
}

IntVec2 World::WorldToChunkXY(const Vec3& worldPos)
{
    const int chunkX = FloorDivision(FloorToInt(worldPos.x), CHUNK_SIZE_X);
    const int chunkY = FloorDivision(FloorToInt(worldPos.y), CHUNK_SIZE_Y);
    return IntVec2(chunkX, chunkY);
}

void World::ProcessDirtyLighting()
{
    while (!m_dirtyLightBlocks.empty())
    {
        ProcessNextDirtyLightBlock();
    }
}

void World::ProcessNextDirtyLightBlock()
{
    if (m_dirtyLightBlocks.empty())
        return;

    BlockIterator iter = m_dirtyLightBlocks.front();
    m_dirtyLightBlocks.pop_front();
    
    if (!iter.IsValid() || !iter.GetChunk() || 
         iter.GetChunk()->GetState() != ChunkState::ACTIVE)
        return;
    
    Block* block = iter.GetBlock();
    if (!block)
        return;
    
    // 清除脏标志（已经从队列中移除）
    block->SetLightDirty(false);
    
    // 计算理论正确的光照值
    uint8_t correctOutdoorLight = 0;
    uint8_t correctIndoorLight = 0;
    if (!iter.IsValid())
        return;
    ComputeCorrectLightInfluence(iter, correctOutdoorLight, correctIndoorLight);
    
    // 获取当前光照值
    uint8_t currentOutdoorLight = block->GetOutdoorLight();
    uint8_t currentIndoorLight = block->GetIndoorLight();
    
    // 检查是否需要更新
    if (correctOutdoorLight != currentOutdoorLight || 
        correctIndoorLight != currentIndoorLight)
    {
        // 更新光照值
        block->SetOutdoorLight(correctOutdoorLight);
        block->SetIndoorLight(correctIndoorLight);
        
        // 标记该 Chunk 和相邻 Chunk 的网格为脏
        Chunk* chunk = iter.GetChunk();
        if (chunk)
        {
            chunk->m_isDirty = true; //是否立刻生成？
        }
        
        // 标记六个相邻方块为脏（只标记非不透明的）
        for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
        {
            BlockIterator neighbor = iter.GetNeighborCrossBoundary((Direction)dir);
            if (neighbor.IsValid())
            {
                MarkLightingDirtyIfNotOpaque(neighbor);
                
                // 如果邻居在不同 Chunk，也标记那个 Chunk 为脏
                if (neighbor.GetChunk() != chunk)
                {
                    neighbor.GetChunk()->m_isDirty = true;
                }
            }
        }
    }
}

void World::MarkLightingDirty(const BlockIterator& iter)
{
    if (!iter.IsValid())
        return;
    
    Block* block = iter.GetBlock();
    if (!block)
        return;
    
    if (block->IsLightDirty())
        return;
    
    m_dirtyLightBlocks.push_back(iter);
    
    block->SetLightDirty(true);
}

void World::MarkLightingDirtyIfNotOpaque(const BlockIterator& iter)
{
    if (!iter.GetChunk())
        return;

    if (!iter.IsValid())
        return;
    
    Block* block = iter.GetBlock();
    if (!block)
        return;
    
    if (!block->IsOpaque())
    {
        MarkLightingDirty(iter);
    }
}

void World::UndirtyAllBlocksInChunk(Chunk* chunk)
{
    if (!chunk)
        return;
    
    // 从队列中移除该 Chunk 的所有方块
    auto it = m_dirtyLightBlocks.begin();
    while (it != m_dirtyLightBlocks.end())
    {
        if (it->GetChunk() == chunk)
        {
            // 清除方块的脏标志
            Block* block = it->GetBlock();
            if (block)
                block->SetLightDirty(false);
            
            // 从队列中移除
            it = m_dirtyLightBlocks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void World::UpdateWorldConstants()
{
    // m_skyColor = CalculateSkyColor();
    // m_outdoorLightColor = CalculateOutdoorLightColor();
    // m_indoorLightColor = Rgba8(255, 230, 204);
    //TODO: 天气系统
    Rgba8 baseSkyColor = CalculateSkyColor();
    Rgba8 baseOutdoorColor = CalculateOutdoorLightColor();
    Rgba8 baseIndoorColor = Rgba8(255, 230, 204); //TODO: 外部改
    
    float lightningStrength = CalculateLightningStrength();
    // 闪电效果：将天空和室外光向白色插值
    Rgba8 white(255, 255, 255);
    Rgba8 finalSkyColor = InterpolateRgba8(baseSkyColor, white, lightningStrength);
    Rgba8 finalOutdoorColor = InterpolateRgba8(baseOutdoorColor, white, lightningStrength);
    
    float glowFlicker = CalculateGlowstoneFlicker();
    
    // 应用闪烁到室内光
    Rgba8 finalIndoorColor = baseIndoorColor;
    finalIndoorColor.r = (unsigned char)(finalIndoorColor.r * glowFlicker);
    finalIndoorColor.g = (unsigned char)(finalIndoorColor.g * glowFlicker);
    finalIndoorColor.b = (unsigned char)(finalIndoorColor.b * glowFlicker);
    
    m_skyColor = finalSkyColor;
    m_outdoorLightColor = finalOutdoorColor;
    m_indoorLightColor = finalIndoorColor;
}

float World::GetTimeOfDay() const
{
    // 返回一天中的时刻 [0, 1)
    // 0.0 = 午夜, 0.25 = 黎明(6am), 0.5 = 正午, 0.75 = 黄昏(6pm)
    float fractionalPart = m_worldTimeInDays - floorf(m_worldTimeInDays);
    return fractionalPart;
}

void World::UpdateDayNightCycle(float deltaSeconds)
{
    float timeScale = m_worldTimeScale;
    if (m_isTimeAccelerated)
    {
        timeScale *= TIME_ACCELERATION_MULTIPLIER;
    }
    
    float realSecondsPerWorldDay = (60.0f * 60.0f * 24.0f) / WORLD_TO_REAL_TIME_RATIO;
    //m_worldTimeInDays += ((1.f/60.f) * timeScale) / realSecondsPerWorldDay;
    m_worldTimeInDays += (deltaSeconds * timeScale) / realSecondsPerWorldDay;
}

Rgba8 World::CalculateSkyColor() const
{
    float t = GetTimeOfDay(); // [0, 1)
    
    // 计算太阳高度角 (-1到1，0是地平线，1是正午，-1是午夜)
    float sunAngle = cosf((t - 0.5f) * 2.0f * PI);
    
    // V3版本：增加更多暖色调，减少单一的蓝白色
    const Rgba8 deepNight(20, 25, 50);         // 深夜蓝
    const Rgba8 lateNight(25, 30, 55);         // 后半夜
    const Rgba8 preDawn(60, 50, 90);           // 黎明前紫蓝（增加红色）
    const Rgba8 dawn(255, 100, 60);            // 日出深橙红（更暖！）
    const Rgba8 earlyMorning(255, 160, 100);   // 早晨橙金色（更暖！）
    const Rgba8 morning(255, 210, 160);        // 上午金黄色（暖色！）
    const Rgba8 noon(180, 220, 255);           // 正午浅蓝（减少白色）
    const Rgba8 afternoon(255, 200, 150);      // 下午暖金色（暖色！）
    const Rgba8 preSunset(255, 140, 120);      // 日落前粉橙（暖色！）
    const Rgba8 sunset(255, 80, 40);           // 日落火红（非常暖！）
    const Rgba8 dusk(180, 100, 150);           // 黄昏紫粉（暖紫色）
    const Rgba8 earlyNight(50, 40, 80);        // 入夜紫蓝

    Rgba8 color;
    
    // 时间分段
    if (t < 0.08f) 
    { 
        // 00:00 - 01:55: 深夜
        float k = t / 0.08f;
        color = InterpolateRgba8(deepNight, lateNight, SmoothStep3(k));
    }
    else if (t < 0.18f) 
    { 
        // 01:55 - 04:19: 后半夜到黎明前
        float k = (t - 0.08f) / 0.10f;
        color = InterpolateRgba8(lateNight, preDawn, SmoothStep3(k));
    }
    else if (t < 0.23f) 
    { 
        // 04:19 - 05:31: 黎明前到日出开始
        float k = (t - 0.18f) / 0.05f;
        color = InterpolateRgba8(preDawn, dawn, SmoothStart3(k));
    }
    else if (t < 0.27f)
    { 
        // 05:31 - 06:29: 日出高峰（橙红色）
        float k = (t - 0.23f) / 0.04f;
        color = InterpolateRgba8(dawn, earlyMorning, SmoothStop3(k));
    }
    else if (t < 0.33f)
    { 
        // 06:29 - 07:55: 早晨金色
        float k = (t - 0.27f) / 0.06f;
        color = InterpolateRgba8(earlyMorning, morning, SmoothStep3(k));
    }
    else if (t < 0.42f)
    { 
        // 07:55 - 10:05: 上午金黄到浅蓝
        float k = (t - 0.33f) / 0.09f;
        color = InterpolateRgba8(morning, noon, SmoothStep3(k));
    }
    else if (t < 0.58f)
    { 
        // 10:05 - 13:55: 正午时段（浅蓝色）
        color = noon;
    }
    else if (t < 0.67f)
    { 
        // 13:55 - 16:05: 下午（回到暖色）
        float k = (t - 0.58f) / 0.09f;
        color = InterpolateRgba8(noon, afternoon, SmoothStep3(k));
    }
    else if (t < 0.73f)
    { 
        // 16:05 - 17:31: 日落前（粉橙色）
        float k = (t - 0.67f) / 0.06f;
        color = InterpolateRgba8(afternoon, preSunset, SmoothStart3(k));
    }
    else if (t < 0.77f)
    { 
        // 17:31 - 18:29: 日落高峰（火红色）
        float k = (t - 0.73f) / 0.04f;
        color = InterpolateRgba8(preSunset, sunset, SmoothStart3(k));
    }
    else if (t < 0.82f)
    { 
        // 18:29 - 19:41: 黄昏（紫粉色）
        float k = (t - 0.77f) / 0.05f;
        color = InterpolateRgba8(sunset, dusk, SmoothStop3(k));
    }
    else if (t < 0.92f)
    { 
        // 19:41 - 22:05: 入夜
        float k = (t - 0.82f) / 0.10f;
        color = InterpolateRgba8(dusk, earlyNight, SmoothStep3(k));
    }
    else
    { 
        // 22:05 - 00:00: 深夜
        float k = (t - 0.92f) / 0.08f;
        color = InterpolateRgba8(earlyNight, deepNight, SmoothStep3(k));
    }

    // 基于太阳高度的自然亮度调整
    float brightness = GetClamped(sunAngle * 0.5f + 0.7f, 0.35f, 1.05f);
    
    // 日出日落时增强颜色饱和度
    if ((t > 0.22f && t < 0.28f) || (t > 0.72f && t < 0.78f))
    {
        brightness *= 1.15f; // 日出日落时增强15%，让暖色更明显
    }
    
    color.r = (unsigned char)GetClamped(color.r * brightness, 0.0f, 255.0f);
    color.g = (unsigned char)GetClamped(color.g * brightness, 0.0f, 255.0f);
    color.b = (unsigned char)GetClamped(color.b * brightness, 0.0f, 255.0f);

    return color;
}

Rgba8 World::CalculateOutdoorLightColor() const
{
    float t = GetTimeOfDay();
    float sunAngle = cosf((t - 0.5f) * 2.0f * PI);

    // V3版本：增加暖色调，让阳光更温暖
    const Rgba8 deepNight(40, 45, 80);         // 深夜月光
    const Rgba8 lateNight(45, 50, 85);         // 后半夜
    const Rgba8 preDawn(70, 70, 110);          // 黎明前
    const Rgba8 dawn(255, 150, 80);            // 日出温暖橙光
    const Rgba8 earlyMorning(255, 200, 130);   // 早晨金色光
    const Rgba8 morning(255, 235, 200);        // 上午暖白光
    const Rgba8 noon(255, 255, 250);           // 正午微暖白光
    const Rgba8 afternoon(255, 240, 210);      // 下午金色光
    const Rgba8 preSunset(255, 200, 150);      // 日落前橙金光
    const Rgba8 sunset(255, 140, 70);          // 日落深橙光
    const Rgba8 dusk(150, 110, 160);           // 黄昏紫光
    const Rgba8 earlyNight(55, 55, 90);        // 入夜

    Rgba8 color;

    // 与天空颜色同步的时间分段
    if (t < 0.08f)
    {
        float k = t / 0.08f;
        color = InterpolateRgba8(deepNight, lateNight, SmoothStep3(k));
    }
    else if (t < 0.18f)
    {
        float k = (t - 0.08f) / 0.10f;
        color = InterpolateRgba8(lateNight, preDawn, SmoothStep3(k));
    }
    else if (t < 0.23f)
    {
        float k = (t - 0.18f) / 0.05f;
        color = InterpolateRgba8(preDawn, dawn, SmoothStart3(k));
    }
    else if (t < 0.27f)
    {
        float k = (t - 0.23f) / 0.04f;
        color = InterpolateRgba8(dawn, earlyMorning, SmoothStop3(k));
    }
    else if (t < 0.33f)
    {
        float k = (t - 0.27f) / 0.06f;
        color = InterpolateRgba8(earlyMorning, morning, SmoothStep3(k));
    }
    else if (t < 0.42f)
    {
        float k = (t - 0.33f) / 0.09f;
        color = InterpolateRgba8(morning, noon, SmoothStep3(k));
    }
    else if (t < 0.58f)
    {
        color = noon;
    }
    else if (t < 0.67f)
    {
        float k = (t - 0.58f) / 0.09f;
        color = InterpolateRgba8(noon, afternoon, SmoothStep3(k));
    }
    else if (t < 0.73f)
    {
        float k = (t - 0.67f) / 0.06f;
        color = InterpolateRgba8(afternoon, preSunset, SmoothStart3(k));
    }
    else if (t < 0.77f)
    {
        float k = (t - 0.73f) / 0.04f;
        color = InterpolateRgba8(preSunset, sunset, SmoothStart3(k));
    }
    else if (t < 0.82f)
    {
        float k = (t - 0.77f) / 0.05f;
        color = InterpolateRgba8(sunset, dusk, SmoothStop3(k));
    }
    else if (t < 0.92f)
    {
        float k = (t - 0.82f) / 0.10f;
        color = InterpolateRgba8(dusk, earlyNight, SmoothStep3(k));
    }
    else
    {
        float k = (t - 0.92f) / 0.08f;
        color = InterpolateRgba8(earlyNight, deepNight, SmoothStep3(k));
    }

    // 更自然的亮度变化
    float exposure = GetClamped(sunAngle * 0.65f + 0.55f, 0.28f, 1.0f);

    color.r = (unsigned char)GetClamped(color.r * exposure, 0.0f, 255.0f);
    color.g = (unsigned char)GetClamped(color.g * exposure, 0.0f, 255.0f);
    color.b = (unsigned char)GetClamped(color.b * exposure, 0.0f, 255.0f);
    
    return color;
}

float World::CalculateLightningStrength() const
{
    // 第1层：慢噪声 - 雷暴时段（罕见）
    float stormChance = Compute1dPerlinNoise(
        m_worldTimeInDays * 80.0f,    // 慢
        1.0f, 2, 0.5f, 2.0f, false,
        GAME_SEED + 200
    );
    
    if (stormChance < 0.75f)  // 大部分时间不是雷暴
        return 0.0f;
    
    // 第2层：快噪声 - 闪电瞬间（短暂）
    float flashIntensity = Compute1dPerlinNoise(
        m_worldTimeInDays * 1000.0f,  //很快！
        1.0f, 5, 0.5f, 2.0f, false,
        GAME_SEED + 201
    );
    
    if (flashIntensity < 0.94f)  //只在尖峰时触发
        return 0.0f;
    
    // 极窄范围
    float strength = RangeMapClamped(flashIntensity, 0.94f, 0.98f, 0.0f, 1.0f);
    
    // 四次方让闪电极其突然
    return strength * strength * strength * strength;
    // float lightningPerlin = Compute1dPerlinNoise(
    //     m_worldTimeInDays * 80.0f,    // 从1000降到80（慢12倍）
    //     1.0f,
    //     3,                             // 从9降到3（更平滑）
    //     0.5f, 2.0f, false,
    //     GAME_SEED + 200
    // );
    //
    // // 提高阈值
    // if (lightningPerlin < 0.85f)
    //     return 0.0f;
    //
    // // 映射范围从[0.6,0.9]改到[0.85,0.95]
    // float strength = RangeMapClamped(lightningPerlin, 0.85f, 0.95f, 0.0f, 1.0f);
    //
    // // 平方让闪电更突然
    // return strength * strength;
}

float World::CalculateGlowstoneFlicker() const
{
    float glowPerlin = Compute1dPerlinNoise(
        m_worldTimeInDays * 1000.0f,
        1.0f,
        4,                             // 较少的 octaves，更平滑
        0.5f,
        2.0f,
        true,
        GAME_SEED + 201
    );
    
    // 将 [-1, 1] 映射到 [0.8, 1.0]
    float glowStrength = RangeMapClamped(glowPerlin, -1.0f, 1.0f, 0.8f, 1.0f);
    
    return glowStrength;
}

GameRaycastResult3D World::RaycastVsBlocks(const Vec3& start, const Vec3& direction, float maxDistance)
{
    GameRaycastResult3D result;
    
    result.m_rayStartPos = start;
    result.m_rayFwdNormal = direction.GetNormalized();
    result.m_rayMaxLength = maxDistance;
    result.m_didImpact = false;
    
    Vec3 rayDir = result.m_rayFwdNormal;
    
    IntVec3 currentBlock(
        FloorToInt(start.x),
        FloorToInt(start.y),
        FloorToInt(start.z)
    );
    
    IntVec2 chunkCoords(
        currentBlock.x >> CHUNK_BITS_X,
        currentBlock.y >> CHUNK_BITS_Y
    );
    
    Chunk* currentChunk = GetChunk(chunkCoords.x, chunkCoords.y);
    if (!currentChunk)
        return result;
    
    IntVec3 localCoords(
        currentBlock.x & CHUNK_MAX_X,
        currentBlock.y & CHUNK_MAX_Y,
        currentBlock.z
    );
    
    BlockIterator iter(currentChunk, localCoords);
    
    IntVec3 step(
        (rayDir.x >= 0) ? 1 : -1,
        (rayDir.y >= 0) ? 1 : -1,
        (rayDir.z >= 0) ? 1 : -1
    );
    
    // 4. 计算 tMax 和 tDelta
    Vec3 tMax, tDelta;
    
    if (fabsf(rayDir.x) > 1e-6f)
    {
        float nextBoundary = (rayDir.x > 0) ? ceilf(start.x) : floorf(start.x);
        tMax.x = (nextBoundary - start.x) / rayDir.x;
        tDelta.x = (float)step.x / rayDir.x;
    }
    else
    {
        tMax.x = FLT_MAX;
        tDelta.x = FLT_MAX;
    }
    
    if (fabsf(rayDir.y) > 1e-6f)
    {
        float nextBoundary = (rayDir.y > 0) ? ceilf(start.y) : floorf(start.y);
        tMax.y = (nextBoundary - start.y) / rayDir.y;
        tDelta.y = (float)step.y / rayDir.y;
    }
    else
    {
        tMax.y = FLT_MAX;
        tDelta.y = FLT_MAX;
    }
    
    if (fabsf(rayDir.z) > 1e-6f)
    {
        float nextBoundary = (rayDir.z > 0) ? ceilf(start.z) : floorf(start.z);
        tMax.z = (nextBoundary - start.z) / rayDir.z;
        tDelta.z = (float)step.z / rayDir.z;
    }
    else
    {
        tMax.z = FLT_MAX;
        tDelta.z = FLT_MAX;
    }
    
    float currentT = 0.0f;
    Direction lastFace = NUM_DIRECTIONS;
    
    while (currentT < maxDistance)
    {
        if (iter.IsValid() && IsSolid(iter.GetBlock()->m_typeIndex))
        {
            result.m_didImpact = true;
            result.m_impactDist = currentT;
            result.m_impactPos = start + rayDir * currentT;
            
            switch (lastFace)
            {
            case DIRECTION_EAST:  result.m_impactNormal = Vec3(-1, 0, 0); break;
            case DIRECTION_WEST:  result.m_impactNormal = Vec3(1, 0, 0); break;
            case DIRECTION_NORTH: result.m_impactNormal = Vec3(0, -1, 0); break;
            case DIRECTION_SOUTH: result.m_impactNormal = Vec3(0, 1, 0); break;
            case DIRECTION_UP:    result.m_impactNormal = Vec3(0, 0, -1); break;
            case DIRECTION_DOWN:  result.m_impactNormal = Vec3(0, 0, 1); break;
            default:              result.m_impactNormal = -rayDir; break;
            }
            
            result.m_hitBlock = iter;
            result.m_hitFace = lastFace;
            result.m_hitLocalCoords = iter.GetLocalCoords();
            result.m_hitGlobalCoords = iter.GetGlobalCoords();
            
            return result;
        }
        
        // 移动到下一个方块
        Direction moveDir;
        
        if (tMax.x < tMax.y && tMax.x < tMax.z)
        {
            // X 轴最近
            currentT = tMax.x;
            tMax.x += tDelta.x;
            lastFace = (step.x > 0) ? DIRECTION_WEST : DIRECTION_EAST;
            moveDir = (step.x > 0) ? DIRECTION_EAST : DIRECTION_WEST;
        }
        else if (tMax.y < tMax.z)
        {
            // Y 轴最近
            currentT = tMax.y;
            tMax.y += tDelta.y;
            lastFace = (step.y > 0) ? DIRECTION_SOUTH : DIRECTION_NORTH;
            moveDir = (step.y > 0) ? DIRECTION_NORTH : DIRECTION_SOUTH;
        }
        else
        {
            // Z 轴最近
            currentT = tMax.z;
            tMax.z += tDelta.z;
            lastFace = (step.z > 0) ? DIRECTION_DOWN : DIRECTION_UP;
            moveDir = (step.z > 0) ? DIRECTION_UP : DIRECTION_DOWN;
        }
        
        // 使用 BlockIterator 移动（自动跨 Chunk）
        iter = iter.GetNeighborCrossBoundary(moveDir);
        
        if (!iter.IsValid())
            break;
    }
    
    return result;
}

void World::OnBlockPlaced(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    
    Block* b = block.GetBlock();
    if (!b)
        return;
    
    if (IsRedstoneComponent(b->m_typeIndex) || IsRedstonePowerable(b->m_typeIndex))
    {
        if (m_redstoneSimulator)
            m_redstoneSimulator->OnBlockPlaced(block);
    }
}

void World::OnBlockRemoved(const BlockIterator& block, uint8_t oldType)
{
    if (!block.IsValid())
        return;
    if (IsRedstoneComponent(oldType) || IsRedstonePowerable(oldType))
    {
        if (m_redstoneSimulator)
            m_redstoneSimulator->OnBlockRemoved(block, oldType);
    }
}

void World::OnBlockStateChanged(const BlockIterator& block, uint8_t oldType, uint8_t newType)
{
    if (!block.IsValid())
        return;
    
    if (m_redstoneSimulator)
    {
        m_redstoneSimulator->OnBlockStateChanged(block, oldType, newType);
    }
}

bool World::RegenerateSingleNearestDirtyChunk()
{
    //if (!m_hasDirtyChunk)
     //   return false;
    const Vec3 cam = m_owner->m_player->m_position;
    Chunk* nearestDirty = nullptr;
    float nearestDist2 = FLT_MAX;

    for (auto& [chunkCoords, chunk] : m_activeChunks)
    {
        if (!chunk->m_isDirty)
            continue;

        IntVec2 intChunkCenter = GetChunkCenter(chunkCoords);
        Vec2 chunkCenter = Vec2((float)intChunkCenter.x, (float)intChunkCenter.y);
        float dist2 = GetDistanceSquared2D(chunkCenter, Vec2(cam.x, cam.y));
        
        if (dist2 < nearestDist2)
        {
            nearestDist2 = dist2;
            nearestDirty = chunk;
        }
    }
    
    if (nearestDirty)
    {
        nearestDirty->GenerateMesh();
        return true;
    }
    return false;
}

bool World::ActivateSingleNearestMissingChunkWithinRange()
{
    const Vec3 cam = m_owner->m_player->m_position;
    const IntVec2 camChunkCoords = WorldToChunkXY(cam);

    IntVec2 bestChunkCoords;
    float bestDist2 = FLT_MAX;
    bool foundCandidate = false;

    for (int dy = -CHUNK_ACTIVATION_RADIUS_Y; dy <= CHUNK_ACTIVATION_RADIUS_Y; ++dy)
    {
        for (int dx = -CHUNK_ACTIVATION_RADIUS_X; dx <= CHUNK_ACTIVATION_RADIUS_X; ++dx)
        {
            IntVec2 candidateCoords(camChunkCoords.x + dx, camChunkCoords.y + dy);
            
            if (m_activeChunks.find(candidateCoords) != m_activeChunks.end())
                continue;
            
            Vec2 chunkCenter = Vec2((float)GetChunkCenter(candidateCoords).x, (float)GetChunkCenter(candidateCoords).y);
            float dist2 = GetDistanceSquared2D(chunkCenter, Vec2(cam.x, cam.y));
            
            if (dist2 <= (float)(CHUNK_ACTIVATION_RANGE * CHUNK_ACTIVATION_RANGE))
            {
                if (dist2 < bestDist2)
                {
                    bestDist2 = dist2;
                    bestChunkCoords = candidateCoords;
                    foundCandidate = true;
                }
            }
        }
    }

    if (!foundCandidate)
        return false;
    
    ActivateChunk(bestChunkCoords);
    return true;
}

void World::DeactivateSingleFarthestOutsideRangeIfAny()
{
    const Vec3 cam = m_owner->m_player->m_position;
    Chunk* farthestChunk = nullptr;
    IntVec2 farthestCoords;
    float farthestDist2 = -1.0f;
    
    for (auto& [chunkCoords, chunk] : m_activeChunks)
    {
        IntVec2 intChunkCenter = GetChunkCenter(chunkCoords);
        Vec2 chunkCenter = Vec2((float)intChunkCenter.x, (float)intChunkCenter.y);
        float dist2 = GetDistanceSquared2D(chunkCenter, Vec2(cam.x, cam.y));
        
        if (dist2 > (float)(CHUNK_DEACTIVATION_RANGE * CHUNK_DEACTIVATION_RANGE))
        {
            if (dist2 > farthestDist2)
            {
                farthestDist2 = dist2;
                farthestChunk = chunk;
                farthestCoords = chunkCoords;
            }
        }
    }
    if (farthestChunk)
    {
        DeactivateChunk(farthestCoords);
    }
}

void World::ActivateChunk(IntVec2 chunkCoords)
{
    Chunk* newChunk = new Chunk(this, chunkCoords);
    
    std::string filename = Chunk::MakeChunkFilename(chunkCoords);
    bool loadedFromDisk = false;
    
    if (g_theSaveSystem && g_theSaveSystem->FileExists(filename))
    {
        newChunk->m_serializer = new ChunkSerializer(newChunk);
        loadedFromDisk = newChunk->Load();
    }
    if (!loadedFromDisk)
    {
        newChunk->GenerateBlocks();
        if (!newChunk->m_serializer)
            newChunk->m_serializer = new ChunkSerializer(newChunk);
    }
    newChunk->m_isDirty = true;
    newChunk->m_needsSaving = true;
    
    m_activeChunks[chunkCoords] = newChunk;

    ConnectChunkNeighbors(newChunk);
}

void World::DeactivateChunk(IntVec2 chunkCoords)
{
    auto it = m_activeChunks.find(chunkCoords);
    if (it == m_activeChunks.end())
        return;
        
    Chunk* chunk = it->second;
    UndirtyAllBlocksInChunk(chunk);
    DisconnectChunkNeighbors(chunk);
    m_activeChunks.erase(it);
    
    if (chunk->m_needsSaving)
    {
        //chunk->Save();
        
        chunk->SetState(ChunkState::QUEUED_FOR_SAVING);
        SaveChunkJob* job = new SaveChunkJob(chunk);
        g_theJobSystem->AddPendingJob(job);
    }
    else
    {
        delete chunk;
    }
    //delete chunk;
}

void World::ConnectChunkNeighbors(Chunk* chunk)
{
    if (!chunk) return;
    IntVec2 coords = chunk->m_chunkCoords;
    
    IntVec2 northCoords(coords.x, coords.y + 1);
    auto northIt = m_activeChunks.find(northCoords);
    if (northIt != m_activeChunks.end() && northIt->second)
    {
        Chunk* northChunk = northIt->second;
        chunk->SetNeighbor(DIRECTION_NORTH, northChunk);
        northChunk->SetNeighbor(DIRECTION_SOUTH, chunk);
        northChunk->m_isDirty = true;
    }
    
    IntVec2 southCoords(coords.x, coords.y - 1);
    auto southIt = m_activeChunks.find(southCoords);
    if (southIt != m_activeChunks.end() && southIt->second)
    {
        Chunk* southChunk = southIt->second;
        chunk->SetNeighbor(DIRECTION_SOUTH, southChunk);
        southChunk->SetNeighbor(DIRECTION_NORTH, chunk);
        southChunk->m_isDirty = true;
    }
    
    IntVec2 eastCoords(coords.x + 1, coords.y);
    auto eastIt = m_activeChunks.find(eastCoords);
    if (eastIt != m_activeChunks.end() && eastIt->second)
    {
        Chunk* eastChunk = eastIt->second;
        chunk->SetNeighbor(DIRECTION_EAST, eastChunk);
        eastChunk->SetNeighbor(DIRECTION_WEST, chunk);
        eastChunk->m_isDirty = true;
    }
    
    IntVec2 westCoords(coords.x - 1, coords.y);
    auto westIt = m_activeChunks.find(westCoords);
    if (westIt != m_activeChunks.end() && westIt->second)
    {
        Chunk* westChunk = westIt->second;
        chunk->SetNeighbor(DIRECTION_WEST, westChunk);
        westChunk->SetNeighbor(DIRECTION_EAST, chunk);
        westChunk->m_isDirty = true;
    }
    
    chunk->m_isDirty = true;
}

void World::DisconnectChunkNeighbors(Chunk* chunk)
{
    if (!chunk) return;
    
    Chunk* northNeighbor = chunk->GetNeighbor(DIRECTION_NORTH);
    if (northNeighbor)
    {
        northNeighbor->SetNeighbor(DIRECTION_SOUTH, nullptr);
        chunk->SetNeighbor(DIRECTION_NORTH, nullptr);
        northNeighbor->m_isDirty = true;
    }
    
    Chunk* southNeighbor = chunk->GetNeighbor(DIRECTION_SOUTH);
    if (southNeighbor)
    {
        southNeighbor->SetNeighbor(DIRECTION_NORTH, nullptr);
        chunk->SetNeighbor(DIRECTION_SOUTH, nullptr);
        southNeighbor->m_isDirty = true;
    }
    
    Chunk* eastNeighbor = chunk->GetNeighbor(DIRECTION_EAST);
    if (eastNeighbor)
    {
        eastNeighbor->SetNeighbor(DIRECTION_WEST, nullptr);
        chunk->SetNeighbor(DIRECTION_EAST, nullptr);
        eastNeighbor->m_isDirty = true;
    }
    
    Chunk* westNeighbor = chunk->GetNeighbor(DIRECTION_WEST);
    if (westNeighbor)
    {
        westNeighbor->SetNeighbor(DIRECTION_EAST, nullptr);
        chunk->SetNeighbor(DIRECTION_WEST, nullptr);
        westNeighbor->m_isDirty = true;
    }
}

void World::ForceDeactivateAllChunks()
{
    std::vector<IntVec2> coordsToDeactivate;
    for (const auto& [coords, chunk] : m_activeChunks)
    {
        coordsToDeactivate.push_back(coords);
    }
    for (const IntVec2& coords : coordsToDeactivate)
    {
        DeactivateChunk(coords);
    }
}

void World::SaveAllModifiedChunks()
{
    for (auto& [coords, chunk] : m_activeChunks)
    {
        // if (chunk->m_needsSaving)
        // {
        //     chunk->Save();
        // }
        chunk->Save();
    }
}

void World::ActivateProcessedChunk(Chunk* chunk)
{
    IntVec2 coords = chunk->GetThisChunkCoords();

    std::lock_guard<std::mutex> lock(m_processingChunksMutex);
    m_processingChunks.erase(coords);

    if (chunk)
    {
		m_activeChunks[coords] = chunk;
		chunk->SetState(ChunkState::ACTIVE);
        //DebuggerPrintf("Activating Chunk (%d, %d)\n", coords.x, coords.y);

		ConnectChunkNeighbors(chunk);

        /*int neighborCount = 0;
        if (chunk->m_eastNeighbor) neighborCount++;
        if (chunk->m_westNeighbor) neighborCount++;
        if (chunk->m_northNeighbor) neighborCount++;
        if (chunk->m_southNeighbor) neighborCount++;*/
    
       // DebuggerPrintf("  Neighbors connected: %d/4\n", neighborCount);

        chunk->InitializeLighting();

        // 同时重新初始化所有已连接邻居的光照
        if (chunk->m_eastNeighbor) chunk->m_eastNeighbor->InitializeLighting();
        if (chunk->m_westNeighbor) chunk->m_westNeighbor->InitializeLighting();
        if (chunk->m_northNeighbor) chunk->m_northNeighbor->InitializeLighting();
        if (chunk->m_southNeighbor) chunk->m_southNeighbor->InitializeLighting();
		chunk->m_isDirty = true;
		m_hasDirtyChunk = true;
    }
}

void World::ProcessCompletedJobs()
{
    std::vector<Job*> completedJobs = g_theJobSystem->RetrieveCompletedJobs();
    
    // 第一步：批量激活所有chunk，但先不连接邻居
    std::vector<Chunk*> newlyActivatedChunks;
    m_generatingChunksCount = 0;
    for (Job* job : completedJobs)
    {
        Chunk* chunk = dynamic_cast<ChunkJob*>(job)->m_chunk;   
        if (chunk)
        {
            IntVec2 coords = chunk->GetThisChunkCoords();
            
            std::lock_guard<std::mutex> lock(m_processingChunksMutex);
            m_processingChunks.erase(coords);
            
            m_activeChunks[coords] = chunk;
            chunk->SetState(ChunkState::ACTIVE);
            
            newlyActivatedChunks.push_back(chunk);
            
            //DebuggerPrintf("Activating Chunk (%d, %d)\n", coords.x, coords.y);
        }
        
        delete job;
    }
    
    // 第二步：现在所有新chunk都在activeChunks中了，批量连接邻居
    m_generatingChunksCount = (int)newlyActivatedChunks.size();
    for (Chunk* chunk : newlyActivatedChunks)
    {
        ConnectChunkNeighbors(chunk);
        chunk->m_isDirty = true;
        m_hasDirtyChunk = true;
    }
    for (Chunk* chunk : newlyActivatedChunks)
    {
        chunk->InitializeLighting();
    }
}

void World::SubmitNewActivateJobs()
{
	std::lock_guard<std::mutex> lock(m_processingChunksMutex);
	int currentJobCount = (int)m_processingChunks.size();
	if (currentJobCount >= MAX_CONCURRENT_JOBS)
	{
		return;
	}

	Vec3 playerPos = m_owner->m_player->m_position;
	IntVec2 playerChunkCoords = WorldToChunkXY(playerPos);

	std::vector<IntVec2> chunksToActivate;

	for (int dy = -CHUNK_ACTIVATION_RADIUS_Y; dy <= CHUNK_ACTIVATION_RADIUS_Y; ++dy)
	{
		for (int dx = -CHUNK_ACTIVATION_RADIUS_X; dx <= CHUNK_ACTIVATION_RADIUS_X; ++dx)
		{
			IntVec2 coords(playerChunkCoords.x + dx, playerChunkCoords.y + dy);

			if (m_activeChunks.find(coords) != m_activeChunks.end())
				continue;

			// Remove this lock
			// std::lock_guard<std::mutex> lock(m_processingChunksMutex);
			if (m_processingChunks.find(coords) != m_processingChunks.end())
				continue;

			Vec2 chunkCenter = Vec2((float)GetChunkCenter(coords).x,
				(float)GetChunkCenter(coords).y);
			float dist2 = GetDistanceSquared2D(chunkCenter, Vec2(playerPos.x, playerPos.y));

			if (dist2 <= (float)(CHUNK_ACTIVATION_RANGE * CHUNK_ACTIVATION_RANGE))
			{
				chunksToActivate.push_back(coords);
			}
		}
	}

	std::sort(chunksToActivate.begin(), chunksToActivate.end(),
		[playerPos](const IntVec2& a, const IntVec2& b)
		{
			Vec2 aCenter((float)GetChunkCenter(a).x, (float)GetChunkCenter(a).y);
			Vec2 bCenter((float)GetChunkCenter(b).x, (float)GetChunkCenter(b).y);
			float aDist2 = GetDistanceSquared2D(aCenter, Vec2(playerPos.x, playerPos.y));
			float bDist2 = GetDistanceSquared2D(bCenter, Vec2(playerPos.x, playerPos.y));
			return aDist2 < bDist2;
		});

	for (const IntVec2& coords : chunksToActivate)
	{
		if (currentJobCount >= MAX_CONCURRENT_JOBS) break;

		Chunk* chunk = new Chunk(this, coords);

		// already locked
		m_processingChunks[coords] = chunk;

		std::string filename = Chunk::MakeChunkFilename(coords);
		if (g_theSaveSystem && g_theSaveSystem->FileExists(filename))
		{
			chunk->SetState(ChunkState::QUEUED_FOR_LOADING);
			g_theJobSystem->AddPendingJob(new LoadChunkJob(chunk));
		}
		else
		{
			chunk->SetState(ChunkState::QUEUED_FOR_GENERATION);
			g_theJobSystem->AddPendingJob(new GenerateChunkJob(chunk));
		}

		currentJobCount++;
	}
}

void World::RebuildDirtyMeshes(int maxPerFrame)
{
    if (!m_hasDirtyChunk) return;
    
    int rebuilt = 0;

    for (auto& [coords, chunk] : m_activeChunks)
    {
        if (rebuilt >= maxPerFrame)
            break;
        if (chunk->m_needsImmediateRebuild)
        {
            if (chunk->GenerateMesh())
                rebuilt++;
        }
    }

    if (rebuilt < maxPerFrame)
    {
        std::vector<Chunk*> dirtyChunks;
        for (auto& [coords, chunk] : m_activeChunks)
        {
            if (chunk->m_isDirty && chunk->GetState() == ChunkState::ACTIVE)
            {
                dirtyChunks.push_back(chunk);
            }
        }
    
        if (dirtyChunks.empty())
        {
            m_hasDirtyChunk = false;
            return;
        }
    
        Vec3 playerPos = m_owner->m_player->m_position;
        std::sort(dirtyChunks.begin(), dirtyChunks.end(), 
            [playerPos](Chunk* a, Chunk* b)
            {
                Vec2 aCenter((float)GetChunkCenter(a->GetThisChunkCoords()).x, 
                            (float)GetChunkCenter(a->GetThisChunkCoords()).y);
                Vec2 bCenter((float)GetChunkCenter(b->GetThisChunkCoords()).x, 
                            (float)GetChunkCenter(b->GetThisChunkCoords()).y);
                float aDist2 = GetDistanceSquared2D(aCenter, Vec2(playerPos.x, playerPos.y));
                float bDist2 = GetDistanceSquared2D(bCenter, Vec2(playerPos.x, playerPos.y));
                return aDist2 < bDist2;
            });
    
        for (Chunk* chunk : dirtyChunks)
        {
            if (rebuilt >= maxPerFrame)
                break;

            if (chunk->GenerateMesh())
                rebuilt++;
        }
    }
}

void World::ComputeCorrectLightInfluence(const BlockIterator& iter, uint8_t& outOutdoorLight, uint8_t& outIndoorLight)
{
    outOutdoorLight = 0;
    outIndoorLight = 0;
    
    Block* block = iter.GetBlock();
    if (!block)
        return;
    
    if (block->IsSky())
    {
        outOutdoorLight = 15;
    }
    
    BlockDefinition const& def = BlockDefinition::GetBlockDef(block->m_typeIndex);

    if (def.m_indoorLightInfluence > 0)
    {
        outIndoorLight = def.m_indoorLightInfluence;
    }
    if (def.m_outdoorLightInfluence > 0)
    {
        outOutdoorLight = def.m_outdoorLightInfluence;
    }
    
    // 规则3：如果方块非不透明，从邻居传播光照
    if (!block->IsOpaque())
    {
        uint8_t maxNeighborOutdoor = 0;
        uint8_t maxNeighborIndoor = 0;
        
        // 检查六个方向的邻居
        for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
        {
            BlockIterator neighbor = iter.GetNeighborCrossBoundary((Direction)dir);
            if (neighbor.IsValid())
            {
                uint8_t neighborOutdoor = neighbor.GetOutdoorLight();
                uint8_t neighborIndoor = neighbor.GetIndoorLight();
                
                if (neighborOutdoor > maxNeighborOutdoor)
                    maxNeighborOutdoor = neighborOutdoor;
                if (neighborIndoor > maxNeighborIndoor)
                    maxNeighborIndoor = neighborIndoor;
            }
        }
        
        // 光照传播会衰减 1 级
        if (maxNeighborOutdoor > 0)
        {
            uint8_t propagatedOutdoor = maxNeighborOutdoor - 1;
            if (propagatedOutdoor > outOutdoorLight)
                outOutdoorLight = propagatedOutdoor;
        }
        
        if (maxNeighborIndoor > 0)
        {
            uint8_t propagatedIndoor = maxNeighborIndoor - 1;
            if (propagatedIndoor > outIndoorLight)
                outIndoorLight = propagatedIndoor;
        }
    }
}

void World::BindWorldConstansBuffer() const
{
    WorldConstants constants;
    constants.CameraWorldPosition[0] = m_owner->m_player->m_worldCamera.GetPosition().x;
    constants.CameraWorldPosition[1] = m_owner->m_player->m_worldCamera.GetPosition().y;
    constants.CameraWorldPosition[2] = m_owner->m_player->m_worldCamera.GetPosition().z;
    float skyColor[4];
    m_skyColor.GetAsFloats(skyColor);
    constants.SkyColor[0] = skyColor[0];
    constants.SkyColor[1] = skyColor[1];
    constants.SkyColor[2] = skyColor[2];
    constants.SkyColor[3] = skyColor[3];
    float outdoorColor[4];
    m_outdoorLightColor.GetAsFloats(outdoorColor);
    constants.OutdoorLightColor[0] = outdoorColor[0];
    constants.OutdoorLightColor[1] = outdoorColor[1];
    constants.OutdoorLightColor[2] = outdoorColor[2];
    constants.OutdoorLightColor[3] = outdoorColor[3];
    float indoorColor[4];
    m_indoorLightColor.GetAsFloats(indoorColor);
    constants.IndoorLightColor[0] = indoorColor[0];
    constants.IndoorLightColor[1] = indoorColor[1];
    constants.IndoorLightColor[2] = indoorColor[2];
    constants.IndoorLightColor[3] = indoorColor[3];

    constants.FogNearDistance = (float)(CHUNK_ACTIVATION_RANGE - 2 * CHUNK_SIZE_X) / 2.0f;
    constants.FogFarDistance = (float)(CHUNK_ACTIVATION_RANGE - 2 * CHUNK_SIZE_X);
    //constants.FogNearDistance = 200.0f;   // 从 200 开始出现雾
    //constants.FogFarDistance = 300.0f;  
    constants.FogMaxAlpha = 1.0f;

    g_theRenderer->CopyCPUToGPU(&constants, sizeof(WorldConstants), m_worldConstantBuffer);
    g_theRenderer->BindConstantBuffer(k_worldConstantsSlot, m_worldConstantBuffer);
}

void World::ToggleDebugMode()
{
    m_isDebugging = !m_isDebugging;
}

void World::ToggleDebugPrintingMode()
{
    m_isDebugPrinting = !m_isDebugPrinting;
}

bool World::IsDebugging() const
{
    return m_isDebugging;
}

bool World::IsDebuggingPrinting() const
{
    return m_isDebugPrinting;
}

void World::UpdateTypeToPlace()
{
    if (g_theApp->WasKeyJustPressed('1'))
    {
        m_typeToPlace = BLOCK_TYPE_GLOWSTONE;
    }
    else if (g_theApp->WasKeyJustPressed('2'))
    {
        m_typeToPlace = BLOCK_TYPE_COBBLESTONE;
    }
    else if (g_theApp->WasKeyJustPressed('3'))
    {
        m_typeToPlace = BLOCK_TYPE_CHISELED_BRICK;
    }
}

void World::UpdateDiggingAndPlacing(float deltaSeconds)
{
    if (g_theInput->WasKeyJustPressed('R'))
    {
        m_isRaycastLocked = !m_isRaycastLocked;
    }
    if (m_isRaycastLocked)
    {
        m_highlightedBlock.m_isValid = false;
        Chunk* chunkToUpdate = GetChunkFromPlayerCameraPosition(m_owner->m_player->m_position);
        if(chunkToUpdate)
            chunkToUpdate->Update(deltaSeconds); //只有Dig TODO：改写成更好的方式
        return;
    }
    
    Vec3 cameraPos = m_owner->m_player->m_gameCamera->GetPosition();
    Vec3 cameraForward;
    Vec3 cameraLeft;
    Vec3 cameraUp;
    m_owner->m_player->m_gameCamera->GetOrientation().GetAsVectors_IFwd_JLeft_KUp(cameraForward, cameraLeft, cameraUp);
    float maxRaycastDistance = 10.0f; 
    
    m_currentRaycast = RaycastVsBlocks(cameraPos, cameraForward, maxRaycastDistance);

    if (m_currentRaycast.m_didImpact)
    {
        m_highlightedBlock.m_isValid = true;
        m_highlightedBlock.m_worldCoords = m_currentRaycast.m_hitGlobalCoords;
        m_highlightedBlock.m_hitFace = m_currentRaycast.m_hitFace;
    }
    
    if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
    {
        //if (m_currentRaycast.m_didImpact)
        if ( m_highlightedBlock.m_isValid)
        {
            //DebuggerPrintf("Impacted! Digging");
            Chunk* chunk = m_currentRaycast.m_hitBlock.GetChunk();
            if (chunk)
            {
                chunk->DigBlock(m_currentRaycast.m_hitLocalCoords);
            }
        }
    }
    
    if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
    {
        //if (m_currentRaycast.m_didImpact)
        if (m_highlightedBlock.m_isValid)
        {
            Block* targetBlock = m_currentRaycast.m_hitBlock.GetBlock();
            
            if (targetBlock)
            {
                uint8_t blockType = targetBlock->m_typeIndex;
                
                // 红石组件交互
                if (blockType == BLOCK_TYPE_LEVER)
                {
                    m_redstoneSimulator->ToggleLever(m_currentRaycast.m_hitBlock);
                    return;  // 不放置方块
                }
                else if (blockType == BLOCK_TYPE_BUTTON_STONE || 
                         blockType == BLOCK_TYPE_BUTTON_WOOD)
                {
                    m_redstoneSimulator->PressButton(m_currentRaycast.m_hitBlock);
                    return;
                }
                else if (blockType == BLOCK_TYPE_REPEATER_OFF ||
                         blockType == BLOCK_TYPE_REPEATER_ON)
                {
                    m_redstoneSimulator->CycleRepeaterDelay(m_currentRaycast.m_hitBlock);
                    return;
                }
            }
            
            BlockIterator placeIter = m_currentRaycast.m_hitBlock.GetNeighborCrossBoundary(m_currentRaycast.m_hitFace);
        
            if (placeIter.IsValid())
            {
                Block* targetBlock = placeIter.GetBlock();
            
                if (targetBlock && targetBlock->m_typeIndex == BLOCK_TYPE_AIR)
                {
                    Chunk* chunk = placeIter.GetChunk();
                    IntVec3 placeCoords = placeIter.GetLocalCoords();
                
                    chunk->PlaceBlock(placeCoords, m_typeToPlace);
                }
            }
        }
    }
}

void World::UpdateAccelerateTime()
{
    if (g_theInput->IsKeyDown('Y'))
    {
        m_isTimeAccelerated = true;
    }
    else
    {
        m_isTimeAccelerated = false;
    }
}

void World::UpdateVisibleChunks()
{
    // m_visibleChunks.clear();
    // Vec3 camPos = m_owner->m_player->m_worldCamera.GetPosition();
    //
    // for (auto& chunkPair : m_activeChunks)
    // {
    //     Chunk* chunk = chunkPair.second;
    //
    //     if (chunk->GetState() != ChunkState::ACTIVE)
    //         continue;
    //
    //     // 临时：只用距离剔除，不用 frustum culling
    //     Vec3 chunkCenter = (chunk->m_bounds.m_mins + chunk->m_bounds.m_maxs) * 0.5f;
    //     float distToCam = (chunkCenter - camPos).GetLength();
    //     
    //     if (distToCam < 300.0f) // 调整这个值
    //     {
    //         m_visibleChunks.push_back(chunk);
    //     }
    // }
    m_visibleChunks.clear();
    Frustum viewF = m_owner->m_player->m_worldCamera.GetFrustum();
	for (auto& chunkPair : m_activeChunks)
	{
		Chunk* chunk = chunkPair.second;
 
		if (chunk->GetState() != ChunkState::ACTIVE)
			continue;
 
		if (!viewF.IsAABBOutside(chunk->m_bounds))
		{
			m_visibleChunks.push_back(chunk);
		}
	}
}
