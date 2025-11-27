// Game/Redstone/RedstoneSimulator.cpp
#include "RedstoneSimulator.h"
#include "Game/World.h"
#include "Game/Chunk.h"
#include "Game/Block.h"
#include "Game/BlockDefinition.h"
#include "Game/ChunkUtils.h"

int WireConnections::GetConnectionCount() const
{
    int count = 0;
    if (m_north != WireConnection::NONE) count++;
    if (m_south != WireConnection::NONE) count++;
    if (m_east != WireConnection::NONE) count++;
    if (m_west != WireConnection::NONE) count++;
    return count;
}

uint8_t WireConnections::GetTextureIndex() const
{
    // 编码：东西南北各1位
    uint8_t index = 0;
    if (m_north != WireConnection::NONE) index |= 0x01;
    if (m_south != WireConnection::NONE) index |= 0x02;
    if (m_east != WireConnection::NONE) index |= 0x04;
    if (m_west != WireConnection::NONE) index |= 0x08;
    return index;
}

RedstoneSimulator::RedstoneSimulator(World* world)
    : m_world(world)
{
}

void RedstoneSimulator::Update(float deltaSeconds)
{
    m_tickTimer += deltaSeconds;
    
    while (m_tickTimer >= REDSTONE_TICK_INTERVAL)
    {
        m_tickTimer -= REDSTONE_TICK_INTERVAL;
        m_currentTick++;
        
        // 处理当前tick到期的更新
        int updatesThisTick = 0;
        const int MAX_UPDATES = 1000;
        
        while (!m_updateQueue.empty() && updatesThisTick < MAX_UPDATES)
        {
            RedstoneUpdate& update = m_updateQueue.front();
            
            // 如果是延迟更新且还没到时间，跳过
            if (update.m_scheduledTick > m_currentTick)
                break;
            
            BlockIterator block = update.m_block;
            m_updateQueue.pop_front();
            
            if (block.IsValid())
            {
                int64_t key = PosToKey(block.GetGlobalCoords());
                m_queuedPositions.erase(key);
                
                block.GetBlock()->SetRedstoneDirty(false);
                ProcessBlockUpdate(block);
            }
            
            updatesThisTick++;
        }
    }
}

void RedstoneSimulator::ScheduleUpdate(const BlockIterator& block, uint8_t delayTicks)
{
    if (!block.IsValid())
        return;
    
    int64_t key = PosToKey(block.GetGlobalCoords());
    
    // 避免重复添加
    if (m_queuedPositions.find(key) != m_queuedPositions.end())
        return;
    
    m_queuedPositions.insert(key);
    block.GetBlock()->SetRedstoneDirty(true);
    
    RedstoneUpdate update;
    update.m_block = block;
    update.m_scheduledTick = m_currentTick + delayTicks;
    
    // 按scheduledTick排序插入
    if (delayTicks == 0 || m_updateQueue.empty())
    {
        m_updateQueue.push_back(update);
    }
    else
    {
        auto it = m_updateQueue.begin();
        while (it != m_updateQueue.end() && it->m_scheduledTick <= update.m_scheduledTick)
            ++it;
        m_updateQueue.insert(it, update);
    }
}

void RedstoneSimulator::ScheduleNeighborUpdates(const BlockIterator& block)
{
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (neighbor.IsValid())
        {
            ScheduleUpdate(neighbor, 0);
        }
    }
}

uint8_t RedstoneSimulator::GetInputPower(const BlockIterator& block) const
{
    if (!block.IsValid())
        return 0;
    
    uint8_t maxPower = 0;
    
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (!neighbor.IsValid())
            continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb)
            continue;
        
        uint8_t neighborType = nb->m_typeIndex;
        
        // 红石块恒定输出15
        if (neighborType == BLOCK_TYPE_REDSTONE_BLOCK)
        {
            maxPower = 15;
            continue;
        }
        
        // 红石火把输出15（不向附着方块输出）
        if (neighborType == BLOCK_TYPE_REDSTONE_TORCH)
        {
            // 火把在我们上方时输出
            if ((Direction)dir == DIRECTION_UP)
                maxPower = 15;
            continue;
        }
        
        // 开启的中继器向前方输出15
        if (neighborType == BLOCK_TYPE_REPEATER_ON)
        {
            Direction facing = (Direction)nb->GetBlockFacing();
            if ((Direction)dir == GetOppositeDir(facing))
                maxPower = 15;
            continue;
        }
        
        // 开启的拉杆输出15
        if (neighborType == BLOCK_TYPE_LEVER && nb->GetSpecialState())
        {
            maxPower = 15;
            continue;
        }
        
        // 红石线（衰减传播）
        if (neighborType == BLOCK_TYPE_REDSTONE_WIRE)
        {
            uint8_t wirePower = nb->GetRedstonePower();
            if (wirePower > 0)
            {
                uint8_t propagated = wirePower - 1;
                if (propagated > maxPower)
                    maxPower = propagated;
            }
        }
    }
    
    return maxPower;
}

bool RedstoneSimulator::IsPowered(const BlockIterator& block) const
{
    return GetInputPower(block) > 0;
}

bool RedstoneSimulator::IsPistonPowered(const BlockIterator& block) const
{
    if (!block.IsValid())
        return false;
    
    Block* piston = block.GetBlock();
    Direction facing = GetPistonFacing(piston);
    
    // 检查所有方向（除了面向方向，因为那里是活塞头/被推方块）
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        // 可选：活塞不从前方接收信号（MC行为）
        // if ((Direction)dir == facing)
        //     continue;
        
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (!neighbor.IsValid())
            continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb)
            continue;
        
        uint8_t neighborType = nb->m_typeIndex;
        
        // 直接电源
        if (neighborType == BLOCK_TYPE_REDSTONE_BLOCK)
            return true;
        
        if (neighborType == BLOCK_TYPE_REDSTONE_TORCH)
        {
            // 火把不向附着方块充能
            if ((Direction)dir != DIRECTION_UP)
                return true;
        }
        
        if ((neighborType == BLOCK_TYPE_LEVER || 
             neighborType == BLOCK_TYPE_BUTTON_STONE ||
             neighborType == BLOCK_TYPE_BUTTON_WOOD) && 
            nb->GetSpecialState())
        {
            return true;
        }
        
        if (neighborType == BLOCK_TYPE_REPEATER_ON)
        {
            Direction repFacing = (Direction)nb->GetBlockFacing();
            if (GetOppositeDir(repFacing) == (Direction)dir)
                return true;
        }
        
        // 红石线（弱充能）
        if (neighborType == BLOCK_TYPE_REDSTONE_WIRE)
        {
            if (nb->GetRedstonePower() > 0)
                return true;
        }
        
        // 被强充能的方块
        if (IsStronglyPowered(neighbor))
            return true;
    }
    return false;
}

void RedstoneSimulator::ProcessBlockUpdate(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    
    Block* b = block.GetBlock();
    if (!b)
        return;
    
    switch (b->m_typeIndex)
    {
    case BLOCK_TYPE_REDSTONE_WIRE:
        UpdateRedstoneWire(block);
        break;
        
    case BLOCK_TYPE_REDSTONE_TORCH:
    case BLOCK_TYPE_REDSTONE_TORCH_OFF:
        UpdateRedstoneTorch(block);
        break;
        
    case BLOCK_TYPE_REPEATER:
    case BLOCK_TYPE_REPEATER_ON:
        UpdateRepeater(block);
        break;
        
    case BLOCK_TYPE_REDSTONE_LAMP:
    case BLOCK_TYPE_REDSTONE_LAMP_ON:
        UpdateLamp(block);
        break;
        
    case BLOCK_TYPE_PISTON:
    case BLOCK_TYPE_STICKY_PISTON:
        UpdatePiston(block);
        break;
        
    case BLOCK_TYPE_BUTTON_STONE:
    case BLOCK_TYPE_BUTTON_WOOD:
        UpdateButton(block);
        break;
    }
}

uint8_t RedstoneSimulator::GetDirectPowerInput(const BlockIterator& block) const
{
    uint8_t maxPower = 0;
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (!neighbor.IsValid())
            continue;
        Block* nb = neighbor.GetBlock();
        if (!nb)
            continue;
        
        uint8_t neighborType = nb->m_typeIndex;
        // 红石块：恒定15
        if (neighborType == BLOCK_TYPE_REDSTONE_BLOCK)
        {
            return 15;  // 最大值，直接返回
        }
        // 红石火把：向四周和上方输出15
        if (neighborType == BLOCK_TYPE_REDSTONE_TORCH)
        {
            // 火把不向下方输出（附着方向）
            if ((Direction)dir != DIRECTION_DOWN)
            {
                return 15;
            }
        }
        // 开启的中继器：向前方输出15
        if (neighborType == BLOCK_TYPE_REPEATER_ON)
        {
            Direction repeaterFacing = (Direction)nb->GetBlockFacing();
            // 如果中继器朝向我们，我们就在它的输出方向
            if ((Direction)dir == GetOppositeDir(repeaterFacing))
            {
                return 15;
            }
        }
        // 开启的拉杆：向所有方向输出15
        if (neighborType == BLOCK_TYPE_LEVER && nb->GetSpecialState())
        {
            return 15;
        }
        // 按下的按钮：向所有方向输出15
        if ((neighborType == BLOCK_TYPE_BUTTON_STONE || 
             neighborType == BLOCK_TYPE_BUTTON_WOOD) && 
            nb->GetSpecialState())
        {
            return 15;
        }
        // 开启的比较器输出
        if (neighborType == BLOCK_TYPE_COMPARATOR_ON)
        {
            Direction compFacing = (Direction)nb->GetBlockFacing();
            if ((Direction)dir == GetOppositeDir(compFacing))
            {
                uint8_t compPower = nb->GetRedstonePower();
                if (compPower > maxPower)
                    maxPower = compPower;
            }
        }
        // 被充能的实心方块（强充能）
        if (IsStronglyPowered(neighbor))
        {
            return 15;
        }
    }
    return maxPower;
}

uint8_t RedstoneSimulator::GetAdjacentWirePower(const BlockIterator& block) const
{
    uint8_t maxPower = 0;
    Direction horizontalDirs[] = {
        DIRECTION_NORTH, DIRECTION_SOUTH, 
        DIRECTION_EAST, DIRECTION_WEST
    };
    
    for (Direction dir : horizontalDirs)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary(dir);
        if (!neighbor.IsValid())
            continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb || nb->m_typeIndex != BLOCK_TYPE_REDSTONE_WIRE)
            continue;
        
        uint8_t neighborPower = nb->GetRedstonePower();
        if (neighborPower > 0)
        {
            uint8_t propagated = neighborPower - 1;  // 衰减1
            if (propagated > maxPower)
                maxPower = propagated;
        }
    }
    
    return maxPower;
}

uint8_t RedstoneSimulator::GetClimbingWirePower(const BlockIterator& block) const
{
    uint8_t maxPower = 0;
    Direction horizontalDirs[] = {
        DIRECTION_NORTH, DIRECTION_SOUTH, 
        DIRECTION_EAST, DIRECTION_WEST
    };
    for (Direction hDir : horizontalDirs)
    {
        // ===== 检查上方侧边红石线（下爬到我们） =====
        BlockIterator side = block.GetNeighborCrossBoundary(hDir);
        if (side.IsValid())
        {
            Block* sideBlock = side.GetBlock();
            
            // 侧边方块必须是透明的，红石才能爬升
            if (!sideBlock || !sideBlock->IsOpaque())
            {
                BlockIterator aboveSide = side.GetNeighborCrossBoundary(DIRECTION_UP);
                if (aboveSide.IsValid())
                {
                    Block* aboveSideBlock = aboveSide.GetBlock();
                    if (aboveSideBlock && 
                        aboveSideBlock->m_typeIndex == BLOCK_TYPE_REDSTONE_WIRE)
                    {
                        // 还要检查我们上方是否有方块阻挡
                        BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
                        if (above.IsValid())
                        {
                            Block* aboveBlock = above.GetBlock();
                            if (!aboveBlock || !aboveBlock->IsOpaque())
                            {
                                uint8_t abovePower = aboveSideBlock->GetRedstonePower();
                                if (abovePower > 0)
                                {
                                    uint8_t propagated = abovePower - 1;
                                    if (propagated > maxPower)
                                        maxPower = propagated;
                                }
                            }
                        }
                    }
                }
            }
        }
        // ===== 检查下方侧边红石线（上爬到我们） =====
        BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
        if (below.IsValid())
        {
            BlockIterator belowSide = below.GetNeighborCrossBoundary(hDir);
            if (belowSide.IsValid())
            {
                Block* belowSideBlock = belowSide.GetBlock();
                if (belowSideBlock && 
                    belowSideBlock->m_typeIndex == BLOCK_TYPE_REDSTONE_WIRE)
                {
                    // 检查侧边方块是否阻挡
                    if (side.IsValid())
                    {
                        Block* sideBlock = side.GetBlock();
                        if (!sideBlock || !sideBlock->IsOpaque())
                        {
                            uint8_t belowPower = belowSideBlock->GetRedstonePower();
                            if (belowPower > 0)
                            {
                                uint8_t propagated = belowPower - 1;
                                if (propagated > maxPower)
                                    maxPower = propagated;
                            }
                        }
                    }
                }
            }
        }
    }
    return maxPower;
}

uint8_t RedstoneSimulator::GetCornerFacing(bool north, bool south, bool east, bool west) const
{
    // 返回角落的朝向（0-3）
    // 朝向定义：facing表示角落的"外角"指向
    
    if (north && east)  return 0;  // NE角，外角指向东北
    if (east && south)  return 1;  // SE角，外角指向东南
    if (south && west)  return 2;  // SW角，外角指向西南
    if (west && north)  return 3;  // NW角，外角指向西北
    
    return 0;  // 默认
}

void RedstoneSimulator::ScheduleClimbingWireUpdates(const BlockIterator& block)
{
    Direction horizontalDirs[] = {
        DIRECTION_NORTH, DIRECTION_SOUTH, 
        DIRECTION_EAST, DIRECTION_WEST
    };
    
    for (Direction hDir : horizontalDirs)
    {
        // 上方侧边
        BlockIterator side = block.GetNeighborCrossBoundary(hDir);
        if (side.IsValid())
        {
            BlockIterator aboveSide = side.GetNeighborCrossBoundary(DIRECTION_UP);
            if (aboveSide.IsValid())
            {
                Block* b = aboveSide.GetBlock();
                if (b && b->m_typeIndex == BLOCK_TYPE_REDSTONE_WIRE)
                {
                    ScheduleUpdate(aboveSide, 0);
                }
            }
        }
        
        // 下方侧边
        BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
        if (below.IsValid())
        {
            BlockIterator belowSide = below.GetNeighborCrossBoundary(hDir);
            if (belowSide.IsValid())
            {
                Block* b = belowSide.GetBlock();
                if (b && b->m_typeIndex == BLOCK_TYPE_REDSTONE_WIRE)
                {
                    ScheduleUpdate(belowSide, 0);
                }
            }
        }
    }
}

void RedstoneSimulator::UpdateRedstoneWire(const BlockIterator& block)
{
    Block* wire = block.GetBlock();
    if (!wire || !IsRedstoneWire(wire->m_typeIndex))
        return;
    uint8_t oldPower = wire->GetRedstonePower();
    uint8_t newPower = 0;
    //检查直接相邻的电源方块
    newPower = GetDirectPowerInput(block);
    
    //检查相邻红石线（信号衰减）
    if (newPower < 15)
    {
        uint8_t wirePower = GetAdjacentWirePower(block);
        if (wirePower > newPower)
            newPower = wirePower;
    }
    //检查上下方红石线（爬升/下降）
    if (newPower < 15)
    {
        uint8_t climbPower = GetClimbingWirePower(block);
        if (climbPower > newPower)
            newPower = climbPower;
    }
    //信号改变时更新
    if (oldPower != newPower)
    {
        wire->SetRedstonePower(newPower);
        
        // 标记区块需要重建网格（颜色变化）
        MarkChunkDirty(block);
        
        // 触发邻居更新（传播信号变化）
        ScheduleNeighborUpdates(block);
        
        // 额外：触发上下方红石线更新（爬升连接）
        ScheduleClimbingWireUpdates(block);
    }
    newPower = GetDirectPowerInput(block);
    
    if (newPower < 15)
    {
        uint8_t wirePower = GetAdjacentWirePower(block);
        if (wirePower > newPower)
            newPower = wirePower;
    }
    
    if (newPower < 15)
    {
        uint8_t climbPower = GetClimbingWirePower(block);
        if (climbPower > newPower)
            newPower = climbPower;
    }
    
    //更新连接外观 
    UpdateWireConnections(block);
    
    // 信号改变时更新
    if (oldPower != newPower)
    {
        wire->SetRedstonePower(newPower);
        MarkChunkDirty(block);
        ScheduleNeighborUpdates(block);
        ScheduleClimbingWireUpdates(block);
    }
    // Block* wire = block.GetBlock();
    // if (!wire)
    //     return;
    //
    // uint8_t newPower = 0;
    //
    // // 检查所有方向的输入
    // for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    // {
    //     BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
    //     if (!neighbor.IsValid())
    //         continue;
    //     
    //     Block* nb = neighbor.GetBlock();
    //     if (!nb)
    //         continue;
    //     
    //     uint8_t neighborType = nb->m_typeIndex;
    //     
    //     // 电源方块直接提供15
    //     if (neighborType == BLOCK_TYPE_REDSTONE_BLOCK ||
    //         neighborType == BLOCK_TYPE_REDSTONE_TORCH ||
    //         neighborType == BLOCK_TYPE_REPEATER_ON ||
    //         (neighborType == BLOCK_TYPE_LEVER && nb->GetSpecialState()))
    //     {
    //         newPower = 15;
    //     }
    //     // 其他红石线（衰减1）
    //     else if (neighborType == BLOCK_TYPE_REDSTONE_WIRE)
    //     {
    //         uint8_t neighborPower = nb->GetRedstonePower();
    //         if (neighborPower > 0 && neighborPower - 1 > newPower)
    //         {
    //             newPower = neighborPower - 1;
    //         }
    //     }
    // }
    //
    // // 检查上下方红石线（爬升）
    // Direction horizontalDirs[] = {DIRECTION_NORTH, DIRECTION_SOUTH, DIRECTION_EAST, DIRECTION_WEST};
    // for (Direction hDir : horizontalDirs)
    // {
    //     // 上方侧边的红石线
    //     BlockIterator side = block.GetNeighborCrossBoundary(hDir);
    //     if (side.IsValid())
    //     {
    //         BlockIterator aboveSide = side.GetNeighborCrossBoundary(DIRECTION_UP);
    //         if (aboveSide.IsValid())
    //         {
    //             Block* aboveSideBlock = aboveSide.GetBlock();
    //             if (aboveSideBlock && aboveSideBlock->m_typeIndex == BLOCK_TYPE_REDSTONE_WIRE)
    //             {
    //                 // 检查侧边方块是否阻挡
    //                 Block* sideBlock = side.GetBlock();
    //                 if (sideBlock && !sideBlock->IsOpaque())
    //                 {
    //                     uint8_t abovePower = aboveSideBlock->GetRedstonePower();
    //                     if (abovePower > 0 && abovePower - 1 > newPower)
    //                         newPower = abovePower - 1;
    //                 }
    //             }
    //         }
    //     }
    //     
    //     // 下方侧边的红石线
    //     BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
    //     if (below.IsValid())
    //     {
    //         BlockIterator belowSide = below.GetNeighborCrossBoundary(hDir);
    //         if (belowSide.IsValid())
    //         {
    //             Block* belowSideBlock = belowSide.GetBlock();
    //             if (belowSideBlock && belowSideBlock->m_typeIndex == BLOCK_TYPE_REDSTONE_WIRE)
    //             {
    //                 uint8_t belowPower = belowSideBlock->GetRedstonePower();
    //                 if (belowPower > 0 && belowPower - 1 > newPower)
    //                     newPower = belowPower - 1;
    //             }
    //         }
    //     }
    // }
    //
    // // 如果信号改变
    // if (wire->GetRedstonePower() != newPower)
    // {
    //     wire->SetRedstonePower(newPower);
    //     
    //     // 标记区块需要重建网格（颜色变化）
    //     Chunk* chunk = block.GetChunk();
    //     if (chunk)
    //     {
    //         chunk->MarkSelfDirty();
    //         m_world->m_hasDirtyChunk = true;
    //     }
    //     
    //     // 触发邻居更新
    //     ScheduleNeighborUpdates(block);
    // }
}

void RedstoneSimulator::UpdateWireAppearance(const IntVec3& pos)
{
    Block* block = m_world->GetBlockAt(pos);
    if (!IsRedstoneWire(block->GetType()))
        return;
    
    // 计算连接状态
    bool north = CanConnectTo(pos + IntVec3(0, 0, 1));
    bool south = CanConnectTo(pos + IntVec3(0, 0, -1));
    bool east  = CanConnectTo(pos + IntVec3(1, 0, 0));
    bool west  = CanConnectTo(pos + IntVec3(-1, 0, 0));
    
    int connections = (north ? 1 : 0) + (south ? 1 : 0) + (east ? 1 : 0) + (west ? 1 : 0);
    
    // 根据连接数和方向选择方块类型
    uint8_t newType;
    uint8_t facing = 0;
    
    if (connections == 0)
    {
        newType = BLOCK_TYPE_REDSTONE_WIRE_DOT;
    }
    else if (connections == 2 && north && south) {
        newType = BLOCK_TYPE_REDSTONE_WIRE_LINE;
        facing = 0; // 南北向
    }
    else if (connections == 2 && east && west) {
        newType = BLOCK_TYPE_REDSTONE_WIRE_LINE;
        facing = 1; // 东西向
    }
    else if (connections == 4) {
        newType = BLOCK_TYPE_REDSTONE_WIRE_CROSS;
    }
    else {
        // 角落或T型，用Corner类型 + 旋转
        newType = BLOCK_TYPE_REDSTONE_WIRE_CORNER;
        facing = CalculateCornerFacing(north, south, east, west);
    }
    
    block->SetType(newType);
    block->SetBlockFacing(facing);
}

void RedstoneSimulator::UpdateRedstoneTorch(const BlockIterator& block)
{
    if (CheckTorchBurnout(block))
    {
        Block* torch = block.GetBlock();
        if (torch && torch->m_typeIndex == BLOCK_TYPE_REDSTONE_TORCH)
        {
            // 强制熄灭
            torch->m_typeIndex = BLOCK_TYPE_REDSTONE_TORCH_OFF;
            MarkChunkDirty(block);
            m_world->MarkLightingDirty(block);
            // 不触发邻居更新，防止继续闪烁
            return;
        }
    }
    
    Block* torch = block.GetBlock();
    if (!torch)
        return;
    
    uint8_t torchType = torch->m_typeIndex;
    if (torchType != BLOCK_TYPE_REDSTONE_TORCH && 
        torchType != BLOCK_TYPE_REDSTONE_TORCH_OFF)
        return;
    
    // ===== 获取附着方块 =====
    // 火把的朝向存储在 BlockFacing 中
    // 0 = 附着在下方（立在方块上）
    // 1-4 = 附着在侧边（北、南、东、西）
    Direction attachDir = GetTorchAttachmentDirection(torch);
    BlockIterator attached = block.GetNeighborCrossBoundary(attachDir);
    
    // ===== 检查附着方块是否被充能 =====
    bool attachedPowered = false;
    if (attached.IsValid())
    {
        // 检查附着方块是否被红石信号充能
        // 注意：不是检查附着方块本身是否是电源
        attachedPowered = IsBlockReceivingPower(attached);
    }
    
    bool isCurrentlyOn = (torchType == BLOCK_TYPE_REDSTONE_TORCH);
    
    // ===== 状态切换 =====
    if (attachedPowered && isCurrentlyOn)
    {
        // 附着方块被充能 → 火把熄灭
        torch->m_typeIndex = BLOCK_TYPE_REDSTONE_TORCH_OFF;
        
        // 更新光照（从7降到0）
        MarkChunkDirty(block);
        m_world->MarkLightingDirty(block);
        
        // 触发邻居更新
        ScheduleNeighborUpdates(block);
        
        // 火把状态改变会影响上方方块
        BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
        if (above.IsValid())
        {
            ScheduleUpdate(above, 0);
        }
    }
    else if (!attachedPowered && !isCurrentlyOn)
    {
        // 附着方块未充能 → 火把点亮
        torch->m_typeIndex = BLOCK_TYPE_REDSTONE_TORCH;
        
        // 更新光照
        MarkChunkDirty(block);
        m_world->MarkLightingDirty(block);
        
        // 触发邻居更新
        ScheduleNeighborUpdates(block);
        
        BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
        if (above.IsValid())
        {
            ScheduleUpdate(above, 0);
        }
    }
    // Block* torch = block.GetBlock();
    // if (!torch)
    //     return;
    //
    // // 检查附着方块是否被充能
    // BlockIterator attached = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
    // bool shouldBeOff = false;
    //
    // if (attached.IsValid())
    // {
    //     shouldBeOff = IsPowered(attached);
    // }
    //
    // bool isCurrentlyOn = (torch->m_typeIndex == BLOCK_TYPE_REDSTONE_TORCH);
    //
    // if (shouldBeOff && isCurrentlyOn)
    // {
    //     torch->m_typeIndex = BLOCK_TYPE_REDSTONE_TORCH_OFF;
    //     MarkChunkDirty(block);
    //     ScheduleNeighborUpdates(block);
    // }
    // else if (!shouldBeOff && !isCurrentlyOn)
    // {
    //     torch->m_typeIndex = BLOCK_TYPE_REDSTONE_TORCH;
    //     MarkChunkDirty(block);
    //     ScheduleNeighborUpdates(block);
    // }
}

void RedstoneSimulator::UpdateRepeater(const BlockIterator& block)
{
    Block* repeater = block.GetBlock();
    if (!repeater)
        return;
    
    uint8_t repeaterType = repeater->m_typeIndex;
    if (repeaterType != BLOCK_TYPE_REPEATER && 
        repeaterType != BLOCK_TYPE_REPEATER_ON)
        return;
    
    // ===== 检查锁定状态 =====
    bool wasLocked = IsRepeaterLocked(repeater);
    bool nowLocked = CheckRepeaterLock(block);
    
    if (nowLocked != wasLocked)
    {
        SetRepeaterLocked(repeater, nowLocked);
        MarkChunkDirty(block);
    }
    
    // 如果被锁定，保持当前状态
    if (nowLocked)
    {
        return;
    }
    
    // ===== 检查输入信号 =====
    Direction facing = (Direction)repeater->GetBlockFacing();
    Direction inputDir = GetOppositeDir(facing);
    
    bool hasInput = CheckRepeaterInput(block, inputDir);
    bool isCurrentlyOn = (repeaterType == BLOCK_TYPE_REPEATER_ON);
    
    // ===== 状态转换 =====
    if (hasInput && !isCurrentlyOn)
    {
        // 关→开：需要延迟
        uint8_t delay = GetRepeaterDelay(repeater);
        
        // 立即切换类型（视觉），但延迟输出信号
        repeater->m_typeIndex = BLOCK_TYPE_REPEATER_ON;
        MarkChunkDirty(block);
        
        // 计划延迟后的邻居更新
        ScheduleDelayedOutput(block, delay);
    }
    else if (!hasInput && isCurrentlyOn)
    {
        // 开→关：同样有延迟
        uint8_t delay = GetRepeaterDelay(repeater);
        
        repeater->m_typeIndex = BLOCK_TYPE_REPEATER;
        MarkChunkDirty(block);
        
        ScheduleDelayedOutput(block, delay);
    }
}

void RedstoneSimulator::UpdateLamp(const BlockIterator& block)
{
    Block* lamp = block.GetBlock();
    if (!lamp)
        return;
    uint8_t lampType = lamp->m_typeIndex;
    if (lampType != BLOCK_TYPE_REDSTONE_LAMP && 
        lampType != BLOCK_TYPE_REDSTONE_LAMP_ON)
        return;
    //检查是否有红石信号输入
    bool powered = IsPowered(block);
    bool isCurrentlyOn = (lampType == BLOCK_TYPE_REDSTONE_LAMP_ON);
    
    if (powered && !isCurrentlyOn)
    {
        lamp->m_typeIndex = BLOCK_TYPE_REDSTONE_LAMP_ON;
        
        // 标记区块需要重建网格（纹理变化）
        MarkChunkDirty(block);
        // 更新光照系统（灯发出15级光）
        UpdateLampLighting(block, true);
    }
    else if (!powered && isCurrentlyOn)
    {
        // TODO：Minecraft中红石灯有2tick的关闭延迟
        lamp->m_typeIndex = BLOCK_TYPE_REDSTONE_LAMP;
        
        MarkChunkDirty(block);
        UpdateLampLighting(block, false);
    }
    // Block* lamp = block.GetBlock();
    // if (!lamp)
    //     return;
    //
    // bool powered = IsPowered(block);
    // bool isOn = (lamp->m_typeIndex == BLOCK_TYPE_REDSTONE_LAMP_ON);
    //
    // if (powered && !isOn)
    // {
    //     lamp->m_typeIndex = BLOCK_TYPE_REDSTONE_LAMP_ON;
    //     MarkChunkDirty(block);
    //     // 更新光照
    //     m_world->MarkLightingDirty(block);
    // }
    // else if (!powered && isOn)
    // {
    //     lamp->m_typeIndex = BLOCK_TYPE_REDSTONE_LAMP;
    //     MarkChunkDirty(block);
    //     m_world->MarkLightingDirty(block);
    // }
}

void RedstoneSimulator::UpdateLampLighting(const BlockIterator& block, bool turnOn)
{
    if (!block.IsValid())
        return;
    Block* lamp = block.GetBlock();
    if (!lamp)
        return;
    // 设置室内光照
    if (turnOn)
    {
        lamp->SetIndoorLight(15);
    }
    else
    {
        lamp->SetIndoorLight(0);
    }
    // 标记周围方块需要光照更新
    lamp->SetLightDirty(true);
    
    // 将方块加入世界的脏光照列表
    if (m_world)
    {
        IntVec3 pos = block.GetGlobalCoords();
        m_world->AddDirtyLightBlock(pos);
        
        // 也标记邻居
        for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
        {
            BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
            if (neighbor.IsValid())
            {
                Block* nb = neighbor.GetBlock();
                if (nb)
                {
                    nb->SetLightDirty(true);
                    m_world->AddDirtyLightBlock(neighbor.GetGlobalCoords());
                }
            }
        }
    }
}

void RedstoneSimulator::UpdatePiston(const BlockIterator& block)
{
    Block* piston = block.GetBlock();
    if (!piston)
        return;
    uint8_t pistonType = piston->m_typeIndex;
    if (pistonType != BLOCK_TYPE_PISTON && 
        pistonType != BLOCK_TYPE_STICKY_PISTON)
        return;
    
    bool powered = IsPistonPowered(block);
    bool extended = piston->GetSpecialState();
    Direction facing = (Direction)piston->GetBlockFacing();
    
    if (powered && !extended)
    {
        TryExtendPiston(block);
    }
    else if (!powered && extended)
    {
        TryRetractPiston(block);
    }
}

bool RedstoneSimulator::CanPistonPush(const BlockIterator& piston, Direction pushDir)
{
    BlockIterator current = piston.GetNeighborCrossBoundary(pushDir);
    
    for (int i = 0; i < MAX_PISTON_PUSH; i++)
    {
        if (!current.IsValid())
            return false;
        
        Block* b = current.GetBlock();
        if (!b)
            return false;
        
        // 空气 = 可以推
        if (b->m_typeIndex == BLOCK_TYPE_AIR)
            return true;
        
        // 黑曜石等不能推
        if (b->m_typeIndex == BLOCK_TYPE_OBSIDIAN)
            return false;
        
        current = current.GetNeighborCrossBoundary(pushDir);
    }
    
    return false;  // 超过最大推动数量
}

void RedstoneSimulator::TryExtendPiston(const BlockIterator& block)
{
    Block* piston = block.GetBlock();
    if (!piston)
        return;
    
    Direction facing = GetPistonFacing(piston);
    bool isSticky = (piston->m_typeIndex == BLOCK_TYPE_STICKY_PISTON);
    
    // 收集要推动的方块
    std::vector<IntVec3> blocksToPush;
    std::vector<IntVec3> blocksToDestroy;
    
    if (!CalculatePushList(block, facing, blocksToPush, blocksToDestroy))
    {
        // 无法推动
        return;
    }
    
    // ===== 执行推动 =====
    
    // 1. 销毁需要销毁的方块（如火把、红石线等）
    for (const IntVec3& pos : blocksToDestroy)
    {
        BlockIterator iter = m_world->GetBlockIterator(pos);
        if (iter.IsValid())
        {
            Block* b = iter.GetBlock();
            if (b)
            {
                // 可选：掉落物品
                // DropBlockItem(iter);
                
                b->SetType(BLOCK_TYPE_AIR);
                MarkChunkDirty(iter);
            }
        }
    }
    
    // 2. 从最远的方块开始移动（避免覆盖）
    // 按距离活塞的距离排序（远的先移动）
    std::sort(blocksToPush.begin(), blocksToPush.end(),
        [&](const IntVec3& a, const IntVec3& b) {
            IntVec3 pistonPos = block.GetGlobalCoords();
            return GetManhattanDistance(a, pistonPos) > 
                   GetManhattanDistance(b, pistonPos);
        });
    
    for (const IntVec3& srcPos : blocksToPush)
    {
        IntVec3 dstPos = srcPos + GetDirectionVector(facing);
        
        BlockIterator src = m_world->GetBlockIterator(srcPos);
        BlockIterator dst = m_world->GetBlockIterator(dstPos);
        
        if (src.IsValid() && dst.IsValid())
        {
            Block* srcBlock = src.GetBlock();
            Block* dstBlock = dst.GetBlock();
            
            // 复制方块数据
            *dstBlock = *srcBlock;
            
            // 清空源位置
            srcBlock->SetType(BLOCK_TYPE_AIR);
            srcBlock->m_redstoneData = 0;
            
            MarkChunkDirty(src);
            MarkChunkDirty(dst);
            
            // 触发红石更新
            ScheduleNeighborUpdates(dst);
        }
    }
    
    // 3. 放置活塞头
    BlockIterator headPos = block.GetNeighborCrossBoundary(facing);
    if (headPos.IsValid())
    {
        Block* head = headPos.GetBlock();
        head->SetType(BLOCK_TYPE_PISTON_HEAD);
        head->SetBlockFacing((uint8_t)facing);
        
        // 标记是否为粘性活塞头
        if (isSticky)
        {
            head->SetRedstonePower(head->GetRedstonePower() | 0x08);
        }
        
        MarkChunkDirty(headPos);
    }
    
    // 4. 标记活塞为已伸出
    SetPistonExtended(piston, true);
    MarkChunkDirty(block);
}

void RedstoneSimulator::TryRetractPiston(const BlockIterator& block)
{
    Block* piston = block.GetBlock();
    if (!piston)
        return;
    
    Direction facing = GetPistonFacing(piston);
    bool isSticky = (piston->m_typeIndex == BLOCK_TYPE_STICKY_PISTON);
    
    // 1. 移除活塞头
    BlockIterator headPos = block.GetNeighborCrossBoundary(facing);
    if (headPos.IsValid())
    {
        Block* head = headPos.GetBlock();
        if (head && head->m_typeIndex == BLOCK_TYPE_PISTON_HEAD)
        {
            head->SetType(BLOCK_TYPE_AIR);
            head->m_redstoneData = 0;
            MarkChunkDirty(headPos);
        }
    }
    
    // 2. 粘性活塞：尝试拉回前方方块
    if (isSticky)
    {
        BlockIterator pullPos = headPos.GetNeighborCrossBoundary(facing);
        if (pullPos.IsValid())
        {
            Block* pullBlock = pullPos.GetBlock();
            if (pullBlock && 
                pullBlock->m_typeIndex != BLOCK_TYPE_AIR &&
                CanBePulled(pullBlock->m_typeIndex))
            {
                // 将方块拉回到活塞头位置
                Block* dst = headPos.GetBlock();
                *dst = *pullBlock;
                
                pullBlock->SetType(BLOCK_TYPE_AIR);
                pullBlock->m_redstoneData = 0;
                
                MarkChunkDirty(headPos);
                MarkChunkDirty(pullPos);
                
                // 触发红石更新
                ScheduleNeighborUpdates(headPos);
                ScheduleNeighborUpdates(pullPos);
            }
        }
    }
    
    // 3. 标记活塞为收回状态
    SetPistonExtended(piston, false);
    MarkChunkDirty(block);
}

bool RedstoneSimulator::CalculatePushList(const BlockIterator& piston, Direction pushDir,
                                          std::vector<IntVec3>& blocksToPush, std::vector<IntVec3>& blocksToDestroy) const
{
    blocksToPush.clear();
    blocksToDestroy.clear();
    
    std::queue<IntVec3> toCheck;
    std::unordered_set<int64_t> checked;
    
    // 从活塞前方开始
    IntVec3 startPos = piston.GetGlobalCoords() + GetDirectionVector(pushDir);
    toCheck.push(startPos);
    
    while (!toCheck.empty())
    {
        IntVec3 pos = toCheck.front();
        toCheck.pop();
        
        int64_t key = PosToKey(pos);
        if (checked.find(key) != checked.end())
            continue;
        checked.insert(key);
        
        BlockIterator iter = m_world->GetBlockIterator(pos);
        if (!iter.IsValid())
        {
            // 超出世界范围
            return false;
        }
        
        Block* b = iter.GetBlock();
        if (!b)
            continue;
        
        uint8_t blockType = b->m_typeIndex;
        
        // 空气：可以推到这里
        if (blockType == BLOCK_TYPE_AIR)
            continue;
        
        // 不可推动的方块
        if (!CanBePushed(blockType))
        {
            return false;  // 整个推动失败
        }
        
        // 会被破坏的方块（火把、红石线等）
        if (IsDestroyedWhenPushed(blockType))
        {
            blocksToDestroy.push_back(pos);
            continue;
        }
        
        // 可推动的方块
        blocksToPush.push_back(pos);
        
        // 检查推动数量限制
        if (blocksToPush.size() > MAX_PISTON_PUSH)
        {
            return false;
        }
        
        // 继续检查这个方块前方
        IntVec3 nextPos = pos + GetDirectionVector(pushDir);
        toCheck.push(nextPos);
        
        // 检查粘性方块连接（史莱姆块、蜂蜜块）
        if (IsStickyBlock(blockType))
        {
            // 添加相邻的可推动方块
            for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
            {
                if ((Direction)dir == pushDir || 
                    (Direction)dir == GetOppositeDir(pushDir))
                    continue;
                
                IntVec3 adjacentPos = pos + GetDirectionVector((Direction)dir);
                toCheck.push(adjacentPos);
            }
        }
    }
    
    return true;
}

bool RedstoneSimulator::IsStronglyPowered(const BlockIterator& block) const
{
    if (!block.IsValid())
        return false;
    
    Block* b = block.GetBlock();
    if (!b || !b->IsSolid() || !b->IsOpaque())
        return false;
    
    // 检查是否有电源直接指向这个方块
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (!neighbor.IsValid())
            continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb)
            continue;
        
        uint8_t neighborType = nb->m_typeIndex;
        
        // 红石火把在下方（附着）会强充能上方方块
        if (neighborType == BLOCK_TYPE_REDSTONE_TORCH && 
            (Direction)dir == DIRECTION_DOWN)
        {
            return true;
        }
        
        // 开启的中继器向前方强充能
        if (neighborType == BLOCK_TYPE_REPEATER_ON)
        {
            Direction facing = (Direction)nb->GetBlockFacing();
            if ((Direction)dir == GetOppositeDir(facing))
                return true;
        }
    }
    
    return false;
}

bool RedstoneSimulator::IsBlockReceivingPower(const BlockIterator& block) const
{
    if (!block.IsValid())
        return false;
    Block* b = block.GetBlock();
    if (!b)
        return false;
    
    // 如果是红石组件，检查其自身状态
    if (IsRedstoneComponent(b->m_typeIndex))
    {
        return b->GetRedstonePower() > 0;
    }
    // 对于普通方块，检查是否有信号输入
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (!neighbor.IsValid())
            continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb)
            continue;
        
        uint8_t neighborType = nb->m_typeIndex;
        
        // 红石线可以弱充能方块
        if (neighborType == BLOCK_TYPE_REDSTONE_WIRE)
        {
            if (nb->GetRedstonePower() > 0)
                return true;
        }
        
        // 红石块可以充能
        if (neighborType == BLOCK_TYPE_REDSTONE_BLOCK)
            return true;
        
        // 开启的拉杆
        if (neighborType == BLOCK_TYPE_LEVER && nb->GetSpecialState())
            return true;
        
        // 按下的按钮
        if ((neighborType == BLOCK_TYPE_BUTTON_STONE || 
             neighborType == BLOCK_TYPE_BUTTON_WOOD) && 
            nb->GetSpecialState())
            return true;
        
        // 开启的中继器（只向输出方向充能）
        if (neighborType == BLOCK_TYPE_REPEATER_ON)
        {
            Direction facing = (Direction)nb->GetBlockFacing();
            if (GetOppositeDir(facing) == (Direction)dir)
                return true;
        }
    }
    return false;
}

bool RedstoneSimulator::CanBePushed(uint8_t blockType) const
{
    //TODO: 什么类型？
    if (IsSolid(blockType))
        return false;
    return true;
}

bool RedstoneSimulator::CanBePulled(uint8_t blockType) const
{
    // 大部分可推动的方块也可被拉动
    if (!CanBePushed(blockType))
        return false;
    // 会被破坏的方块不会被拉动
    if (IsDestroyedWhenPushed(blockType))
        return false;
    return true;
}

bool RedstoneSimulator::IsDestroyedWhenPushed(uint8_t blockType) const
{
    if (IsRedstoneWire(blockType))
        return true;
    switch (blockType)
    {
    case BLOCK_TYPE_REDSTONE_TORCH:
    case BLOCK_TYPE_REDSTONE_TORCH_OFF:
    case BLOCK_TYPE_LEVER:
    case BLOCK_TYPE_BUTTON_STONE:
    case BLOCK_TYPE_BUTTON_WOOD:
        return true;
    }
    
    return false;
}

bool RedstoneSimulator::IsStickyBlock(uint8_t blockType) const
{
    return false;
    // return blockType == BLOCK_TYPE_SLIME_BLOCK || 
    //        blockType == BLOCK_TYPE_HONEY_BLOCK;
}

IntVec3 RedstoneSimulator::GetDirectionVector(Direction dir) const
{
    switch (dir)
    {
    case DIRECTION_DOWN:  return IntVec3(0, 0, -1);
    case DIRECTION_UP:    return IntVec3(0, 0, 1);
    case DIRECTION_NORTH: return IntVec3(0, -1, 0);
    case DIRECTION_SOUTH: return IntVec3(0, 1, 0);
    case DIRECTION_EAST:  return IntVec3(1, 0, 0);
    case DIRECTION_WEST:  return IntVec3(-1, 0, 0);
    default: return IntVec3(0, 0, 0);
    }
}

Direction RedstoneSimulator::GetTorchAttachmentDirection(Block* torch) const
{
    uint8_t facing = torch->GetBlockFacing();
    switch (facing)
    {
    case 0: return DIRECTION_DOWN; 
    case 1: return DIRECTION_NORTH;
    case 2: return DIRECTION_SOUTH;
    case 3: return DIRECTION_EAST;
    case 4: return DIRECTION_WEST;
    default: return DIRECTION_DOWN;
    }
}

uint8_t RedstoneSimulator::GetTorchOutputPower(const BlockIterator& torch, Direction toDir) const
{
    Block* b = torch.GetBlock();
    if (!b || b->m_typeIndex != BLOCK_TYPE_REDSTONE_TORCH)
        return 0;
    
    // 火把不向附着方向输出
    Direction attachDir = GetTorchAttachmentDirection(b);
    if (toDir == attachDir)
        return 0;
    return 15;
}

bool RedstoneSimulator::CheckTorchBurnout(const BlockIterator& block)
{
    int64_t key = PosToKey(block.GetGlobalCoords());
    float currentTime = m_world->GetGameTime();
    
    auto it = m_torchBurnoutTracker.find(key);
    if (it == m_torchBurnoutTracker.end())
    {
        // 首次切换
        TorchBurnoutEntry entry;
        entry.m_posKey = key;
        entry.m_timestamp = currentTime;
        entry.m_toggleCount = 1;
        m_torchBurnoutTracker[key] = entry;
        return false;
    }
    TorchBurnoutEntry& entry = it->second;
    // 检查时间窗口
    if (currentTime - entry.m_timestamp > TORCH_BURNOUT_WINDOW)
    {
        // 重置计数
        entry.m_timestamp = currentTime;
        entry.m_toggleCount = 1;
        return false;
    }
    // 增加计数
    entry.m_toggleCount++;
    
    if (entry.m_toggleCount >= TORCH_BURNOUT_THRESHOLD)
    {
        // 触发保护：火把熄灭并保持一段时间
        return true;
    }
    return false;
}

void RedstoneSimulator::CleanupBurnoutTracker(float currentTime)
{
}

Direction RedstoneSimulator::GetLeverAttachmentDirection(Block* lever) const
{
    // 拉杆朝向存储在 BlockFacing 中
    // 0 = 附着在下方（地面）
    // 1 = 附着在上方（天花板）
    // 2-5 = 附着在侧边（北、南、东、西）
    uint8_t facing = lever->GetBlockFacing();
    switch (facing)
    {
    case 0: return DIRECTION_DOWN;   // 在地面上
    case 1: return DIRECTION_UP;     // 在天花板上
    case 2: return DIRECTION_NORTH;
    case 3: return DIRECTION_SOUTH;
    case 4: return DIRECTION_EAST;
    case 5: return DIRECTION_WEST;
    default: return DIRECTION_DOWN;
    }
}

uint8_t RedstoneSimulator::GetLeverOutputPower(const BlockIterator& lever) const
{
    if (!lever.IsValid())
        return 0;
    Block* b = lever.GetBlock();
    if (!b || b->m_typeIndex != BLOCK_TYPE_LEVER)
        return 0;
    return b->GetSpecialState() ? 15 : 0;
}

void RedstoneSimulator::PowerAdjacentBlocks(const BlockIterator& source, uint8_t power)
{
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = source.GetNeighborCrossBoundary((Direction)dir);
        if (!neighbor.IsValid())
            continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb)
            continue;
        
        // 红石组件立即更新
        if (IsRedstoneComponent(nb->m_typeIndex))
        {
            ScheduleUpdate(neighbor, 0);
        }
    }
}

bool RedstoneSimulator::CheckRepeaterInput(const BlockIterator& block, Direction inputDir) const
{
    BlockIterator inputBlock = block.GetNeighborCrossBoundary(inputDir);
    if (!inputBlock.IsValid())
        return false;
    
    Block* input = inputBlock.GetBlock();
    if (!input)
        return false;
    
    uint8_t inputType = input->m_typeIndex;
    
    // 红石线
    if (inputType == BLOCK_TYPE_REDSTONE_WIRE)
    {
        return input->GetRedstonePower() > 0;
    }
    
    // 红石块
    if (inputType == BLOCK_TYPE_REDSTONE_BLOCK)
    {
        return true;
    }
    
    // 红石火把
    if (inputType == BLOCK_TYPE_REDSTONE_TORCH)
    {
        return true;
    }
    
    // 另一个中继器（同向输出）
    if (inputType == BLOCK_TYPE_REPEATER_ON)
    {
        Direction otherFacing = (Direction)input->GetBlockFacing();
        // 检查它是否向我们输出
        if (otherFacing == inputDir)
        {
            return true;
        }
    }
    
    // 比较器输出
    if (inputType == BLOCK_TYPE_COMPARATOR_ON)
    {
        Direction compFacing = (Direction)input->GetBlockFacing();
        if (compFacing == inputDir)
        {
            return input->GetRedstonePower() > 0;
        }
    }
    
    // 被强充能的实心方块
    if (IsStronglyPowered(inputBlock))
    {
        return true;
    }
    
    return false;
}

bool RedstoneSimulator::CheckRepeaterLock(const BlockIterator& block) const
{
    Block* repeater = block.GetBlock();
    if (!repeater)
        return false;
    
    Direction facing = (Direction)repeater->GetBlockFacing();
    
    // 获取垂直于朝向的两个方向
    Direction leftDir, rightDir;
    GetPerpendicularDirectionsForLeftAndRight(facing, leftDir, rightDir);
    
    // 检查左侧
    BlockIterator leftBlock = block.GetNeighborCrossBoundary(leftDir);
    if (leftBlock.IsValid())
    {
        Block* left = leftBlock.GetBlock();
        if (left && left->m_typeIndex == BLOCK_TYPE_REPEATER_ON)
        {
            Direction leftFacing = (Direction)left->GetBlockFacing();
            // 左侧中继器指向我们（它的输出方向是我们的右边方向）
            if (leftFacing == GetOppositeDir(leftDir))
            {
                return true;
            }
        }
    }
    // 检查右侧
    BlockIterator rightBlock = block.GetNeighborCrossBoundary(rightDir);
    if (rightBlock.IsValid())
    {
        Block* right = rightBlock.GetBlock();
        if (right && right->m_typeIndex == BLOCK_TYPE_REPEATER_ON)
        {
            Direction rightFacing = (Direction)right->GetBlockFacing();
            if (rightFacing == GetOppositeDir(rightDir))
            {
                return true;
            }
        }
    }
    return false;
}

void RedstoneSimulator::ScheduleDelayedOutput(const BlockIterator& block, uint8_t delayTicks)
{
    Block* repeater = block.GetBlock();
    if (!repeater)
        return;
    
    Direction facing = (Direction)repeater->GetBlockFacing();
    
    // 计划前方方块更新
    BlockIterator output = block.GetNeighborCrossBoundary(facing);
    if (output.IsValid())
    {
        ScheduleUpdate(output, delayTicks);
    }
    
    // 也计划周围方块更新
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        if ((Direction)dir == facing || (Direction)dir == GetOppositeDir(facing))
            continue;  // 跳过输入和输出方向
            
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (neighbor.IsValid())
        {
            ScheduleUpdate(neighbor, delayTicks);
        }
    }
}

uint8_t RedstoneSimulator::GetRepeaterOutputPower(const BlockIterator& repeater, Direction toDir) const
{
    if (!repeater.IsValid())
        return 0;
    
    Block* b = repeater.GetBlock();
    if (!b)
        return 0;
    
    // 只有激活的中继器输出功率
    if (b->m_typeIndex != BLOCK_TYPE_REPEATER_ON)
        return 0;
    
    // 只向前方输出
    Direction facing = (Direction)b->GetBlockFacing();
    if (toDir != facing)
        return 0;
    
    // 中继器输出恒定15级信号（信号增强功能）
    return 15;
}

uint8_t RedstoneSimulator::GetRedstoneBlockOutput(const BlockIterator& block) const
{
    if (!block.IsValid())
        return 0;
    
    Block* b = block.GetBlock();
    if (!b || b->m_typeIndex != BLOCK_TYPE_REDSTONE_BLOCK)
        return 0;
    
    // 红石块向所有方向输出15级信号
    return 15;
}

void RedstoneSimulator::UpdateButton(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    Block* button = block.GetBlock();
    if (!button)
        return;
    uint8_t buttonType = button->m_typeIndex;
    if (buttonType != BLOCK_TYPE_BUTTON_STONE && 
        buttonType != BLOCK_TYPE_BUTTON_WOOD)
        return;
    
    // 如果按钮是按下状态，弹起
    if (button->GetSpecialState())
    {
        button->SetSpecialState(false);
        button->SetRedstonePower(0);
        
        MarkChunkDirty(block);
        
        ScheduleNeighborUpdates(block);
        
        Direction attachDir = GetButtonAttachmentDirection(button);
        BlockIterator attached = block.GetNeighborCrossBoundary(attachDir);
        if (attached.IsValid())
        {
            ScheduleUpdate(attached, 0);
            ScheduleNeighborUpdates(attached);
        }
    }
}

void RedstoneSimulator::UpdateWireConnections(const BlockIterator& block)
{
   if (!block.IsValid())
        return;
    
    Block* wire = block.GetBlock();
    if (!wire)
        return;
    
    // 检查是否是红石线类型
    uint8_t currentType = wire->m_typeIndex;
    if (!IsRedstoneWire(currentType))
        return;
    
    // 获取连接状态
    WireConnections conn = GetWireConnections(block);
    bool north = (conn.m_north != WireConnection::NONE);
    bool south = (conn.m_south != WireConnection::NONE);
    bool east  = (conn.m_east != WireConnection::NONE);
    bool west  = (conn.m_west != WireConnection::NONE);
    
    int connectionCount = (north ? 1 : 0) + (south ? 1 : 0) + 
                          (east ? 1 : 0) + (west ? 1 : 0);
    
    uint8_t newType = currentType;
    uint8_t newFacing = 0;
    
    if (connectionCount == 0)
    {
        // 孤立点
        newType = BLOCK_TYPE_REDSTONE_WIRE_DOT;
    }
    else if (connectionCount == 1)
    {
        // 单连接 - 显示为直线指向那个方向
        if (north || south)
        {
            newType = BLOCK_TYPE_REDSTONE_WIRE_NS;
        }
        else
        {
            newType = BLOCK_TYPE_REDSTONE_WIRE_EW;
        }
    }
    else if (connectionCount == 2)
    {
        if (north && south)
        {
            // 南北直线
            newType = BLOCK_TYPE_REDSTONE_WIRE_NS;
        }
        else if (east && west)
        {
            // 东西直线
            newType = BLOCK_TYPE_REDSTONE_WIRE_EW;
        }
        else
        {
            // L形角落
            newType = BLOCK_TYPE_REDSTONE_WIRE_CORNER;
            newFacing = GetCornerFacing(north, south, east, west);
        }
    }
    else if (connectionCount == 3)
    {
        // T形 - 用十字代替，或者用角落+facing
        newType = BLOCK_TYPE_REDSTONE_WIRE_CROSS;
    }
    else // connectionCount == 4
    {
        // 十字
        newType = BLOCK_TYPE_REDSTONE_WIRE_CROSS;
    }
    
    // 保留原有的信号强度
    uint8_t power = wire->GetRedstonePower();
    
    // 更新方块
    bool changed = (wire->m_typeIndex != newType) || 
                   (wire->GetBlockFacing() != newFacing);
    
    if (changed)
    {
        wire->SetType(newType);
        wire->SetBlockFacing(newFacing);
        wire->SetRedstonePower(power);  // 恢复信号强度
        MarkChunkDirty(block);
    }
}

void RedstoneSimulator::UpdateNeighborWireConnections(const BlockIterator& block)
{
    // 更新自己
    UpdateWireConnections(block);
    
    // 更新四个水平方向的红石线
    Direction horizontalDirs[] = { 
        DIRECTION_NORTH, DIRECTION_SOUTH, 
        DIRECTION_EAST, DIRECTION_WEST 
    };
    
    for (Direction dir : horizontalDirs)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary(dir);
        if (neighbor.IsValid())
        {
            Block* nb = neighbor.GetBlock();
            if (nb && nb->m_typeIndex == BLOCK_TYPE_REDSTONE_WIRE)
            {
                UpdateWireConnections(neighbor);
            }
        }
    }
    
    // 也检查上下方的邻居红石线
    // 上方四周
    BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
    if (above.IsValid())
    {
        for (Direction dir : horizontalDirs)
        {
            BlockIterator aboveSide = above.GetNeighborCrossBoundary(dir);
            if (aboveSide.IsValid())
            {
                Block* b = aboveSide.GetBlock();
                if (b && b->m_typeIndex == BLOCK_TYPE_REDSTONE_WIRE)
                {
                    UpdateWireConnections(aboveSide);
                }
            }
        }
    }
    
    // 下方四周
    BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
    if (below.IsValid())
    {
        for (Direction dir : horizontalDirs)
        {
            BlockIterator belowSide = below.GetNeighborCrossBoundary(dir);
            if (belowSide.IsValid())
            {
                Block* b = belowSide.GetBlock();
                if (b && IsRedstoneWire(b->m_typeIndex))
                {
                    UpdateWireConnections(belowSide);
                }
            }
        }
    }
}

void RedstoneSimulator::ToggleLever(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    Block* lever = block.GetBlock();
    if (!lever || lever->m_typeIndex != BLOCK_TYPE_LEVER)
        return;
    
    //切换状态
    bool wasOn = lever->GetSpecialState();
    bool nowOn = !wasOn;
    
    lever->SetSpecialState(nowOn);
    lever->SetRedstonePower(nowOn ? 15 : 0);
    
    MarkChunkDirty(block);
    
    // 触发红石更新 
    // 拉杆自身
    ScheduleUpdate(block, 0);
    // 附着方块
    Direction attachDir = GetLeverAttachmentDirection(lever);
    BlockIterator attached = block.GetNeighborCrossBoundary(attachDir);
    if (attached.IsValid())
    {
        ScheduleUpdate(attached, 0);
        // 附着方块的邻居也需要更新
        ScheduleNeighborUpdates(attached);
    }
    // 拉杆的邻居
    ScheduleNeighborUpdates(block);
    
    // Block* lever = block.GetBlock();
    // if (!lever || lever->m_typeIndex != BLOCK_TYPE_LEVER)
    //     return;
    //
    // bool newState = !lever->GetSpecialState();
    // lever->SetSpecialState(newState);
    // lever->SetRedstonePower(newState ? 15 : 0);
    //
    // MarkChunkDirty(block);
    // ScheduleNeighborUpdates(block);
    //
    // // 也更新附着方块
    // BlockIterator attached = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
    // if (attached.IsValid())
    // {
    //     ScheduleNeighborUpdates(attached);
    // }
}

void RedstoneSimulator::PressButton(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    Block* button = block.GetBlock();
    if (!button)
        return;
    uint8_t buttonType = button->m_typeIndex;
    if (buttonType != BLOCK_TYPE_BUTTON_STONE && 
        buttonType != BLOCK_TYPE_BUTTON_WOOD)
        return;
    // 如果已经按下，忽略
    if (button->GetSpecialState())
        return;
    
    // 按下按钮
    button->SetSpecialState(true);
    button->SetRedstonePower(15);
    
    MarkChunkDirty(block);
    //触发红石更新
    ScheduleNeighborUpdates(block);
    // 附着方块也需要更新
    Direction attachDir = GetButtonAttachmentDirection(button);
    BlockIterator attached = block.GetNeighborCrossBoundary(attachDir);
    if (attached.IsValid())
    {
        ScheduleUpdate(attached, 0);
        ScheduleNeighborUpdates(attached);
    }
    // 计划自动弹起
    uint8_t delayTicks = (buttonType == BLOCK_TYPE_BUTTON_STONE) 
                         ? STONE_BUTTON_TICKS 
                         : WOOD_BUTTON_TICKS;
    
    ScheduleUpdate(block, delayTicks);
}

Direction RedstoneSimulator::GetButtonAttachmentDirection(Block* button) const
{
    // 与拉杆类似
    uint8_t facing = button->GetBlockFacing();
    
    switch (facing)
    {
    case 0: return DIRECTION_DOWN;   // 地面
    case 1: return DIRECTION_UP;     // 天花板
    case 2: return DIRECTION_NORTH;
    case 3: return DIRECTION_SOUTH;
    case 4: return DIRECTION_EAST;
    case 5: return DIRECTION_WEST;
    default: return DIRECTION_DOWN;
    }
}

void RedstoneSimulator::CycleRepeaterDelay(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    Block* repeater = block.GetBlock();
    if (!repeater)
        return;
    
    if (repeater->m_typeIndex != BLOCK_TYPE_REPEATER &&
        repeater->m_typeIndex != BLOCK_TYPE_REPEATER_ON)
        return;
    
    // 延迟存储在低4位的低2位 (0-3 代表 1-4 tick)
    uint8_t currentDelay = GetRepeaterDelay(repeater);
    uint8_t newDelay = (currentDelay % 4) + 1;
    
    SetRepeaterDelay(repeater, newDelay);
    // uint8_t currentDelay = repeater->GetRedstonePower() & 0x03;
    // uint8_t newDelay = (currentDelay + 1) & 0x03;
    //
    // // 保留其他位，只改延迟
    // repeater->SetRedstonePower((repeater->GetRedstonePower() & 0xFC) | newDelay);
    
    MarkChunkDirty(block);
}

WireConnections RedstoneSimulator::GetWireConnections(const BlockIterator& block) const
{
    WireConnections conn;
    
    if (!block.IsValid())
        return conn;
    
    Block* wire = block.GetBlock();
    if (!wire || wire->m_typeIndex != BLOCK_TYPE_REDSTONE_WIRE)
        return conn;
    
    // 检查4个水平方向
    conn.m_north = GetWireConnectionInDirection(block, DIRECTION_NORTH);
    conn.m_south = GetWireConnectionInDirection(block, DIRECTION_SOUTH);
    conn.m_east = GetWireConnectionInDirection(block, DIRECTION_EAST);
    conn.m_west = GetWireConnectionInDirection(block, DIRECTION_WEST);
    
    return conn;
}

WireConnection RedstoneSimulator::GetWireConnectionInDirection(const BlockIterator& block, Direction dir) const
{
    // ===== 检查同高度连接 =====
    BlockIterator side = block.GetNeighborCrossBoundary(dir);
    if (side.IsValid())
    {
        Block* sideBlock = side.GetBlock();
        if (sideBlock)
        {
            // 直接连接到红石组件
            if (CanConnectToRedstone(sideBlock->m_typeIndex, GetOppositeDir(dir)))
            {
                return WireConnection::SIDE;
            }
        }
    }
    
    // ===== 检查向上连接（爬升） =====
    if (side.IsValid())
    {
        Block* sideBlock = side.GetBlock();
        
        // 侧边方块不透明时，检查上方
        if (sideBlock && sideBlock->IsOpaque())
        {
            BlockIterator aboveSide = side.GetNeighborCrossBoundary(DIRECTION_UP);
            if (aboveSide.IsValid())
            {
                Block* aboveSideBlock = aboveSide.GetBlock();
                if (aboveSideBlock && 
                    aboveSideBlock->m_typeIndex == BLOCK_TYPE_REDSTONE_WIRE)
                {
                    // 检查我们上方是否有阻挡
                    BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
                    if (above.IsValid())
                    {
                        Block* aboveBlock = above.GetBlock();
                        if (!aboveBlock || !aboveBlock->IsOpaque())
                        {
                            return WireConnection::UP;
                        }
                    }
                }
            }
        }
    }
    
    // ===== 检查向下连接（下降） =====
    BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
    if (below.IsValid())
    {
        BlockIterator belowSide = below.GetNeighborCrossBoundary(dir);
        if (belowSide.IsValid())
        {
            Block* belowSideBlock = belowSide.GetBlock();
            if (belowSideBlock && 
                belowSideBlock->m_typeIndex == BLOCK_TYPE_REDSTONE_WIRE)
            {
                // 检查侧边方块是否阻挡
                if (side.IsValid())
                {
                    Block* sideBlock = side.GetBlock();
                    if (!sideBlock || !sideBlock->IsOpaque())
                    {
                        return WireConnection::DOWN;
                    }
                }
            }
        }
    }
    
    return WireConnection::NONE;
}

bool RedstoneSimulator::CanConnectToRedstone(uint8_t blockType, Direction fromDir) const
{
    switch (blockType)
    {
    case BLOCK_TYPE_REDSTONE_WIRE:
    case BLOCK_TYPE_REDSTONE_BLOCK:
    case BLOCK_TYPE_REDSTONE_TORCH:
    case BLOCK_TYPE_REDSTONE_TORCH_OFF:
    case BLOCK_TYPE_LEVER:
    case BLOCK_TYPE_BUTTON_STONE:
    case BLOCK_TYPE_BUTTON_WOOD:
        return true;
        
    case BLOCK_TYPE_REPEATER:
    case BLOCK_TYPE_REPEATER_ON:
        // 中继器只在输入/输出方向连接
        // 需要检查中继器朝向
        return true;  // 简化：始终连接 TODO: 实现方向检查
        
    case BLOCK_TYPE_REDSTONE_LAMP:
    case BLOCK_TYPE_REDSTONE_LAMP_ON:
        return true;
        
    case BLOCK_TYPE_PISTON:
    case BLOCK_TYPE_STICKY_PISTON:
        return true;
        
    default:
        return false;
    }
}

void RedstoneSimulator::OnBlockPlaced(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    Block* b = block.GetBlock();
    if (!b)
        return;
    if (b->m_typeIndex == BLOCK_TYPE_REDSTONE_BLOCK)
    {
        // 红石块放置时，更新周围所有方块
        ScheduleNeighborUpdates(block);
        
        // 也更新斜对角的红石线（爬升连接）
        ScheduleClimbingWireUpdates(block);
    }
    // 如果放置的是红石线，更新自己和邻居的连接
    {
        if (IsRedstoneWire(b->m_typeIndex))
        {
            UpdateNeighborWireConnections(block);
        }
        else
        {
            // 放置其他方块也可能影响红石线连接
            // 更新周围的红石线
            UpdateNeighborWireConnections(block);
        }
    }
    
    ScheduleUpdate(block, 0);
    ScheduleNeighborUpdates(block);
}

void RedstoneSimulator::OnBlockRemoved(const BlockIterator& block, uint8_t oldType)
{
    if (oldType == BLOCK_TYPE_REDSTONE_BLOCK)
    {
        // 红石块移除时，更新周围所有方块
        ScheduleNeighborUpdates(block);
        ScheduleClimbingWireUpdates(block);
    }
    UpdateNeighborWireConnections(block);
    ScheduleNeighborUpdates(block);
}

bool RedstoneSimulator::IsRedstoneBlockPowering(const BlockIterator& block, Direction toDir) const
{
    if (!block.IsValid())
        return false;
    
    Block* b = block.GetBlock();
    if (!b || b->m_typeIndex != BLOCK_TYPE_REDSTONE_BLOCK)
        return false;
    
    // 红石块向所有6个方向提供信号
    return true;
}

int64_t RedstoneSimulator::PosToKey(const IntVec3& pos) const
{
    return ((int64_t)(pos.x + 30000000) << 40) | 
           ((int64_t)(pos.y + 30000000) << 20) | 
           (int64_t)(pos.z + 30000000);
}

Direction RedstoneSimulator::GetOppositeDir(Direction dir) const
{
    switch (dir)
    {
    case DIRECTION_NORTH: return DIRECTION_SOUTH;
    case DIRECTION_SOUTH: return DIRECTION_NORTH;
    case DIRECTION_EAST:  return DIRECTION_WEST;
    case DIRECTION_WEST:  return DIRECTION_EAST;
    case DIRECTION_UP:    return DIRECTION_DOWN;
    case DIRECTION_DOWN:  return DIRECTION_UP;
    default: return DIRECTION_NORTH;
    }
}

void RedstoneSimulator::MarkChunkDirty(const BlockIterator& block)
{
    Chunk* chunk = block.GetChunk();
    if (chunk)
    {
        chunk->MarkSelfDirty();
        m_world->m_hasDirtyChunk = true;
    }
}