// Game/UI/InventoryScreen.cpp
#include "InventoryScreen.h"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Panel.h"
#include "Engine/UI/Text.h"
#include "Engine/UI/Sprite.h"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/Game.hpp"
extern Game* g_theGame;
InventoryScreen::InventoryScreen(UISystem* uiSystem)
    : UIScreen(uiSystem, UIScreenType::INVENTORY, g_theGame->m_screenCamera, true)
{
}

InventoryScreen::~InventoryScreen()
{
}

void InventoryScreen::Build()
{
    if (!m_canvas || !m_camera)
    {
        return;
    }
    AABB2 bounds = m_camera->GetOrthographicBounds();
    Vec2 size = bounds.GetDimensions();
    m_camera->SetOrthographicView(Vec2(0, 0), size);
    
    BuildBackground();
    BuildTitle();
    BuildInventoryGrid();
    BuildDraggedItemSprite();
    RegisterSlotEvents();
}

void InventoryScreen::BuildBackground()
{
    // 全屏半透明黑色遮罩
    AABB2 screenBounds(0, 0, 1600, 900);
    m_backgroundDimmer = new Panel(
        m_canvas,
        screenBounds,
        Rgba8(0, 0, 0, 180),
        nullptr,
        false,
        Rgba8::BLACK
    );

    // 背包主面板
    AABB2 inventoryBounds(350, 150, 1250, 750);
    m_inventoryPanel = new Panel(
        m_canvas,
        inventoryBounds,
        Rgba8(60, 60, 60),  // MC 风格的深灰
        nullptr,
        true,
        Rgba8(120, 120, 120)
    );
}

void InventoryScreen::BuildTitle()
{
    TextSetting titleSetting;
    titleSetting.m_text = "Inventory";
    titleSetting.m_color = Rgba8(220, 220, 220);
    titleSetting.m_height = 36.0f;
    
    Vec2 titlePos(400, 710);
    m_titleText = new Text(m_canvas, titlePos, titleSetting);
    
    // 工具提示（初始隐藏）
    TextSetting tooltipSetting;
    tooltipSetting.m_text = "";
    tooltipSetting.m_color = Rgba8::WHITE;
    tooltipSetting.m_height = 20.0f;
    
    Vec2 tooltipPos(0, 0);  // 动态更新
    m_tooltipText = new Text(m_canvas, tooltipPos, tooltipSetting);
    m_tooltipText->SetEnabled(false);
}

void InventoryScreen::BuildInventoryGrid()
{
    float slotSize = 70.0f;
    float padding = 6.0f;
    
    // === 主背包 (3x9) ===
    Vec2 mainGridStart(400, 580);
    
    for (int row = 0; row < INVENTORY_ROWS; row++)
    {
        for (int col = 0; col < INVENTORY_COLS; col++)
        {
            int slotIndex = row * INVENTORY_COLS + col;
            
            float x = mainGridStart.x + col * (slotSize + padding);
            float y = mainGridStart.y - row * (slotSize + padding);
            
            AABB2 slotBounds(x, y, x + slotSize, y + slotSize);
            
            ItemSlot* slot = new ItemSlot(m_canvas, slotBounds, slotIndex);
            m_itemSlots.push_back(slot);
        }
    }
    
    // === 快捷栏 (1x9) - 与主背包分隔 ===
    Vec2 hotbarStart(400, 350);
    
    for (int i = 0; i < HOTBAR_SLOTS; i++)
    {
        int slotIndex = INVENTORY_ROWS * INVENTORY_COLS + i;
        
        float x = hotbarStart.x + i * (slotSize + padding);
        float y = hotbarStart.y;
        
        AABB2 slotBounds(x, y, x + slotSize, y + slotSize);
        
        ItemSlot* slot = new ItemSlot(m_canvas, slotBounds, slotIndex);
        m_itemSlots.push_back(slot);
    }
    
    // 在快捷栏上方添加分隔线标签
    TextSetting hotbarLabelSetting;
    hotbarLabelSetting.m_text = "Hotbar";
    hotbarLabelSetting.m_color = Rgba8(180, 180, 180);
    hotbarLabelSetting.m_height = 20.0f;
    
    Vec2 hotbarLabelPos(400, 310);
    Text* hotbarLabel = new Text(m_canvas, hotbarLabelPos, hotbarLabelSetting);
}

void InventoryScreen::BuildDraggedItemSprite()
{
    // 创建跟随鼠标的物品图标（初始隐藏）
    AABB2 spriteBounds(0, 0, 64, 64);
    m_draggedItemSprite = new Sprite(m_canvas, spriteBounds, nullptr);
    m_draggedItemSprite->SetEnabled(false);
}

void InventoryScreen::RegisterSlotEvents()
{
    for (size_t i = 0; i < m_itemSlots.size(); i++)
    {
        ItemSlot* slot = m_itemSlots[i];
        if (!slot) continue;
        
        int slotIndex = slot->GetSlotIndex();
        
        // 左键点击
        slot->OnClickEvent([this, slotIndex]() {
            OnSlotLeftClicked(slotIndex);
        });
        
        // 右键点击
        slot->OnRightClickEvent([this, slotIndex]() {
            OnSlotRightClicked(slotIndex);
        });
        
        // 鼠标悬停进入
        slot->OnHoverEnterEvent([this, slotIndex]() {
            OnSlotHoverEnter(slotIndex);
        });
        
        // 鼠标悬停退出
        slot->OnHoverExitEvent([this, slotIndex]() {
            OnSlotHoverExit(slotIndex);
        });
    }
}

void InventoryScreen::Update(float deltaSeconds)
{
    UIScreen::Update(deltaSeconds);
    
    // 更新拖拽物品位置
    if (m_draggedItem.isDragging)
    {
        UpdateDraggedItemPosition();
    }
}

void InventoryScreen::Render() const
{
    UIScreen::Render();
    
    // 拖拽的物品最后渲染（在最上层）
    if (m_draggedItemSprite && m_draggedItem.isDragging)
    {
        m_draggedItemSprite->Render();
    }
}

void InventoryScreen::OnEnter()
{
    UIScreen::OnEnter();
    
    // 播放打开背包音效（如果有）
    // g_theAudio->PlaySound("inventory_open");
}

void InventoryScreen::OnExit()
{
    UIScreen::OnExit();
    
    // 如果还在拖拽，取消拖拽
    if (m_draggedItem.isDragging)
    {
        // 将物品放回原位
        if (IsValidSlotIndex(m_draggedItem.sourceSlotIndex))
        {
            m_itemSlots[m_draggedItem.sourceSlotIndex]->SetItem(m_draggedItem.item);
        }
        m_draggedItem.isDragging = false;
        m_draggedItemSprite->SetEnabled(false);
    }
    
    // 播放关闭背包音效
    // g_theAudio->PlaySound("inventory_close");
}

void InventoryScreen::OnSlotLeftClicked(int slotIndex)
{
    if (!IsValidSlotIndex(slotIndex))
    {
        return;
    }
    
    ItemSlot* clickedSlot = m_itemSlots[slotIndex];
    
    if (m_draggedItem.isDragging)
    {
        // 正在拖拽，放下物品
        StopDragging(slotIndex);
    }
    else
    {
        // 没有拖拽，拿起物品
        if (!clickedSlot->IsEmpty())
        {
            StartDragging(slotIndex);
        }
    }
}

void InventoryScreen::OnSlotRightClicked(int slotIndex)
{
    if (!IsValidSlotIndex(slotIndex))
    {
        return;
    }
    
    ItemSlot* clickedSlot = m_itemSlots[slotIndex];
    
    if (m_draggedItem.isDragging)
    {
        // 正在拖拽，只放一个物品
        if (m_draggedItem.item.count > 0)
        {
            ItemData singleItem = m_draggedItem.item;
            singleItem.count = 1;
            
            int remaining = clickedSlot->AddItem(singleItem);
            
            if (remaining == 0)
            {
                // 成功放置
                m_draggedItem.item.count--;
                
                if (m_draggedItem.item.count <= 0)
                {
                    // 拖拽的物品用完了
                    m_draggedItem.isDragging = false;
                    m_draggedItemSprite->SetEnabled(false);
                }
            }
        }
    }
    else
    {
        // 没有拖拽，拿起一半物品
        if (!clickedSlot->IsEmpty())
        {
            ItemData slotItem = clickedSlot->GetItem();
            int halfCount = (slotItem.count + 1) / 2;  // 向上取整
            int remainCount = slotItem.count - halfCount;
            
            // 拿起一半
            m_draggedItem.item = slotItem;
            m_draggedItem.item.count = halfCount;
            m_draggedItem.sourceSlotIndex = slotIndex;
            m_draggedItem.isDragging = true;
            
            // 剩下一半留在槽位
            if (remainCount > 0)
            {
                slotItem.count = remainCount;
                clickedSlot->SetItem(slotItem);
            }
            else
            {
                clickedSlot->ClearItem();
            }
            
            // 更新拖拽图标
            m_draggedItemSprite->SetTexture(m_draggedItem.item.texture);
            m_draggedItemSprite->SetEnabled(true);
        }
    }
}

void InventoryScreen::OnSlotHoverEnter(int slotIndex)
{
    if (!IsValidSlotIndex(slotIndex))
    {
        return;
    }
    
    ItemSlot* slot = m_itemSlots[slotIndex];
    if (!slot->IsEmpty() && !m_draggedItem.isDragging)
    {
        // 显示物品提示
        InputSystem* input = m_canvas->GetSystemInputSystem();
        if (input)
        {
            Vec2 mousePos = input->GetCursorClientPosition();
            ShowTooltip(slot->GetItem(), mousePos);
        }
    }
}

void InventoryScreen::OnSlotHoverExit(int slotIndex)
{
    UNUSED(slotIndex);
    HideTooltip();
}

void InventoryScreen::StartDragging(int slotIndex)
{
    if (!IsValidSlotIndex(slotIndex))
    {
        return;
    }
    
    ItemSlot* slot = m_itemSlots[slotIndex];
    
    m_draggedItem.item = slot->TakeItem();
    m_draggedItem.sourceSlotIndex = slotIndex;
    m_draggedItem.isDragging = true;
    
    // 设置拖拽图标
    m_draggedItemSprite->SetTexture(m_draggedItem.item.texture);
    m_draggedItemSprite->SetEnabled(true);
    
    // 隐藏提示
    HideTooltip();
}

void InventoryScreen::StopDragging(int targetSlotIndex)
{
    if (!IsValidSlotIndex(targetSlotIndex))
    {
        return;
    }
    
    ItemSlot* targetSlot = m_itemSlots[targetSlotIndex];
    
    if (targetSlot->IsEmpty())
    {
        // 目标槽位为空，直接放入
        targetSlot->SetItem(m_draggedItem.item);
    }
    else if (targetSlot->GetItem().CanStackWith(m_draggedItem.item))
    {
        // 可以堆叠
        int remaining = targetSlot->AddItem(m_draggedItem.item);
        
        if (remaining > 0)
        {
            // 还有剩余，放回原位
            m_draggedItem.item.count = remaining;
            if (IsValidSlotIndex(m_draggedItem.sourceSlotIndex))
            {
                m_itemSlots[m_draggedItem.sourceSlotIndex]->SetItem(m_draggedItem.item);
            }
        }
    }
    else
    {
        // 不能堆叠，交换物品
        ItemData targetItem = targetSlot->TakeItem();
        targetSlot->SetItem(m_draggedItem.item);
        
        if (IsValidSlotIndex(m_draggedItem.sourceSlotIndex))
        {
            m_itemSlots[m_draggedItem.sourceSlotIndex]->SetItem(targetItem);
        }
    }
    
    // 停止拖拽
    m_draggedItem.isDragging = false;
    m_draggedItemSprite->SetEnabled(false);
}

void InventoryScreen::UpdateDraggedItemPosition()
{
    if (!m_draggedItemSprite || !m_canvas)
    {
        return;
    }
    
    InputSystem* input = m_canvas->GetSystemInputSystem();
    if (!input)
    {
        return;
    }
    
    Vec2 mousePos = input->GetCursorClientPosition();
    
    // 物品图标跟随鼠标，稍微偏移
    float iconSize = 64.0f;
    AABB2 newBounds(
        mousePos.x - iconSize * 0.5f,
        mousePos.y - iconSize * 0.5f,
        mousePos.x + iconSize * 0.5f,
        mousePos.y + iconSize * 0.5f
    );
    
    m_draggedItemSprite->SetBounds(newBounds);
}

void InventoryScreen::ShowTooltip(ItemData const& item, Vec2 const& position)
{
    if (!m_tooltipText)
    {
        return;
    }
    
    // 显示物品名称
    std::string tooltipText = item.displayName.empty() ? item.name : item.displayName;
    
    m_tooltipText->SetText(tooltipText);
    m_tooltipText->SetPosition(position + Vec2(15, 15));  // 鼠标右下方
    m_tooltipText->SetEnabled(true);
}

void InventoryScreen::HideTooltip()
{
    if (m_tooltipText)
    {
        m_tooltipText->SetEnabled(false);
    }
}

void InventoryScreen::SetItemSlot(int slotIndex, ItemData const& item)
{
    if (IsValidSlotIndex(slotIndex))
    {
        m_itemSlots[slotIndex]->SetItem(item);
    }
}

ItemData InventoryScreen::GetItemSlot(int slotIndex) const
{
    if (IsValidSlotIndex(slotIndex))
    {
        return m_itemSlots[slotIndex]->GetItem();
    }
    return ItemData();
}

bool InventoryScreen::AddItemToInventory(ItemData const& item)
{
    if (item.IsEmpty())
    {
        return false;
    }
    
    ItemData remaining = item;
    
    // 1. 尝试堆叠到现有物品上
    for (ItemSlot* slot : m_itemSlots)
    {
        if (!slot->IsEmpty() && slot->GetItem().CanStackWith(remaining))
        {
            remaining.count = slot->AddItem(remaining);
            
            if (remaining.count <= 0)
            {
                return true;  // 全部添加成功
            }
        }
    }
    
    // 2. 找空槽位
    while (remaining.count > 0)
    {
        int emptySlot = FindEmptySlot();
        
        if (emptySlot < 0)
        {
            return false;  // 背包满了
        }
        
        remaining.count = m_itemSlots[emptySlot]->AddItem(remaining);
    }
    
    return true;
}

int InventoryScreen::FindEmptySlot() const
{
    for (size_t i = 0; i < m_itemSlots.size(); i++)
    {
        if (m_itemSlots[i]->IsEmpty())
        {
            return (int)i;
        }
    }
    return -1;
}

bool InventoryScreen::IsValidSlotIndex(int index) const
{
    return index >= 0 && index < (int)m_itemSlots.size();
}