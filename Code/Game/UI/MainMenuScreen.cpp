#include "MainMenuScreen.h"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Panel.h"
#include "Engine/UI/Button.h"
#include "Engine/UI/Text.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/Game.hpp"
extern Game* g_theGame;
MainMenuScreen::MainMenuScreen(UISystem* uiSystem)
    : UIScreen(uiSystem, UIScreenType::MAIN_MENU, g_theGame->m_screenCamera, true)
{
}

MainMenuScreen::~MainMenuScreen()
{
}

void MainMenuScreen::Build()
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
    BuildButtons();
}

void MainMenuScreen::BuildBackground()
{
    // 全屏背景（可以是纹理）
    AABB2 screenBounds(0, 0, 1600, 900);
    m_backgroundPanel = new Panel(
        m_canvas,
        screenBounds,
        Rgba8(20, 20, 30),  // 深色背景
        nullptr,  // TODO: 添加背景纹理
        false,
        Rgba8::BLACK
    );
}

void MainMenuScreen::BuildTitle()
{
    TextSetting titleSetting;
    titleSetting.m_text = "MINECRAFT";  // 你的游戏名
    titleSetting.m_color = Rgba8::WHITE;
    titleSetting.m_height = 80.0f;
    
    Vec2 titlePos(800, 700);
    m_titleText = new Text(m_canvas, titlePos, titleSetting);
}

void MainMenuScreen::BuildButtons()
{
    float buttonWidth = 400.0f;
    float buttonHeight = 60.0f;
    float spacing = 25.0f;
    float startY = 500.0f;
    float centerX = 800.0f;
    
    AABB2 singleplayerBounds(
        centerX - buttonWidth * 0.5f, startY,
        centerX + buttonWidth * 0.5f, startY + buttonHeight
    );
    m_singleplayerButton = new Button(
        m_canvas,
        singleplayerBounds,
        Rgba8(120, 180, 120),
        Rgba8::GREY,
        Rgba8::WHITE,
        "Singleplayer",
        Vec2(0.5f, 0.5f)
    );
    
    // Multiplayer 按钮
    startY -= (buttonHeight + spacing);
    AABB2 multiplayerBounds(
        centerX - buttonWidth * 0.5f, startY,
        centerX + buttonWidth * 0.5f, startY + buttonHeight
    );
    m_multiplayerButton = new Button(
        m_canvas,
        multiplayerBounds,
        Rgba8(120, 120, 180),
        Rgba8::GREY,
        Rgba8::WHITE,
        "Multiplayer",
        Vec2(0.5f, 0.5f)
    );
    
    // Settings 按钮
    startY -= (buttonHeight + spacing);
    AABB2 settingsBounds(
        centerX - buttonWidth * 0.5f, startY,
        centerX + buttonWidth * 0.5f, startY + buttonHeight
    );
    m_settingsButton = new Button(
        m_canvas,
        settingsBounds,
        Rgba8(180, 150, 100),
        Rgba8::GREY,
        Rgba8::WHITE,
        "Options",
        Vec2(0.5f, 0.5f)
    );
    
    // Quit 按钮
    startY -= (buttonHeight + spacing);
    AABB2 quitBounds(
        centerX - buttonWidth * 0.5f, startY,
        centerX + buttonWidth * 0.5f, startY + buttonHeight
    );
    m_quitButton = new Button(
        m_canvas,
        quitBounds,
        Rgba8(180, 80, 80),
        Rgba8::GREY,
        Rgba8::WHITE,
        "QuitGame",
        Vec2(0.5f, 0.5f)
    );
}