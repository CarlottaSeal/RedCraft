#include "MinecraftUIAtlas.h"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/UI/UISpriteAtlas.h"

MinecraftUIAtlas& MinecraftUIAtlas::Get()
{
    static MinecraftUIAtlas instance;
    return instance;
}

void MinecraftUIAtlas::Initialize(Renderer* renderer)
{
    // 加载纹理（确保文件存在）
    m_iconsTexture = renderer->CreateOrGetTextureFromFile("Data/Images/UI/icons.png");
    m_widgetsTexture = renderer->CreateOrGetTextureFromFile("Data/Images/UI/widgets.png");
    
    // 创建精灵表
    if (m_iconsTexture)
    {
        m_iconsAtlas = new UISpriteAtlas(*m_iconsTexture, IntVec2(1, 1));
        SetupIconsSprites();
    }
    
    if (m_widgetsTexture)
    {
        m_widgetsAtlas = new UISpriteAtlas(*m_widgetsTexture, IntVec2(1, 1));
        SetupWidgetsSprites();
    }
}

void MinecraftUIAtlas::Shutdown()
{
    delete m_iconsAtlas;
    delete m_widgetsAtlas;
    m_iconsAtlas = nullptr;
    m_widgetsAtlas = nullptr;
}

// ============================================================================
// icons.png 布局 (256x256)
// 
// 注意：像素坐标是从图片左上角开始的！
// Y=0 是图片顶部，Y=255 是图片底部
// ============================================================================
void MinecraftUIAtlas::SetupIconsSprites()
{
    if (!m_iconsAtlas) return;
    
    // ========== 心形图标（第0行，y=0）==========
    // 背景/容器
    m_iconsAtlas->DefineSprite("heart_container",       16, 0, 9, 9);
    m_iconsAtlas->DefineSprite("heart_container_blink", 25, 0, 9, 9);
    
    // 普通心
    m_iconsAtlas->DefineSprite("heart_full",            52, 0, 9, 9);
    m_iconsAtlas->DefineSprite("heart_half",            61, 0, 9, 9);
    m_iconsAtlas->DefineSprite("heart_empty",           16, 0, 9, 9);  // 同 container
    
    // 硬核心（y=45）
    m_iconsAtlas->DefineSprite("heart_hardcore_full",   52, 45, 9, 9);
    m_iconsAtlas->DefineSprite("heart_hardcore_half",   61, 45, 9, 9);
    
    // 中毒心（绿色）
    m_iconsAtlas->DefineSprite("heart_poison_full",     88, 0, 9, 9);
    m_iconsAtlas->DefineSprite("heart_poison_half",     97, 0, 9, 9);
    
    // 凋零心（黑色）
    m_iconsAtlas->DefineSprite("heart_wither_full",     124, 0, 9, 9);
    m_iconsAtlas->DefineSprite("heart_wither_half",     133, 0, 9, 9);
    
    // 吸收心（金色）
    m_iconsAtlas->DefineSprite("heart_absorb_full",     160, 0, 9, 9);
    m_iconsAtlas->DefineSprite("heart_absorb_half",     169, 0, 9, 9);
    
    // ========== 护甲图标（第1行，y=9）==========
    m_iconsAtlas->DefineSprite("armor_empty",           16, 9, 9, 9);
    m_iconsAtlas->DefineSprite("armor_half",            25, 9, 9, 9);
    m_iconsAtlas->DefineSprite("armor_full",            34, 9, 9, 9);
    
    // ========== 氧气图标（第2行，y=18）==========
    m_iconsAtlas->DefineSprite("bubble_full",           16, 18, 9, 9);
    m_iconsAtlas->DefineSprite("bubble_pop",            25, 18, 9, 9);
    
    // ========== 饥饿图标（第3行，y=27）==========
    m_iconsAtlas->DefineSprite("hunger_container",      16, 27, 9, 9);
    m_iconsAtlas->DefineSprite("hunger_empty",          16, 27, 9, 9);  // 同 container
    m_iconsAtlas->DefineSprite("hunger_full",           52, 27, 9, 9);
    m_iconsAtlas->DefineSprite("hunger_half",           61, 27, 9, 9);
    
    // 饥饿效果（绿色）
    m_iconsAtlas->DefineSprite("hunger_effect_full",    88, 27, 9, 9);
    m_iconsAtlas->DefineSprite("hunger_effect_half",    97, 27, 9, 9);
    
    // ========== 经验条（y=64）==========
    m_iconsAtlas->DefineSprite("exp_bar_background",    0, 64, 182, 5);
    m_iconsAtlas->DefineSprite("exp_bar_progress",      0, 69, 182, 5);
    
    // ========== 马的心（y=9，右侧）==========
    m_iconsAtlas->DefineSprite("mount_heart_full",      88, 9, 9, 9);
    m_iconsAtlas->DefineSprite("mount_heart_half",      97, 9, 9, 9);
}

// ============================================================================
// widgets.png 布局 (256x256)
// ============================================================================
void MinecraftUIAtlas::SetupWidgetsSprites()
{
    if (!m_widgetsAtlas) return;
    
    // ========== 快捷栏 ==========
    m_widgetsAtlas->DefineSprite("hotbar",              0, 0, 182, 22);
    m_widgetsAtlas->DefineSprite("hotbar_selection",    0, 22, 24, 24);
    
    // 副手槽
    m_widgetsAtlas->DefineSprite("offhand_slot",        24, 22, 29, 24);
    
    // ========== 按钮 ==========
    m_widgetsAtlas->DefineSprite("button",              0, 66, 200, 20);
    m_widgetsAtlas->DefineSprite("button_highlighted",  0, 86, 200, 20);
    m_widgetsAtlas->DefineSprite("button_disabled",     0, 46, 200, 20);
    
    // ========== 跳跃条 ==========
    m_widgetsAtlas->DefineSprite("jump_bar_bg",         0, 84, 182, 5);
    m_widgetsAtlas->DefineSprite("jump_bar_fill",       0, 89, 182, 5);
}

// ========== 便捷方法 ==========
AABB2 MinecraftUIAtlas::GetHeartFullUVs() const
{
    return m_iconsAtlas ? m_iconsAtlas->GetSpriteUVs("heart_full") : AABB2::ZERO_TO_ONE;
}

AABB2 MinecraftUIAtlas::GetHeartHalfUVs() const
{
    return m_iconsAtlas ? m_iconsAtlas->GetSpriteUVs("heart_half") : AABB2::ZERO_TO_ONE;
}

AABB2 MinecraftUIAtlas::GetHeartEmptyUVs() const
{
    return m_iconsAtlas ? m_iconsAtlas->GetSpriteUVs("heart_empty") : AABB2::ZERO_TO_ONE;
}

AABB2 MinecraftUIAtlas::GetHungerFullUVs() const
{
    return m_iconsAtlas ? m_iconsAtlas->GetSpriteUVs("hunger_full") : AABB2::ZERO_TO_ONE;
}

AABB2 MinecraftUIAtlas::GetHungerHalfUVs() const
{
    return m_iconsAtlas ? m_iconsAtlas->GetSpriteUVs("hunger_half") : AABB2::ZERO_TO_ONE;
}

AABB2 MinecraftUIAtlas::GetHungerEmptyUVs() const
{
    return m_iconsAtlas ? m_iconsAtlas->GetSpriteUVs("hunger_empty") : AABB2::ZERO_TO_ONE;
}

AABB2 MinecraftUIAtlas::GetArmorFullUVs() const
{
    return m_iconsAtlas ? m_iconsAtlas->GetSpriteUVs("armor_full") : AABB2::ZERO_TO_ONE;
}

AABB2 MinecraftUIAtlas::GetArmorHalfUVs() const
{
    return m_iconsAtlas ? m_iconsAtlas->GetSpriteUVs("armor_half") : AABB2::ZERO_TO_ONE;
}

AABB2 MinecraftUIAtlas::GetArmorEmptyUVs() const
{
    return m_iconsAtlas ? m_iconsAtlas->GetSpriteUVs("armor_empty") : AABB2::ZERO_TO_ONE;
}

AABB2 MinecraftUIAtlas::GetHotbarUVs() const
{
    return m_widgetsAtlas ? m_widgetsAtlas->GetSpriteUVs("hotbar") : AABB2::ZERO_TO_ONE;
}

AABB2 MinecraftUIAtlas::GetHotbarSelectionUVs() const
{
    return m_widgetsAtlas ? m_widgetsAtlas->GetSpriteUVs("hotbar_selection") : AABB2::ZERO_TO_ONE;
}

Texture* MinecraftUIAtlas::GetIconsTexture() const
{
    return m_iconsTexture;
}

Texture* MinecraftUIAtlas::GetWidgetsTexture() const
{
    return m_widgetsTexture;
}