#pragma once
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <map>
#include <string>

class Texture;
class Renderer;

// ============================================================================
// GameUIAtlas - 游戏 UI 素材表
// 
// 设计：一张纹理，两种访问方式
//   1. 网格访问：用于规则排列的图标（如物品图标、状态图标）
//   2. 命名访问：用于不规则尺寸的 UI 元素（如按钮、快捷栏）
// 
// 纹理布局建议：
//   ┌────────────────────────────────┐
//   │  网格区域 (Grid Region)        │  <- 上半部分，规则网格
//   │  例如：16x16 的图标网格        │
//   ├────────────────────────────────┤
//   │  自定义区域 (Custom Region)    │  <- 下半部分，不规则元素
//   │  hotbar, buttons, panels...    │
//   └────────────────────────────────┘
// ============================================================================

class GameUIAtlas
{
public:
    // 构造函数 - 加载纹理并设置网格参数
    // gridLayout: 网格划分 (列数, 行数)，IntVec2(1,1) 表示不使用网格
    // gridRegionHeight: 网格区域占纹理高度的像素数（0 = 整张纹理都是网格）
    GameUIAtlas(Renderer* renderer, 
                std::string const& texturePath,
                IntVec2 const& gridLayout = IntVec2(1, 1),
                int gridRegionHeight = 0);
    
    ~GameUIAtlas();
    
    // ========== 网格访问 ==========
    // 通过行列获取 UV（仅在网格区域内有效）
    AABB2 GetGridSpriteUVs(int col, int row) const;
    AABB2 GetGridSpriteUVs(IntVec2 const& coords) const;
    
    // 通过线性索引获取（row * numCols + col）
    AABB2 GetGridSpriteUVsByIndex(int index) const;
    
    // 获取网格信息
    IntVec2 GetGridLayout() const { return m_gridLayout; }
    int GetGridCellWidth() const { return m_cellWidth; }
    int GetGridCellHeight() const { return m_cellHeight; }
    
    // ========== 命名访问 ==========
    // 定义一个命名精灵（像素坐标，相对于纹理左上角）
    void DefineSprite(std::string const& name, int pixelX, int pixelY, int width, int height);
    
    // 定义一个命名精灵（直接用 UV）
    void DefineSpriteUV(std::string const& name, Vec2 const& uvMins, Vec2 const& uvMaxs);
    
    // 通过名称获取 UV
    AABB2 GetSpriteUVs(std::string const& name) const;
    bool HasSprite(std::string const& name) const;
    
    // ========== 通用 ==========
    Texture* GetTexture() const { return m_texture; }
    IntVec2 GetTextureDimensions() const { return m_textureDimensions; }
    
    // 像素坐标转 UV（工具方法）
    AABB2 PixelToUV(int pixelX, int pixelY, int width, int height) const;
    
private:
    Texture* m_texture = nullptr;
    IntVec2 m_textureDimensions;    // 纹理尺寸（像素）
    
    // 网格相关
    IntVec2 m_gridLayout;           // 网格布局 (列数, 行数)
    int m_gridRegionHeight = 0;     // 网格区域高度（像素），0 = 整张纹理
    int m_cellWidth = 0;            // 单元格宽度（像素）
    int m_cellHeight = 0;           // 单元格高度（像素）
    
    // 命名精灵
    std::map<std::string, AABB2> m_namedSprites;
};

// ============================================================================
// GameUIAtlas 使用示例
// ============================================================================

/*
推荐的纹理布局 (GameUI.png - 256x256):

    0                                                    255
  0 ┌──────────────────────────────────────────────────────┐
    │                                                      │
    │              网格区域 (Grid Region)                  │
    │           16x8 网格，每格 16x16 像素                 │
    │                                                      │
    │   用于：作物图标、状态图标、小图标等                 │
    │                                                      │
128 ├──────────────────────────────────────────────────────┤
    │                                                      │
    │             自定义区域 (Custom Region)               │
    │                                                      │
    │   hotbar (182x22)                                    │
    │   hotbar_selection (24x24)                           │
    │   button_normal (200x20)                             │
    │   button_hover (200x20)                              │
    │   progress_bar_bg (100x10)                           │
    │   progress_bar_fill (100x10)                         │
    │   ...                                                │
    │                                                      │
255 └──────────────────────────────────────────────────────┘

*/

// ============================================================================
// 创建和初始化
// ============================================================================

// // 方法1：在 Game 或 GameUIManager 中作为成员
// class GameUIManager
// {
// private:
//     GameUIAtlas* m_uiAtlas = nullptr;
//     
// public:
//     void Startup(Renderer* renderer)
//     {
//         // 创建 Atlas
//         // 参数：渲染器, 纹理路径, 网格布局(16列x8行), 网格区域高度(128像素)
//         m_uiAtlas = new GameUIAtlas(
//             renderer,
//             "Data/Images/UI/GameUI.png",
//             IntVec2(16, 8),   // 16列 x 8行 的网格
//             128               // 网格区域占上半部分 128 像素
//         );
//         
//         // 定义自定义区域的精灵（像素坐标相对于纹理左上角）
//         SetupCustomSprites();
//     }
//     
//     void SetupCustomSprites()
//     {
//         // Y=128 开始是自定义区域
//         m_uiAtlas->DefineSprite("hotbar",            0, 128, 182, 22);
//         m_uiAtlas->DefineSprite("hotbar_selection",  0, 150, 24, 24);
//         m_uiAtlas->DefineSprite("button_normal",     0, 174, 200, 20);
//         m_uiAtlas->DefineSprite("button_hover",      0, 194, 200, 20);
//         m_uiAtlas->DefineSprite("button_disabled",   0, 214, 200, 20);
//         m_uiAtlas->DefineSprite("progress_bg",       0, 234, 100, 10);
//         m_uiAtlas->DefineSprite("progress_fill",     0, 244, 100, 10);
//         
//         // 也可以在网格区域外定义其他元素
//         m_uiAtlas->DefineSprite("crosshair",         200, 128, 16, 16);
//     }
//     
//     void Shutdown()
//     {
//         delete m_uiAtlas;
//         m_uiAtlas = nullptr;
//     }
//     
//     GameUIAtlas* GetUIAtlas() const { return m_uiAtlas; }
// };
//
// // ============================================================================
// // 使用示例 - 网格访问
// // ============================================================================
//
// void SomeScreen::RenderCropIcon(int cropType, AABB2 const& bounds)
// {
//     GameUIAtlas* atlas = g_theGameUIManager->GetUIAtlas();
//     
//     // 假设作物图标在网格中的位置：
//     // 小麦: (0, 0), 胡萝卜: (1, 0), 土豆: (2, 0), 甜菜: (3, 0)
//     // 南瓜: (0, 1), 西瓜: (1, 1), ...
//     
//     int col = cropType % 16;
//     int row = cropType / 16;
//     
//     AABB2 uvs = atlas->GetGridSpriteUVs(col, row);
//     Texture* tex = atlas->GetTexture();
//     
//     std::vector<Vertex_PCU> verts;
//     AddVertsForAABB2D(verts, bounds, Rgba8::WHITE, uvs.m_mins, uvs.m_maxs);
//     
//     g_theRenderer->BindTexture(tex);
//     g_theRenderer->DrawVertexArray(verts);
// }
//
// // 或者用索引访问
// void SomeScreen::RenderIconByIndex(int iconIndex, AABB2 const& bounds)
// {
//     GameUIAtlas* atlas = g_theGameUIManager->GetUIAtlas();
//     
//     AABB2 uvs = atlas->GetGridSpriteUVsByIndex(iconIndex);
//     
//     // ... 渲染
// }
//
// // ============================================================================
// // 使用示例 - 命名访问
// // ============================================================================
//
// void HUD::BuildHotbar()
// {
//     GameUIAtlas* atlas = g_theGameUIManager->GetUIAtlas();
//     Texture* tex = atlas->GetTexture();
//     
//     // 快捷栏背景
//     AABB2 hotbarUVs = atlas->GetSpriteUVs("hotbar");
//     
//     float scale = 2.0f;  // 放大两倍
//     float hotbarWidth = 182.0f * scale;
//     float hotbarHeight = 22.0f * scale;
//     float hotbarX = 800 - hotbarWidth * 0.5f;
//     float hotbarY = 10.0f;
//     
//     AABB2 hotbarBounds(hotbarX, hotbarY, hotbarX + hotbarWidth, hotbarY + hotbarHeight);
//     
//     // 创建带纹理的 Panel 或 Sprite
//     m_hotbarSprite = new Sprite(m_canvas, hotbarBounds, tex);
//     m_hotbarSprite->SetUVs(hotbarUVs);
//     m_elements.push_back(m_hotbarSprite);
//     
//     // 选中框
//     AABB2 selectionUVs = atlas->GetSpriteUVs("hotbar_selection");
//     // ...
// }
//
// void SomeScreen::CreateButton(AABB2 const& bounds, std::string const& text)
// {
//     GameUIAtlas* atlas = g_theGameUIManager->GetUIAtlas();
//     
//     AABB2 normalUVs = atlas->GetSpriteUVs("button_normal");
//     AABB2 hoverUVs = atlas->GetSpriteUVs("button_hover");
//     AABB2 disabledUVs = atlas->GetSpriteUVs("button_disabled");
//     
//     // 创建按钮，设置三种状态的 UV
//     // ...
// }
//
// // ============================================================================
// // 定义图标枚举（可选，让代码更清晰）
// // ============================================================================
//
// namespace UIIcons
// {
//     // 网格图标索引
//     enum GridIcon
//     {
//         // 第 0 行：作物
//         CROP_WHEAT = 0,
//         CROP_CARROT = 1,
//         CROP_POTATO = 2,
//         CROP_BEETROOT = 3,
//         CROP_PUMPKIN = 4,
//         CROP_MELON = 5,
//         
//         // 第 1 行：红石组件
//         REDSTONE_REPEATER = 16,
//         REDSTONE_COMPARATOR = 17,
//         REDSTONE_OBSERVER = 18,
//         REDSTONE_PISTON = 19,
//         REDSTONE_STICKY_PISTON = 20,
//         
//         // 第 2 行：状态图标
//         ICON_ARROW_UP = 32,
//         ICON_ARROW_DOWN = 33,
//         ICON_ARROW_LEFT = 34,
//         ICON_ARROW_RIGHT = 35,
//         ICON_CHECK = 36,
//         ICON_CROSS = 37,
//     };
//     
//     // 命名精灵
//     namespace Named
//     {
//         constexpr const char* HOTBAR = "hotbar";
//         constexpr const char* HOTBAR_SELECTION = "hotbar_selection";
//         constexpr const char* BUTTON_NORMAL = "button_normal";
//         constexpr const char* BUTTON_HOVER = "button_hover";
//         constexpr const char* BUTTON_DISABLED = "button_disabled";
//         constexpr const char* PROGRESS_BG = "progress_bg";
//         constexpr const char* PROGRESS_FILL = "progress_fill";
//         constexpr const char* CROSSHAIR = "crosshair";
//     }
// }
//
// // 使用：
// void Example()
// {
//     GameUIAtlas* atlas = g_theGameUIManager->GetUIAtlas();
//     
//     // 网格访问
//     AABB2 wheatUVs = atlas->GetGridSpriteUVsByIndex(UIIcons::CROP_WHEAT);
//     AABB2 observerUVs = atlas->GetGridSpriteUVsByIndex(UIIcons::REDSTONE_OBSERVER);
//     
//     // 命名访问
//     AABB2 hotbarUVs = atlas->GetSpriteUVs(UIIcons::Named::HOTBAR);
// }