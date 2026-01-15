#include "HUD.h"

#include "IconAtlas.h"
#include "GameUIManager.h"
#include "Game/Gamecommon.hpp"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Panel.h"
#include "Engine/UI/ProgressBar.h"
#include "Engine/UI/Sprite.h"
#include "Engine/UI/Text.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/Gameplay/Item.h"
#include "Game/Gameplay/PlayerInventory.h"

extern Game* g_theGame;

HUD::HUD(UISystem* uiSystem)
    : UIScreen(uiSystem, UIScreenType::HUD, g_theGame->m_screenCamera, false)
{
}

HUD::~HUD()
{
}

void HUD::Build()
{
    if (!m_canvas || !m_camera)
    {
        return;
    }
    AABB2 bounds = m_camera->GetOrthographicBounds();
    Vec2 size = bounds.GetDimensions();
    m_camera->SetOrthographicView(Vec2(0, 0), size);
    
    BuildCrosshair();
    // BuildHealthBar();
    // BuildArmorBar(); 
    // BuildHungerBar();
    // BuildExperienceBar();
    BuildHotbar();
    //BuildActionText();
    //BuildItemTooltip();
}

void HUD::BuildCrosshair()
{
    Vec2 center = m_camera->GetOrthographicBounds().GetCenter();
    float size = 16.0f;
    
    AABB2 crosshairBounds(
        center.x - size * 0.5f,
        center.y - size * 0.5f,
        center.x + size * 0.5f,
        center.y + size * 0.5f
    );
    IconAtlas* atlas = g_theGameUIManager->m_uiAtlas;
    m_crosshair = new Sprite(m_canvas, crosshairBounds, nullptr,
        ((SpriteSheet*)atlas)->GetSpriteUVs(IntVec2(0, 3)));
    m_crosshair->SetColor(Rgba8::WHITE);
}

// void HUD::BuildHealthBar()
// {
//     AABB2 healthBounds(20, 40, 220, 60);
//     
//     m_healthBar = new ProgressBar(
//         m_canvas,
//         healthBounds,
//         0.0f,      // minValue
//         100.0f,    // maxValue
//         Rgba8(50, 50, 50, 180),    // 背景色（半透明深灰）
//         Rgba8(220, 20, 20),         // 填充色（红色）
//         Rgba8::WHITE,               // 边框色
//         true                        // 有边框
//     );
//     
//     m_healthBar->SetValue(100.0f);  // 初始满血
//     m_elements.push_back(m_healthBar);
// }
//
// void HUD::BuildHungerBar()
// {
//     // 饥饿值条在生命值右侧
//     AABB2 hungerBounds(240, 40, 440, 60);
//     
//     m_hungerBar = new ProgressBar(
//         m_canvas,
//         hungerBounds,
//         0.0f,
//         100.0f,
//         Rgba8(50, 50, 50, 180),
//         Rgba8(205, 133, 63),  // 棕褐色（食物色）
//         Rgba8::WHITE,
//         true
//     );
//     
//     m_hungerBar->SetValue(100.0f);
//     m_elements.push_back(m_hungerBar);
// }
//
// void HUD::BuildArmorBar()
// {
//     AABB2 armorBounds(20, 70, 220, 90);
//     
//     m_armorBar = new ProgressBar(
//         m_canvas,
//         armorBounds,
//         0.0f,      // minValue
//         100.0f,    // maxValue
//         Rgba8(50, 50, 50, 180),      // 背景色（半透明深灰）
//         Rgba8(180, 180, 180),        // 填充色（银灰色，代表护甲）
//         Rgba8::WHITE,                // 边框色
//         true                         // 有边框
//     );
//     
//     m_armorBar->SetValue(0.0f);  // 初始无护甲
//     m_elements.push_back(m_armorBar);
//     
//     // 可选：添加护甲图标
//     // TextSetting iconSetting;
//     // iconSetting.m_text = "🛡";  // 盾牌emoji，或者用纹理
//     // iconSetting.m_color = Rgba8(200, 200, 200);
//     // iconSetting.m_height = 16.0f;
//     // Vec2 iconPos(5, 75);
//     // Text* armorIcon = new Text(m_canvas, iconPos, iconSetting);
//     // m_elements.push_back(armorIcon);
// }
//
// void HUD::BuildExperienceBar()
// {
//     // 经验值条在屏幕底部中央，快捷栏上方
//     AABB2 expBounds(600, 100, 1000, 110);
//     
//     m_experienceBar = new ProgressBar(
//         m_canvas,
//         expBounds,
//         0.0f,
//         100.0f,
//         Rgba8(0, 0, 0, 100),
//         Rgba8(127, 255, 0),  // 绿色经验条
//         Rgba8(0, 200, 0),
//         true
//     );
//     
//     m_experienceBar->SetValue(0.0f);
//     m_elements.push_back(m_experienceBar);
// }

void HUD::BuildHotbar()
{
    Vec2 size = m_camera->GetOrthographicBounds().GetDimensions();
    float hotbarWidth = size.x*0.45f;
    float hotbarHeight = size.x * 0.05f;
    float hotbarX = size.x*0.5f - hotbarWidth * 0.5f;  
    float hotbarY = size.y *0.01f;
    
    AABB2 hotbarBounds(
        hotbarX,
        hotbarY,
        hotbarX + hotbarWidth,
        hotbarY + hotbarHeight
    );
    AABB2 hotbarUV =g_theGameUIManager->m_uiAtlas->GetSpriteUVsByName("HotbarPanel");
    m_hotbarPanel = new Panel(
        m_canvas,
        hotbarBounds,
        Rgba8::WHITE,
        nullptr,
        false,              
        Rgba8(100, 100, 100),
        hotbarUV
    );
    float slotPaddingRatio = 0.01f;  
    float slotPadding = hotbarWidth * slotPaddingRatio;
    float availableWidth = hotbarWidth - slotPadding * 2;  
    float slotSize = (availableWidth - slotPadding * (SLOTS_PER_ROW - 1)) / SLOTS_PER_ROW;
    
    float startX = hotbarX + slotPadding;
    float startY = hotbarY + slotPadding;
    
    float iconPaddingRatio = 0.07f;  // 图标内边距占槽位的比例
    float iconPadding = slotSize * iconPaddingRatio;
    float iconTopPadding = slotSize * 0.21f;  // 图标顶部额外留白
    float countTextHeight = slotSize * 0.2f;  // 数字高度
    float countTextOffsetX = slotSize * 0.29f;  // 数字右侧偏移
    float countTextOffsetY = slotSize * 0.07f;  // 数字底部偏移
    
    for (int i = 0; i < SLOTS_PER_ROW; i++)
    {
        float slotX = startX + i * (slotSize + slotPadding);
        AABB2 slotBounds(
            slotX,
            startY,
            slotX + slotSize,
            startY + slotSize
        );
        Panel* slot = new Panel(
            m_canvas,
            slotBounds,
            Rgba8(0, 0, 0,0),
            nullptr,
            true,
            Rgba8(139, 139, 139),
            AABB2(0,0,0,0)
        );
        m_hotbarSlots.push_back(slot);
        
        AABB2 iconBounds(
            slotX + iconPadding,
            startY + iconTopPadding,
            slotX + slotSize - iconPadding,
            startY + slotSize - iconPadding
        );
        Sprite* icon = new Sprite(m_canvas, iconBounds, nullptr, AABB2(0,0,0,0));
        m_hotbarIcons.push_back(icon);
        
        TextSetting countSetting;
        countSetting.m_text = "";
        countSetting.m_color = Rgba8::WHITE;
        countSetting.m_height = countTextHeight;
        Vec2 countPos(
            slotX + slotSize - countTextOffsetX,
            startY + countTextOffsetY
        );
        Text* countText = new Text(m_canvas, countPos, countSetting);
        m_hotbarCounts.push_back(countText);
    }
    AABB2 uv = g_theGameUIManager->m_uiAtlas->GetSpriteUVsByName("SelectionFrame");
    AABB2 selectionBounds = m_hotbarSlots[0]->GetBounds();
    m_hotbarSelectionFrame = new Panel(
        m_canvas,
        selectionBounds,
        Rgba8::WHITE,
        nullptr,
        false,
        Rgba8::WHITE,
        uv
    );
}
void HUD::BuildActionText()
{
    TextSetting setting;
    setting.m_text = "";
    setting.m_color = Rgba8::WHITE;
    setting.m_height = 30.0f;
    
    Vec2 textPos(800, 350);  // 屏幕中心偏下 TODO
    
    m_actionText = new Text(m_canvas, textPos, setting);
    m_actionText->SetEnabled(false);  
}

void HUD::Update(float deltaSeconds)
{
    UIScreen::Update(deltaSeconds);

    if (m_actionTextTimer > 0.0f)
    {
        m_actionTextTimer -= deltaSeconds;
        
        if (m_actionTextTimer <= 0.0f)
        {
            if (m_actionText)
            {
                m_actionText->SetEnabled(false);
            }
        }
        else
        {
            float alpha = 255.0f;
            if (m_actionTextTimer < 0.5f)
            {
                alpha = m_actionTextTimer / 0.5f * 255.0f;
            }
            
            if (m_actionText)
            {
                Rgba8 color = m_actionText->GetColor();
                color.a = (unsigned char)alpha;
                m_actionText->SetColor(color);
            }
        }
    }
    if (m_tooltipTimer > 0.0f)
    {
        m_tooltipTimer -= deltaSeconds;
        if (m_tooltipTimer <= 0.0f && m_itemTooltip)
        {
            m_itemTooltip->SetEnabled(false);
        }
    }
    
    RefreshHotbarDisplay();
    UpdateSelectionFrame();
}

void HUD::Render() const
{
    g_theRenderer->BindTexture(g_theGameUIManager->m_uiAtlas->GetTexture());
    UIScreen::Render();
}

// void HUD::UpdateHealth(float healthPercent)
// {
//     if (m_healthBar)
//     {
//         m_healthBar->SetValueNormalized(healthPercent);
//     }
// }
//
// void HUD::UpdateHunger(float hungerPercent)
// {
//     if (m_hungerBar)
//     {
//         m_hungerBar->SetValueNormalized(hungerPercent);
//     }
// }
//
// void HUD::UpdateArmor(float armorPercent)
// {
//     if (m_armorBar)
//     {
//         m_armorBar->SetValueNormalized(armorPercent);
//         
//         // 可选：没有护甲时隐藏护甲条
//         if (armorPercent <= 0.0f)
//         {
//             m_armorBar->SetEnabled(false);
//         }
//         else
//         {
//             m_armorBar->SetEnabled(true);
//         }
//     }
// }
//
// void HUD::UpdateExperience(float expPercent)
// {
//     if (m_experienceBar)
//     {
//         m_experienceBar->SetValueNormalized(expPercent);
//     }
// }

void HUD::SetHotbarSlot(int slotIndex, Texture* itemTexture)
{
    if (slotIndex < 0 || slotIndex >= SLOTS_PER_ROW)
    {
        return;
    }
    
    if (m_hotbarIcons[slotIndex])
    {
        m_hotbarIcons[slotIndex]->SetTexture(itemTexture);
    }
}

void HUD::ShowActionMessage(std::string const& message, float duration)
{
    if (m_actionText)
    {
        m_actionText->SetText(message);
        m_actionText->SetEnabled(true);
        m_actionTextTimer = duration;
        m_actionTextDuration = duration;
    }
}

void HUD::BuildItemTooltip()
{
    TextSetting tooltipSetting;
    tooltipSetting.m_text = "";
    tooltipSetting.m_color = Rgba8::WHITE;
    tooltipSetting.m_height = 20.0f;
    
    Vec2 tooltipPos(800, 100);  // 快捷栏上方
    m_itemTooltip = new Text(m_canvas, tooltipPos, tooltipSetting);
    m_itemTooltip->SetEnabled(false);
}

void HUD::RefreshHotbarDisplay()
{
    Player* player = g_theGame->m_player;
    PlayerInventory* inventory = player->GetInventory();
    IconAtlas* atlas = g_theGameUIManager->m_uiAtlas;
    
    if (!inventory || !atlas)
        return;
    
    int activeRow = inventory->GetActiveRow();
    
    for (int i = 0; i < SLOTS_PER_ROW; i++)  
    {
        int absoluteSlot = activeRow * SLOTS_PER_ROW + i;
        Item const& item = inventory->GetHotbarSlot(absoluteSlot);
        
        if (m_hotbarIcons[i])
        {
            if (item.IsEmpty())
            {
                m_hotbarIcons[i]->SetUVs(AABB2(0, 0, 0, 0));
            }
            else
            {
                AABB2 uvs = atlas->GetItemIconUVs(item.GetName());
                m_hotbarIcons[i]->SetUVs(uvs);
            }
        }
    }
}

void HUD::UpdateSelectionFrame()
{
    Player* player = g_theGame->m_player;
    if (!player) return;
    
    PlayerInventory* inventory = player->GetInventory();
    if (!inventory) return;
    
    int slotInRow = inventory->GetSelectedSlotInRow();
    
    if (slotInRow >= 0 && slotInRow < (int)m_hotbarSlots.size())
    {
        AABB2 bounds = m_hotbarSlots[slotInRow]->GetBounds();
        
        if (m_hotbarSelectionFrame)
        {
            m_hotbarSelectionFrame->SetBounds(bounds);
        }
    }
}
