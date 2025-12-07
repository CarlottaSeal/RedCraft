#pragma once
#include "Engine/Math/AABB2.hpp"

class UISpriteAtlas;
class Texture;
class Renderer;

class MinecraftUIAtlas // 单例，管理 Minecraft UI 素材
{
public:
    static MinecraftUIAtlas& Get();
    
    void Initialize(Renderer* renderer);
    void Shutdown();
    
    UISpriteAtlas* GetIconsAtlas() const { return m_iconsAtlas; }
    UISpriteAtlas* GetWidgetsAtlas() const { return m_widgetsAtlas; }
    
    AABB2 GetHeartFullUVs() const;
    AABB2 GetHeartHalfUVs() const;
    AABB2 GetHeartEmptyUVs() const;
    AABB2 GetHungerFullUVs() const;
    AABB2 GetHungerHalfUVs() const;
    AABB2 GetHungerEmptyUVs() const;
    AABB2 GetArmorFullUVs() const;
    AABB2 GetArmorHalfUVs() const;
    AABB2 GetArmorEmptyUVs() const;
    AABB2 GetHotbarUVs() const;
    AABB2 GetHotbarSelectionUVs() const;
    
    // 获取纹理（用于渲染）
    Texture* GetIconsTexture() const;
    Texture* GetWidgetsTexture() const;
    
private:
    MinecraftUIAtlas() = default;
    ~MinecraftUIAtlas() = default;
    
    void SetupIconsSprites();
    void SetupWidgetsSprites();
    
    UISpriteAtlas* m_iconsAtlas = nullptr;
    UISpriteAtlas* m_widgetsAtlas = nullptr;
    Texture* m_iconsTexture = nullptr;
    Texture* m_widgetsTexture = nullptr;
};



// ============================================================================
// 使用示例
// ============================================================================
/*
// 1. 初始化（在 Game::Startup 中）
MinecraftUIAtlas::Get().Initialize(g_theRenderer);

// 2. 在 HUD 中使用
void HUD::RenderHearts()
{
    MinecraftUIAtlas& atlas = MinecraftUIAtlas::Get();
    UISpriteAtlas* icons = atlas.GetIconsAtlas();
    Texture* texture = atlas.GetIconsTexture();
    
    std::vector<Vertex_PCU> verts;
    
    int maxHearts = m_maxHealth / 2;  // 10 颗心
    float heartSize = 18.0f;
    float spacing = 2.0f;
    Vec2 startPos(20.0f, 50.0f);
    
    for (int i = 0; i < maxHearts; i++)
    {
        float x = startPos.x + i * (heartSize + spacing);
        AABB2 bounds(x, startPos.y, x + heartSize, startPos.y + heartSize);
        
        int heartHealth = m_currentHealth - (i * 2);
        
        // 先画背景
        AABB2 containerUV = icons->GetSpriteUVs("heart_container");
        AddVertsForAABB2D(verts, bounds, Rgba8::WHITE, containerUV.m_mins, containerUV.m_maxs);
        
        // 再画实际心
        AABB2 heartUV;
        if (heartHealth >= 2)
        {
            heartUV = atlas.GetHeartFullUVs();
        }
        else if (heartHealth == 1)
        {
            heartUV = atlas.GetHeartHalfUVs();
        }
        else
        {
            continue;  // 空心已经画了背景
        }
        
        AddVertsForAABB2D(verts, bounds, Rgba8::WHITE, heartUV.m_mins, heartUV.m_maxs);
    }
    
    g_theRenderer->BindTexture(texture);
    g_theRenderer->DrawVertexArray(verts);
}

// 3. 使用 UI Sprite 组件
void SomeScreen::CreateHeartSprite()
{
    MinecraftUIAtlas& atlas = MinecraftUIAtlas::Get();
    
    Sprite* heartSprite = new Sprite(m_canvas, AABB2(100, 100, 118, 118));
    heartSprite->SetSprite(atlas.GetIconsAtlas(), "heart_full");
}

// 4. 关闭（在 Game::Shutdown 中）
MinecraftUIAtlas::Get().Shutdown();
*/