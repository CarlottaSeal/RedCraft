#include "ChestScreen.h"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Panel.h"
#include "Engine/UI/Text.h"
#include "Engine/UI/Sprite.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Game/Game.hpp"
extern Game* g_theGame;
ChestScreen::ChestScreen(UISystem* uiSystem, int chestID)
    : UIScreen(uiSystem, UIScreenType::CHEST, g_theGame->m_screenCamera, true)
    , m_chestID(chestID)
{
}

ChestScreen::~ChestScreen()
{
}

void ChestScreen::Build()
{
    if (!m_canvas || !m_camera)
    {
        return;
    }
    AABB2 bounds = m_camera->GetOrthographicBounds();
    Vec2 size = bounds.GetDimensions();
    m_camera->SetOrthographicView(Vec2(0, 0), size);
    
    BuildBackground();
    BuildChestGrid();
    BuildPlayerGrid();
    BuildDraggedItemSprite();
    RegisterSlotEvents();
}

void ChestScreen::BuildBackground()
{
    // 全屏半透明遮罩
    AABB2 screenBounds(0, 0, 1600, 900);
    m_backgroundDimmer = new Panel(
        m_canvas,
        screenBounds,
        Rgba8(0, 0, 0, 180),
        nullptr,
        false,
        Rgba8::BLACK
    );
    
    // 箱子面板
    AABB2 chestPanelBounds(300, 500, 1300, 750);
    m_chestPanel = new Panel(
        m_canvas,
        chestPanelBounds,
        Rgba8(60, 50, 40),  // 木质颜色
        nullptr,
        true,
        Rgba8(120, 100, 80)
    );
    
    // 玩家背包面板
    AABB2 playerPanelBounds(300, 100, 1300, 450);
    m_playerPanel = new Panel(
        m_canvas,
        playerPanelBounds,
        Rgba8(60, 60, 60),
        nullptr,
        true,
        Rgba8(120, 120, 120)
    );
    
    // 箱子标题
    TextSetting chestTitleSetting;
    chestTitleSetting.m_text = "Chest";
    chestTitleSetting.m_color = Rgba8(220, 220, 220);
    chestTitleSetting.m_height = 28.0f;
    
    Vec2 chestTitlePos(350, 720);
    m_chestTitleText = new Text(m_canvas, chestTitlePos, chestTitleSetting);
    
    // 背包标题
    TextSetting invTitleSetting;
    invTitleSetting.m_text = "Inventory";
    invTitleSetting.m_color = Rgba8(220, 220, 220);
    invTitleSetting.m_height = 28.0f;
    
    Vec2 invTitlePos(350, 420);
    m_inventoryTitleText = new Text(m_canvas, invTitlePos, invTitleSetting);
}

void ChestScreen::BuildChestGrid()
{
    float slotSize = 70.0f;
    float padding = 6.0f;
    Vec2 gridStart(350, 650);
    
    for (int row = 0; row < CHEST_ROWS; row++)
    {
        for (int col = 0; col < CHEST_COLS; col++)
        {
            int slotIndex = row * CHEST_COLS + col;
            
            float x = gridStart.x + col * (slotSize + padding);
            float y = gridStart.y - row * (slotSize + padding);
            
            AABB2 slotBounds(x, y, x + slotSize, y + slotSize);
            
            ItemSlot* slot = new ItemSlot(m_canvas, slotBounds, slotIndex);
            m_chestSlots.push_back(slot);
        }
    }
}

void ChestScreen::BuildPlayerGrid()
{
    float slotSize = 70.0f;
    float padding = 6.0f;
    
    // 主背包
    Vec2 mainGridStart(350, 370);
    for (int row = 0; row < PLAYER_ROWS; row++)
    {
        for (int col = 0; col < PLAYER_COLS; col++)
        {
            int slotIndex = row * PLAYER_COLS + col;
            
            float x = mainGridStart.x + col * (slotSize + padding);
            float y = mainGridStart.y - row * (slotSize + padding);
            
            AABB2 slotBounds(x, y, x + slotSize, y + slotSize);
            
            ItemSlot* slot = new ItemSlot(m_canvas, slotBounds, slotIndex);
            m_playerSlots.push_back(slot);
        }
    }
    
    // 快捷栏
    Vec2 hotbarStart(350, 140);
    for (int i = 0; i < PLAYER_HOTBAR; i++)
    {
        int slotIndex = PLAYER_ROWS * PLAYER_COLS + i;
        
        float x = hotbarStart.x + i * (slotSize + padding);
        float y = hotbarStart.y;
        
        AABB2 slotBounds(x, y, x + slotSize, y + slotSize);
        
        ItemSlot* slot = new ItemSlot(m_canvas, slotBounds, slotIndex);
        m_playerSlots.push_back(slot);
    }
}

void ChestScreen::BuildDraggedItemSprite()
{
    AABB2 spriteBounds(0, 0, 64, 64);
    m_draggedItemSprite = new Sprite(m_canvas, spriteBounds, nullptr);
    m_draggedItemSprite->SetEnabled(false);
}

void ChestScreen::RegisterSlotEvents()
{
    // 箱子槽位事件
    for (size_t i = 0; i < m_chestSlots.size(); i++)
    {
        ItemSlot* slot = m_chestSlots[i];
        if (!slot) continue;
        
        int slotIndex = slot->GetSlotIndex();
        
        slot->OnClickEvent([this, slotIndex]() {
            OnChestSlotClicked(slotIndex);
        });
    }
    
    // 玩家槽位事件
    for (size_t i = 0; i < m_playerSlots.size(); i++)
    {
        ItemSlot* slot = m_playerSlots[i];
        if (!slot) continue;
        
        int slotIndex = slot->GetSlotIndex();
        
        slot->OnClickEvent([this, slotIndex]() {
            OnPlayerSlotClicked(slotIndex);
        });
    }
}

void ChestScreen::Update(float deltaSeconds)
{
    UIScreen::Update(deltaSeconds);
    
    if (m_draggedItem.isDragging)
    {
        UpdateDraggedItemPosition();
    }
}

void ChestScreen::OnEnter()
{
    UIScreen::OnEnter();
    // 播放箱子打开音效
    // g_theAudio->PlaySound("chest_open");
}

void ChestScreen::OnExit()
{
    UIScreen::OnExit();
    
    // 如果还在拖拽，取消
    if (m_draggedItem.isDragging)
    {
        m_draggedItem.isDragging = false;
        m_draggedItemSprite->SetEnabled(false);
    }
    
    // 播放箱子关闭音效
    // g_theAudio->PlaySound("chest_close");
}

void ChestScreen::OnChestSlotClicked(int slotIndex)
{
    if (!IsValidChestSlot(slotIndex))
    {
        return;
    }
    
    ItemSlot* clickedSlot = m_chestSlots[slotIndex];
    
    if (m_draggedItem.isDragging)
    {
        StopDragging(slotIndex, true);
    }
    else if (!clickedSlot->IsEmpty())
    {
        StartDragging(slotIndex, true);
    }
}

void ChestScreen::OnPlayerSlotClicked(int slotIndex)
{
    if (!IsValidPlayerSlot(slotIndex))
    {
        return;
    }
    
    ItemSlot* clickedSlot = m_playerSlots[slotIndex];
    
    if (m_draggedItem.isDragging)
    {
        StopDragging(slotIndex, false);
    }
    else if (!clickedSlot->IsEmpty())
    {
        StartDragging(slotIndex, false);
    }
}

void ChestScreen::StartDragging(int slotIndex, bool isChestSlot)
{
    ItemSlot* slot = isChestSlot ? m_chestSlots[slotIndex] : m_playerSlots[slotIndex];
    
    m_draggedItem.item = slot->TakeItem();
    m_draggedItem.sourceSlotIndex = slotIndex;
    m_draggedItem.isDragging = true;
    
    m_draggedItemSprite->SetTexture(m_draggedItem.item.texture);
    m_draggedItemSprite->SetEnabled(true);
}

void ChestScreen::StopDragging(int slotIndex, bool isChestSlot)
{
    ItemSlot* targetSlot = isChestSlot ? m_chestSlots[slotIndex] : m_playerSlots[slotIndex];
    
    if (targetSlot->IsEmpty())
    {
        targetSlot->SetItem(m_draggedItem.item);
    }
    else if (targetSlot->GetItem().CanStackWith(m_draggedItem.item))
    {
        int remaining = targetSlot->AddItem(m_draggedItem.item);
        if (remaining > 0)
        {
            m_draggedItem.item.count = remaining;
            // TODO: 放回原位
        }
    }
    else
    {
        // 交换
        ItemData targetItem = targetSlot->TakeItem();
        targetSlot->SetItem(m_draggedItem.item);
        // TODO: 将 targetItem 放回原位
    }
    
    m_draggedItem.isDragging = false;
    m_draggedItemSprite->SetEnabled(false);
}

void ChestScreen::UpdateDraggedItemPosition()
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
    float iconSize = 64.0f;
    
    AABB2 newBounds(
        mousePos.x - iconSize * 0.5f,
        mousePos.y - iconSize * 0.5f,
        mousePos.x + iconSize * 0.5f,
        mousePos.y + iconSize * 0.5f
    );
    
    m_draggedItemSprite->SetBounds(newBounds);
}

void ChestScreen::SetChestItem(int slotIndex, ItemData const& item)
{
    if (IsValidChestSlot(slotIndex))
    {
        m_chestSlots[slotIndex]->SetItem(item);
    }
}

void ChestScreen::SetPlayerItem(int slotIndex, ItemData const& item)
{
    if (IsValidPlayerSlot(slotIndex))
    {
        m_playerSlots[slotIndex]->SetItem(item);
    }
}

ItemData ChestScreen::GetChestItem(int slotIndex) const
{
    if (IsValidChestSlot(slotIndex))
    {
        return m_chestSlots[slotIndex]->GetItem();
    }
    return ItemData();
}

ItemData ChestScreen::GetPlayerItem(int slotIndex) const
{
    if (IsValidPlayerSlot(slotIndex))
    {
        return m_playerSlots[slotIndex]->GetItem();
    }
    return ItemData();
}

bool ChestScreen::IsValidChestSlot(int index) const
{
    return index >= 0 && index < (int)m_chestSlots.size();
}

bool ChestScreen::IsValidPlayerSlot(int index) const
{
    return index >= 0 && index < (int)m_playerSlots.size();
}