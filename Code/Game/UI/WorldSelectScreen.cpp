#include "WorldSelectScreen.h"

#include "GameUIManager.h"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Text.h"
#include "Engine/UI/Panel.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/UI/Button.h"
#include "Game/Game.hpp"
#include "Game/World.h"

extern Game* g_theGame;

WorldSelectScreen::WorldSelectScreen(UISystem* uiSystem)
    : UIScreen(uiSystem, UIScreenType::WORLD_SELECT, g_theGame->m_screenCamera)
{
}

WorldSelectScreen::~WorldSelectScreen()
{
}

void WorldSelectScreen::Build()
{
    if (!m_canvas)
    {
        return;
    }
    AABB2 bounds = m_camera->GetOrthographicBounds();
    Vec2 screenSize = bounds.GetDimensions();
    m_camera->SetOrthographicView(Vec2(0, 0), screenSize);
    
    //Vec2 screenSize = m_camera->GetOrthographicTopRight() -
    //    m_camera->GetOrthographicBottomLeft();
    //Vec2 center = screenSize * 0.5f;
    //AABB2 backgroundBounds(0.0f, 0.0f, screenSize.x, screenSize.y);
    AABB2 backgroundBounds(m_camera->GetOrthographicBottomLeft(),m_camera->GetOrthographicTopRight());
    Vec2 center = backgroundBounds.GetCenter();
    Panel* background = new Panel(
        m_canvas,
        backgroundBounds,
        Rgba8(20, 20, 30, 200) 
    );
    m_titleText = new Text(
        m_canvas,
        Vec2(center.x, center.y + screenSize.y * 0.15f),
        TextSetting("Select World Type", screenSize.y * 0.06f, Rgba8::WHITE)
    );
    
    float buttonWidth = screenSize.x * 0.2f;
    float buttonHeight = screenSize.x * 0.1f;
    float spacing = screenSize.x * 0.15f;
    
    AABB2 infiniteBounds(
        center.x - buttonWidth - spacing * 0.5f,
        center.y - buttonHeight * 0.5f,
        center.x - spacing * 0.5f,
        center.y + buttonHeight * 0.5f
    );
    m_infiniteButton = new Button(
        m_canvas,                           // Canvas* canvas
        infiniteBounds,                     // AABB2 bound
        Rgba8(50, 100, 50),                // Rgba8 normalColor (深绿色)
        Rgba8(80, 160, 80),                // Rgba8 hoveredColor (亮绿色)
        Rgba8::WHITE,                      // Rgba8 textColor
        "Infinite"                        // std::string text
    );
    m_infiniteButton->OnClickEvent([this]() { OnInfiniteWorldClicked(); });
    
    AABB2 farmBounds(
        center.x + spacing * 0.5f,
        center.y - buttonHeight * 0.5f,
        center.x + buttonWidth + spacing * 0.5f,
        center.y + buttonHeight * 0.5f
    );
    m_farmButton = new Button(
        m_canvas,
        farmBounds,
        Rgba8(100, 150, 50),              
        Rgba8(130, 200, 80),              
        Rgba8::WHITE,
        "Farm World"
    );
    m_farmButton->OnClickEvent([this]() { OnFarmWorldClicked(); });
    
    Text* infiniteDesc = new Text(
        m_canvas,
        Vec2(infiniteBounds.GetCenter().x, infiniteBounds.m_mins.y - 30.0f),
        TextSetting("Endless procedural world", infiniteBounds.GetHeight() * 0.1f, Rgba8(180, 180, 180))
    );
    Text* farmDesc = new Text(
        m_canvas,
        Vec2(farmBounds.GetCenter().x, farmBounds.m_mins.y - 30.0f),
        TextSetting("12x12 farmland chunks", farmBounds.GetHeight()*0.1f, Rgba8(180, 180, 180))
    );
}

void WorldSelectScreen::Update(float deltaSeconds)
{
    UIScreen::Update(deltaSeconds);
    
    //HandleInput();
}

void WorldSelectScreen::Render() const
{
    g_theRenderer->BindTexture(nullptr);
    UIScreen::Render();
}

// void WorldSelectScreen::HandleInput()
// {
//     if (!m_uiSystem || !m_canvas)
//     {
//         return;
//     }
//     InputSystem* input = m_uiSystem->GetInputSystem();
//     if (!input)
//     {
//         return;
//     }
//     Vec2 mousePos = input->GetCursorClientPosition();
//     
//     if (input->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
//     {
//         if (m_infiniteBounds.IsPointInside(mousePos))
//         {
//             OnInfiniteWorldClicked();
//         }
//         else if (m_farmBounds.IsPointInside(mousePos))
//         {
//             OnFarmWorldClicked();
//         }
//     }
//     
//     if (input->WasKeyJustPressed('1'))
//     {
//         OnInfiniteWorldClicked();
//     }
//     else if (input->WasKeyJustPressed('2'))
//     {
//         OnFarmWorldClicked();
//     }
// }
//
// void WorldSelectScreen::UpdateHoverEffects()
// {
//     if (!m_uiSystem || !m_infinitePanel || !m_farmPanel)
//     {
//         return;
//     }
//     
//     InputSystem* input = m_uiSystem->GetInputSystem();
//     if (!input)
//     {
//         return;
//     }
//     Vec2 bounds = m_camera->GetOrthographicBounds().GetDimensions();
//     Vec2 mousePos = input->GetCursorNormalizedPosition();
//     Vec2 mousePosCorrected = mousePos * bounds;
//     if (m_infiniteBounds.IsPointInside(mousePosCorrected))
//     {
//         m_infinitePanel->SetBackgroundColor(Rgba8(80, 160, 80));  // 亮绿色
//     }
//     else
//     {
//         m_infinitePanel->SetBackgroundColor(Rgba8(50, 100, 50));  // 深绿色
//     }
//     
//     if (m_farmBounds.IsPointInside(mousePosCorrected))
//     {
//         m_farmPanel->SetBackgroundColor(Rgba8(130, 200, 80));     // 亮黄绿色
//     }
//     else
//     {
//         m_farmPanel->SetBackgroundColor(Rgba8(100, 150, 50));     // 黄绿色
//     }
// }

void WorldSelectScreen::OnInfiniteWorldClicked()
{
    if (!g_theGame)
    {
        return;
    }
    if (g_theGameUIManager)
    {
        g_theGameUIManager->CloseWorldSelect();  
    }
    g_theGame->m_isInAttractMode = false;
    g_theGame->m_selectedWorldType = WorldGenPipeline::WorldGenMode::NORMAL;
    g_theGame->Startup();
    
    if (g_theGameUIManager)
    {
        g_theGameUIManager->CloseTopScreen();
    }
}

void WorldSelectScreen::OnFarmWorldClicked()
{
    if (!g_theGame)
    {
        return;
    }
    if (g_theGameUIManager)
    {
        g_theGameUIManager->CloseWorldSelect();  
    }
    g_theGame->m_isInAttractMode = false;
    g_theGame->m_selectedWorldType = WorldGenPipeline::WorldGenMode::FLAT_FARM;
    g_theGame->Startup();
    
    if (g_theGameUIManager)
    {
        g_theGameUIManager->CloseTopScreen();
    }
}