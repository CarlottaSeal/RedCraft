#pragma once
#include <cstdint>

#include "Game/BlockIterator.h"

enum class WireConnection : uint8_t
{
    NONE = 0,       // 无连接
    SIDE = 1,       // 侧边连接（同高度）
    UP = 2,         // 向上连接（爬升）
    DOWN = 3        // 向下连接（下降）
};

struct WireConnections
{
    WireConnection m_north = WireConnection::NONE;
    WireConnection m_south = WireConnection::NONE;
    WireConnection m_east = WireConnection::NONE;
    WireConnection m_west = WireConnection::NONE;
    
    int GetConnectionCount() const
    {
        int count = 0;
        if (m_north != WireConnection::NONE) count++;
        if (m_south != WireConnection::NONE) count++;
        if (m_east != WireConnection::NONE) count++;
        if (m_west != WireConnection::NONE) count++;
        return count;
    }
    // 转换为纹理索引（0-15）
    uint8_t GetTextureIndex() const
    {
        // 编码：东西南北各1位
        uint8_t index = 0;
        if (m_north != WireConnection::NONE) index |= 0x01;
        if (m_south != WireConnection::NONE) index |= 0x02;
        if (m_east != WireConnection::NONE) index |= 0x04;
        if (m_west != WireConnection::NONE) index |= 0x08;
        return index;
    }
};

// 红石tick间隔（MC是0.1秒 = 2 game ticks）
constexpr float REDSTONE_TICK_INTERVAL = 0.1f;
constexpr uint8_t REDSTONE_MAX_POWER = 15;
constexpr int MAX_PISTON_PUSH = 12;
static constexpr uint8_t STONE_BUTTON_TICKS = 10;  // 1秒
static constexpr uint8_t WOOD_BUTTON_TICKS = 15;   // 1.5秒
constexpr uint8_t OBSERVER_PULSE_TICKS = 2;

static constexpr int TORCH_BURNOUT_THRESHOLD = 8;  // 8次切换触发保护
static constexpr float TORCH_BURNOUT_WINDOW = 1.0f; // 1秒内

struct RedstoneUpdate
{
    BlockIterator m_block;
    uint32_t m_scheduledTick = 0;  // 0 = 立即更新
};

struct TorchBurnoutEntry
{
    int64_t m_posKey = 0;
    float m_timestamp = 0.f;
    int m_toggleCount = 0;
};

struct ObserverUpdate
{
    BlockIterator m_block;
    uint32_t m_scheduledTick = 0;
    bool m_shouldPulse = false;  // 是否应该发出脉冲
};