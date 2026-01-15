#include "PauseMenuScreen.h"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Panel.h"
#include "Engine/UI/Button.h"
#include "Engine/UI/Text.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Game/Game.hpp"
extern Game* g_theGame;

PauseMenuScreen::PauseMenuScreen(UISystem* uiSystem)
    : UIScreen(uiSystem, UIScreenType::PAUSE_MENU, g_theGame->m_screenCamera, true)
{
}

PauseMenuScreen::~PauseMenuScreen()
{
}

void PauseMenuScreen::Build()
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

void PauseMenuScreen::BuildBackground()
{
    // 全屏半透明遮罩
    AABB2 screenBounds(0, 0, 1600, 900);
    m_backgroundDimmer = new Panel(
        m_canvas,
        screenBounds,
        Rgba8(0, 0, 0, 200),
        nullptr,
        false,
        Rgba8::BLACK
    );
    
    // 菜单面板
    AABB2 menuBounds(550, 200, 1050, 700);
    m_menuPanel = new Panel(
        m_canvas,
        menuBounds,
        Rgba8(50, 50, 50),
        nullptr,
        true,
        Rgba8(120, 120, 120)
    );
}

void PauseMenuScreen::BuildTitle()
{
    TextSetting titleSetting;
    titleSetting.m_text = "Game Paused";
    titleSetting.m_color = Rgba8(230, 230, 230);
    titleSetting.m_height = 48.0f;
    
    Vec2 titlePos(800, 650);  // 居中显示 TODO
    m_titleText = new Text(m_canvas, titlePos, titleSetting);
}

void PauseMenuScreen::BuildButtons()
{
    float buttonWidth = 350.0f;
    float buttonHeight = 55.0f;
    float spacing = 20.0f;
    float startY = 560.0f;
    float centerX = 800.0f;
    
    // Resume 按钮
    AABB2 resumeBounds(
        centerX - buttonWidth * 0.5f, startY,
        centerX + buttonWidth * 0.5f, startY + buttonHeight
    );
    m_resumeButton = new Button(
        m_canvas,
        resumeBounds,
        Rgba8(100, 200, 100),  // 绿色 hover
        Rgba8::WHITE,
        Rgba8::GREY,
        "ResumeGame",
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
        Rgba8(100, 150, 200),  // 蓝色 hover
        Rgba8::WHITE,
        Rgba8::GREY,
        "OpenSettings",
        Vec2(0.5f, 0.5f)
    );
    
    // Save Game 按钮
    startY -= (buttonHeight + spacing);
    AABB2 saveBounds(
        centerX - buttonWidth * 0.5f, startY,
        centerX + buttonWidth * 0.5f, startY + buttonHeight
    );
    m_saveButton = new Button(
        m_canvas,
        saveBounds,
        Rgba8(200, 180, 100),  // 黄色 hover
        Rgba8::WHITE,
        Rgba8::GREY,
        "SaveGame",
        Vec2(0.5f, 0.5f)
    );
    
    // Main Menu 按钮
    startY -= (buttonHeight + spacing);
    AABB2 mainMenuBounds(
        centerX - buttonWidth * 0.5f, startY,
        centerX + buttonWidth * 0.5f, startY + buttonHeight
    );
    m_mainMenuButton = new Button(
        m_canvas,
        mainMenuBounds,
        Rgba8(200, 120, 100),  // 橙红色 hover
        Rgba8::WHITE,
        Rgba8::GREY,
        "BackToMainMenu",
        Vec2(0.5f, 0.5f)
    );
    
    // Quit Game 按钮
    startY -= (buttonHeight + spacing);
    AABB2 quitBounds(
        centerX - buttonWidth * 0.5f, startY,
        centerX + buttonWidth * 0.5f, startY + buttonHeight
    );
    m_quitButton = new Button(
        m_canvas,
        quitBounds,
        Rgba8(200, 80, 80),  // 红色 hover
        Rgba8::WHITE,
        Rgba8::GREY,
        "QuitGame",
        Vec2(0.5f, 0.5f)
    );
}

void PauseMenuScreen::OnEnter()
{
    UIScreen::OnEnter();
    // 暂停游戏音乐
    // g_theAudio->PauseMusic();
}

void PauseMenuScreen::OnExit()
{
    UIScreen::OnExit();
    // 恢复游戏音乐
    // g_theAudio->ResumeMusic();
}