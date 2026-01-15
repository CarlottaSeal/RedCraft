#include "CropDropManager.h"
#include "CropDefinitions.h"
#include "Game/Gamecommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include <random>

#include "Game/UI/IconAtlas.h"

void CropDropManager::Update(float deltaSeconds)
{
    // 更新每个掉落物
    for (int i = (int)m_drops.size() - 1; i >= 0; i--)
    {
        PendingDrop& drop = m_drops[i];
        
        // 更新生命周期
        drop.m_lifetime -= deltaSeconds;
        if (drop.m_lifetime <= 0.0f)
        {
            m_drops.erase(m_drops.begin() + i);
            continue;
        }
        
        // 更新动画
        drop.m_bobTimer += deltaSeconds * 2.0f;  // 浮动频率
        drop.m_rotationAngle += deltaSeconds * 90.0f;  // 旋转速度（度/秒）
        
        // 保持旋转角度在 0-360
        if (drop.m_rotationAngle >= 360.0f)
            drop.m_rotationAngle -= 360.0f;
    }
}

void CropDropManager::Render(Renderer* renderer, Camera const& camera, IconAtlas* uiAtlas) const
{
    if (m_drops.empty() || !uiAtlas)
        return;
    
    std::vector<Vertex_PCU> verts;
    verts.reserve(m_drops.size() * 6);  // 每个 billboard 6 顶点（2三角形）
    
    Vec3 forward,left,up;
    camera.GetOrientation().GetAsVectors_IFwd_JLeft_KUp(forward, left, up);
    Vec3 camForward = forward;
    Vec3 camRight = -left;
    Vec3 camUp = up;
    
    for (const PendingDrop& drop : m_drops)
    {
        // 1. 浮动动画
        float bobOffset = sinf(drop.m_lifetime * 4.0f) * 0.05f;
        Vec3 center = drop.m_position + Vec3(0, 0, 0.25f + bobOffset);
        
        // 2. Billboard 尺寸
        float halfSize = 0.2f;
        
        // 3. 计算四个角
        Vec3 bottomLeft  = center - camRight * halfSize - camUp * halfSize;
        Vec3 bottomRight = center + camRight * halfSize - camUp * halfSize;
        Vec3 topRight    = center + camRight * halfSize + camUp * halfSize;
        Vec3 topLeft     = center - camRight * halfSize + camUp * halfSize;
        
        // 4. 获取 UV（根据作物类型）
        AABB2 uvs = GetDropUVs(drop.m_itemType, uiAtlas);
        
        // 5. 添加两个三角形
        Rgba8 color = Rgba8::WHITE;
        
        // 闪烁效果（快消失时）
        if (drop.m_lifetime - drop.m_lifetime < 10.0f)
        {
            float flash = sinf(drop.m_lifetime * 10.0f);
            color.a = (flash > 0) ? 255 : 100;
        }
        
        verts.push_back(Vertex_PCU(bottomLeft,  color, Vec2(uvs.m_mins.x, uvs.m_mins.y)));
        verts.push_back(Vertex_PCU(bottomRight, color, Vec2(uvs.m_maxs.x, uvs.m_mins.y)));
        verts.push_back(Vertex_PCU(topRight,    color, Vec2(uvs.m_maxs.x, uvs.m_maxs.y)));
        
        verts.push_back(Vertex_PCU(bottomLeft,  color, Vec2(uvs.m_mins.x, uvs.m_mins.y)));
        verts.push_back(Vertex_PCU(topRight,    color, Vec2(uvs.m_maxs.x, uvs.m_maxs.y)));
        verts.push_back(Vertex_PCU(topLeft,     color, Vec2(uvs.m_mins.x, uvs.m_maxs.y)));
    }
    
    if (!verts.empty())
    {
        renderer->SetBlendMode(BlendMode::ALPHA);
        renderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
        renderer->BindTexture(uiAtlas->GetTexture());
        renderer->DrawVertexArray(verts);
    }
}

AABB2 CropDropManager::GetDropUVs(uint8_t cropType, IconAtlas* atlas) const
{
    int iconIndex = 0;
    
    // switch (cropType)
    // {
    // case BLOCK_TYPE_WHEAT_7:    iconIndex = UIIcons::CROP_WHEAT;   break;
    // case BLOCK_TYPE_CARROTS_3:  iconIndex = UIIcons::CROP_CARROT;  break;
    // case BLOCK_TYPE_POTATOES_3: iconIndex = UIIcons::CROP_POTATO;  break;
    // case BLOCK_TYPE_BEETROOTS_3:iconIndex = UIIcons::CROP_BEETROOT;break;
    // default: iconIndex = 0; break;
    // }

    return atlas->GetBlockIconUVs(cropType);
    return atlas->GetGridSpriteUVsByIndex(iconIndex);
}

void CropDropManager::SpawnDrop(const Vec3& position, uint8_t itemType, int count)
{
    if (count <= 0)
        return;
    
    PendingDrop drop;
    drop.m_position = position;
    drop.m_itemType = itemType;
    drop.m_count = count;
    drop.m_lifetime = 300.0f;  // 5分钟
    
    // 随机初始动画状态
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.0f, 6.28f);
    drop.m_bobTimer = dist(rng);
    drop.m_rotationAngle = dist(rng) * 57.3f;  // 转为度
    
    // 添加一点随机偏移，避免堆叠在一起
    std::uniform_real_distribution<float> offsetDist(-0.2f, 0.2f);
    drop.m_position.x += offsetDist(rng);
    drop.m_position.y += offsetDist(rng);
    
    m_drops.push_back(drop);
}

void CropDropManager::SpawnDropAtBlock(const IntVec3& blockPos, std::string itemName, int count)
{
    Vec3 worldPos(
        (float)blockPos.x + 0.5f,
        (float)blockPos.y + 0.5f,
        (float)blockPos.z + 0.5f
    );
    ItemType itemType = StringToItemType(itemName);
    SpawnDrop(worldPos, itemType, count);
}

void CropDropManager::SpawnDropAtBlock(const IntVec3& blockPos, ItemType itemType, int count)
{
    Vec3 worldPos(
        (float)blockPos.x + 0.5f,
        (float)blockPos.y + 0.5f,
        (float)blockPos.z + 0.5f
    );
    SpawnDrop(worldPos, itemType, count);
}

void CropDropManager::SpawnCropDrops(const IntVec3& blockPos, ItemType cropType)
{
    const CropDefinition* def = CropDefinition::GetDefinition(cropType);
    if (!def)
    {
        // 未知作物，掉落自身
        SpawnDropAtBlock(blockPos, cropType, 1);
        return;
    }
    
    int stageIndex = def->GetStageIndex(cropType);
    if (stageIndex < 0 || stageIndex >= (int)def->m_stages.size())
    {
        SpawnDropAtBlock(blockPos, cropType, 1);
        return;
    }
    
    const CropStage& stage = def->m_stages[stageIndex];
    
    // 处理该阶段的掉落物
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
    
    for (const CropDrop& drop : stage.m_drops)
    {
        // 检查是否只在成熟时掉落
        if (drop.m_onlyWhenMature && !stage.m_isMature)
            continue;
        
        // 随机决定是否掉落
        if (chanceDist(rng) <= drop.m_chance)
        {
            int count = drop.RollCount();
            if (count > 0)
            {
                SpawnDropAtBlock(blockPos, drop.m_itemName, count);
            }
        }
    }
    
    // 如果不是成熟阶段且没有掉落物，至少掉落种子
    // if (!stage.m_isMature && stage.m_drops.empty() && !def->m_stages.empty())
    // {
    //     // 第一阶段的方块类型通常就是种子
    //     SpawnDropAtBlock(blockPos, def->m_stages[0].m_blockType, 1);
    // }
}

std::vector<PendingDrop> CropDropManager::CollectDropsNear(const Vec3& position, float radius)
{
    std::vector<PendingDrop> collected;
    float radiusSq = radius * radius;
    
    for (int i = (int)m_drops.size() - 1; i >= 0; i--)
    {
        Vec3 diff = m_drops[i].m_position - position;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        
        if (distSq <= radiusSq)
        {
            collected.push_back(m_drops[i]);
            m_drops.erase(m_drops.begin() + i);
        }
    }
    
    return collected;
}

std::vector<PendingDrop> CropDropManager::CollectDropsInArea(const IntVec3& minCorner, const IntVec3& maxCorner)
{
    std::vector<PendingDrop> collected;
    
    for (int i = (int)m_drops.size() - 1; i >= 0; i--)
    {
        const Vec3& pos = m_drops[i].m_position;
        
        if (pos.x >= (float)minCorner.x && pos.x <= (float)maxCorner.x + 1.0f &&
            pos.y >= (float)minCorner.y && pos.y <= (float)maxCorner.y + 1.0f &&
            pos.z >= (float)minCorner.z && pos.z <= (float)maxCorner.z + 1.0f)
        {
            collected.push_back(m_drops[i]);
            m_drops.erase(m_drops.begin() + i);
        }
    }
    
    return collected;
}

void CropDropManager::ClearAllDrops()
{
    m_drops.clear();
}

DropStatistics CropDropManager::GetStatistics() const
{
    DropStatistics stats;
    stats.m_totalDrops = (int)m_drops.size();
    
    for (const PendingDrop& drop : m_drops)
    {
        stats.m_totalItems += drop.m_count;
        CategorizeItem(drop.m_itemType, stats);
    }
    
    return stats;
}

int CropDropManager::GetTotalItemCount() const
{
    int total = 0;
    for (const PendingDrop& drop : m_drops)
    {
        total += drop.m_count;
    }
    return total;
}

Rgba8 CropDropManager::GetDropColor(uint8_t itemType) const
{
    // 小麦 - 金黄色
    if (itemType >= BLOCK_TYPE_WHEAT_0 && itemType <= BLOCK_TYPE_WHEAT_7)
        return Rgba8(220, 180, 50, 255);
    
    // 胡萝卜 - 橙色
    if (itemType >= BLOCK_TYPE_CARROTS_0 && itemType <= BLOCK_TYPE_CARROTS_3)
        return Rgba8(255, 140, 0, 255);
    
    // 土豆 - 棕色
    if (itemType >= BLOCK_TYPE_POTATOES_0 && itemType <= BLOCK_TYPE_POTATOES_3)
        return Rgba8(180, 140, 80, 255);
    
    // 甜菜 - 深红色
    if (itemType >= BLOCK_TYPE_BEETROOTS_0 && itemType <= BLOCK_TYPE_BEETROOTS_3)
        return Rgba8(150, 40, 50, 255);
    
    // 南瓜 - 橙色
    if (itemType == BLOCK_TYPE_PUMPKIN || 
        (itemType >= BLOCK_TYPE_PUMPKIN_STEM_0 && itemType <= BLOCK_TYPE_PUMPKIN_STEM_7))
        return Rgba8(230, 120, 30, 255);
    
    // 西瓜 - 绿色
    if (itemType == BLOCK_TYPE_MELON || 
        (itemType >= BLOCK_TYPE_MELON_STEM_0 && itemType <= BLOCK_TYPE_MELON_STEM_7))
        return Rgba8(100, 180, 80, 255);
    
    // 甘蔗 - 浅绿色
    if (itemType == BLOCK_TYPE_SUGAR_CANE)
        return Rgba8(150, 220, 120, 255);
    
    // 仙人掌 - 深绿色
    if (itemType == BLOCK_TYPE_CACTUS)
        return Rgba8(60, 140, 60, 255);
    
    // 地狱疣 - 暗红色
    if (itemType >= BLOCK_TYPE_NETHER_WART_0 && itemType <= BLOCK_TYPE_NETHER_WART_3)
        return Rgba8(120, 30, 40, 255);
    
    // 海带 - 深绿色
    if (itemType == BLOCK_TYPE_KELP || itemType == BLOCK_TYPE_KELP_TOP)
        return Rgba8(40, 100, 60, 255);
    
    // 默认 - 白色
    return Rgba8(200, 200, 200, 255);
}

void CropDropManager::CategorizeItem(uint8_t itemType, DropStatistics& stats) const
{
    if (itemType >= BLOCK_TYPE_WHEAT_0 && itemType <= BLOCK_TYPE_WHEAT_7)
    {
        stats.m_wheatCount++;
    }
    else if (itemType >= BLOCK_TYPE_CARROTS_0 && itemType <= BLOCK_TYPE_CARROTS_3)
    {
        stats.m_carrotCount++;
    }
    else if (itemType >= BLOCK_TYPE_POTATOES_0 && itemType <= BLOCK_TYPE_POTATOES_3)
    {
        stats.m_potatoCount++;
    }
    else if (itemType >= BLOCK_TYPE_BEETROOTS_0 && itemType <= BLOCK_TYPE_BEETROOTS_3)
    {
        stats.m_beetrootCount++;
    }
    else if (itemType == BLOCK_TYPE_MELON || 
             (itemType >= BLOCK_TYPE_MELON_STEM_0 && itemType <= BLOCK_TYPE_MELON_STEM_7))
    {
        stats.m_melonCount++;
    }
    else if (itemType == BLOCK_TYPE_PUMPKIN || 
             (itemType >= BLOCK_TYPE_PUMPKIN_STEM_0 && itemType <= BLOCK_TYPE_PUMPKIN_STEM_7))
    {
        stats.m_pumpkinCount++;
    }
    else
    {
        stats.m_otherCount++;
    }
}