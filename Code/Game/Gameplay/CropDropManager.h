#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec3.h"
#include "Engine/Core/Rgba8.hpp"
#include <vector>
#include <cstdint>
#include <xstring>

#include "Engine/Math/AABB2.hpp"

enum ItemType : uint8_t;
class IconAtlas;
class Camera;
class Renderer;

// 管理作物被红石活塞破坏后的掉落物
// 暂时没有物品系统，所以掉落物只是视觉表现 + 统计
struct PendingDrop
{
    Vec3 m_position;
    uint8_t m_itemType = 0;
    int m_count = 1;
    float m_lifetime = 300.0f;      // 5分钟后消失
    float m_bobTimer = 0.0f;        // 上下浮动动画
    float m_rotationAngle = 0.0f;   // 旋转动画
    
    PendingDrop() = default;
    PendingDrop(const Vec3& pos, uint8_t type, int count)
        : m_position(pos), m_itemType(type), m_count(count) {}
};

// 掉落物统计
struct DropStatistics
{
    int m_totalDrops = 0;           // 总掉落物数量
    int m_totalItems = 0;           // 总物品数量（考虑堆叠）
    int m_wheatCount = 0;
    int m_carrotCount = 0;
    int m_potatoCount = 0;
    int m_beetrootCount = 0;
    int m_melonCount = 0;
    int m_pumpkinCount = 0;
    int m_otherCount = 0;
};

class CropDropManager
{
public:
    CropDropManager() = default;
    ~CropDropManager() = default;
    
    // 每帧更新
    void Update(float deltaSeconds);
    
    // 渲染掉落物
    void Render(Renderer* renderer, Camera const& camera, IconAtlas* uiAtlas) const;
    
    AABB2 GetDropUVs(uint8_t cropType, IconAtlas* atlas) const;
    
    void SpawnDrop(const Vec3& position, uint8_t itemType, int count = 1);
    void SpawnDropAtBlock(const IntVec3& blockPos, std::string itemName, int count = 1);
    void SpawnDropAtBlock(const IntVec3& blockPos, ItemType itemType, int count = 1);
    
    // 根据作物定义生成掉落物
    void SpawnCropDrops(const IntVec3& blockPos, ItemType cropType);
    
    // 收集掉落物（给玩家或漏斗用）
    std::vector<PendingDrop> CollectDropsNear(const Vec3& position, float radius);
    
    // 收集指定区域内的所有掉落物
    std::vector<PendingDrop> CollectDropsInArea(const IntVec3& minCorner, const IntVec3& maxCorner);
    
    // 清除所有掉落物
    void ClearAllDrops();
    
    // 获取统计信息
    DropStatistics GetStatistics() const;
    int GetTotalDropCount() const { return (int)m_drops.size(); }
    int GetTotalItemCount() const;
    
    // 调试
    void SetDebugRender(bool enable) { m_debugRender = enable; }
    
private:
    std::vector<PendingDrop> m_drops;
    bool m_debugRender = false;
    
    // 渲染单个掉落物
    void RenderDrop(Renderer* renderer, const PendingDrop& drop) const;
    
    // 根据物品类型获取颜色
    Rgba8 GetDropColor(uint8_t itemType) const;
    
    // 更新掉落物统计分类
    void CategorizeItem(uint8_t itemType, DropStatistics& stats) const;
};