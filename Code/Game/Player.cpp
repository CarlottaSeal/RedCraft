#include "Player.hpp"
#include "Game.hpp"
#include "App.hpp"
#include "World.h"
#include "Physics/GameCamera.h"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Window/Window.hpp"
#include "Gameplay/PlayerInventory.h"
#include "UI/GameUIManager.h"
#include "UI/HUD.h"

extern InputSystem* g_theInput;
extern Window* g_theWindow;
extern App* g_theApp;

Player::Player(Game* owner, Vec3 const& startPos)
	: Entity(owner, startPos)
{
	m_runSpeed = 12.f;
	
	// Player dimensions (1.80m tall, 0.60m wide)
	m_physicsHeight = 1.80f;
	m_physicsRadius = 0.30f;  // Half width = 0.30m
	m_eyeHeight = 1.65f;
	
	// Update physics body
	m_physicsBody.m_mins = Vec3(-m_physicsRadius, -m_physicsRadius, 0.0f);
	m_physicsBody.m_maxs = Vec3(m_physicsRadius, m_physicsRadius, m_physicsHeight);
	
	// Initialize orientation
	m_orientation = EulerAngles(45.0f, 0.0f, 0.0f);
	
	m_gameCamera = new GameCamera();
	m_gameCamera->SetPosition(GetEyePosition());
	m_gameCamera->SetOrientation(m_orientation);
	
	m_worldCamera.SetPosition(GetEyePosition());
	m_worldCamera.SetOrientation(m_orientation);

	m_inventory = new PlayerInventory();
}

Player::~Player()
{
	if (m_gameCamera)
	{
		delete m_gameCamera;
		m_gameCamera = nullptr;
	}
	if (m_inventory)
	{
		delete m_inventory;
		m_inventory = nullptr;
	}
}

void Player::Update(float deltaSeconds)
{
	UpdateInput(deltaSeconds);
	
	Entity::Update(deltaSeconds);
	
	if (m_gameCamera)
	{
		m_gameCamera->Update(deltaSeconds, this, m_game->m_currentWorld);
		
		// Sync world camera with game camera
		m_worldCamera.SetPosition(m_gameCamera->GetPosition());
		m_worldCamera.SetOrientation(m_gameCamera->GetOrientation());

		HandleHotbarInput();
	}
}

void Player::Render() const
{
	// Only render entity bounds if not in first person
	if (m_gameCamera && m_gameCamera->GetCameraMode() != GameCameraMode::FIRST_PERSON)
	{
		Entity::Render();
	}
}

void Player::UpdateInput(float deltaSeconds)
{
	// Check for mode cycling
	if (g_theInput->WasKeyJustPressed('V'))
	{
		CyclePhysicsMode();
	}
	if (g_theInput->WasKeyJustPressed('C'))
	{
		CycleCameraMode();
	}

	XboxController const& controller = g_theInput->GetController(0);

	if (m_gameCamera)
	{
		GameCameraMode cameraMode = m_gameCamera->GetCameraMode();
		
		// Only allow player orientation control in these modes
		if (cameraMode == GameCameraMode::FIRST_PERSON ||
			cameraMode == GameCameraMode::OVER_SHOULDER ||
			cameraMode == GameCameraMode::FIXED_ANGLE_TRACKING ||
			cameraMode == GameCameraMode::INDEPENDENT)
		{
			Vec2 mouseDelta = g_theInput->GetCursorClientDelta();
			m_orientation.m_yawDegrees -= mouseDelta.x * 0.075f;
			m_orientation.m_pitchDegrees += mouseDelta.y * 0.075f;
			
			m_orientation.m_yawDegrees -= controller.GetRightStick().GetPosition().x * 90.0f * deltaSeconds;
			m_orientation.m_pitchDegrees += controller.GetRightStick().GetPosition().y * 90.0f * deltaSeconds;
			
			m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.0f, 85.0f);
			m_orientation.m_yawDegrees = fmodf(m_orientation.m_yawDegrees, 360.0f);
		}
	}
	
	UpdateFromKeyboard(deltaSeconds);
	UpdateFromController(deltaSeconds);
	
	if (g_theInput->IsKeyDown(KEYCODE_SPACE) || controller.IsButtonDown(XboxButtonID::A))
	{
		Jump();
	}
	
}

void Player::UpdateFromKeyboard(float deltaSeconds)
{
	m_isRunning = g_theInput->IsKeyDown(KEYCODE_SHIFT);
	
	if (m_isRunning)
	{
		m_maxSpeed = m_runSpeed;
		m_flySpeed = m_runSpeed * 2.0f;
	}
	else
	{
		m_maxSpeed = m_normalSpeed;
		m_flySpeed = m_normalSpeed * 2.0f;
	}
	
	Vec3 moveDir = Vec3();
	
	Vec3 forward = GetForwardVector();
	Vec3 left = GetLeftVector();
	
	if (m_physicsMode == PhysicsMode::WALKING)
	{
		forward.z = 0.0f;
		if (forward.GetLengthSquared() > 0.0f)
			forward = forward.GetNormalized();
		
		left.z = 0.0f;
		if (left.GetLengthSquared() > 0.0f)
			left = left.GetNormalized();
	}
	
	if (g_theInput->IsKeyDown('W'))
		moveDir += forward;
	if (g_theInput->IsKeyDown('S'))
		moveDir -= forward;
	if (g_theInput->IsKeyDown('A'))
		moveDir += left;
	if (g_theInput->IsKeyDown('D'))
		moveDir -= left;

	if (m_physicsMode == PhysicsMode::FLYING || m_physicsMode == PhysicsMode::NOCLIP)
	{
		if (g_theInput->IsKeyDown('Q'))
			moveDir.z += 1.0f;
		if (g_theInput->IsKeyDown('E'))
			moveDir.z -= 1.0f;
	}
	
	if (moveDir.GetLengthSquared() > 0.0f)
	{
		MoveInDirection(moveDir, deltaSeconds);
	}
}

void Player::UpdateFromController(float deltaSeconds)
{
	XboxController const& controller = g_theInput->GetController(0);
	if (controller.IsButtonDown(XboxButtonID::LS) || controller.IsButtonDown(XboxButtonID::RS))
	{
		m_isRunning = true;
		m_maxSpeed = m_runSpeed;
		m_flySpeed = m_runSpeed * 2.0f;
	}
	
	Vec2 leftStick = controller.GetLeftStick().GetPosition();
	if (leftStick.GetLengthSquared() > 0.01f)
	{
		Vec3 forward = GetForwardVector();
		Vec3 left = GetLeftVector();
		
		// For walking mode, flatten to XY
		if (m_physicsMode == PhysicsMode::WALKING)
		{
			forward.z = 0.0f;
			if (forward.GetLengthSquared() > 0.0f)
				forward = forward.GetNormalized();
			
			left.z = 0.0f;
			if (left.GetLengthSquared() > 0.0f)
				left = left.GetNormalized();
		}
		
		Vec3 moveDir = forward * leftStick.y + left * (-leftStick.x);
		MoveInDirection(moveDir, deltaSeconds);
	}
	
	// Triggers for vertical movement (flying/noclip only)
	if (m_physicsMode == PhysicsMode::FLYING || m_physicsMode == PhysicsMode::NOCLIP)
	{
		float verticalInput = controller.GetLeftTrigger() - controller.GetRightTrigger();
		if (fabsf(verticalInput) > 0.01f)
		{
			Vec3 moveDir(0.0f, 0.0f, verticalInput);
			MoveInDirection(moveDir, deltaSeconds);
		}
	}
}

void Player::CyclePhysicsMode()
{
	int currentMode = (int)m_physicsMode;
	currentMode = (currentMode + 1) % 3;
	m_physicsMode = (PhysicsMode)currentMode;
	
	// Print mode to console
	const char* modeNames[] = { "WALKING", "FLYING", "NOCLIP" };
	//DebuggerPrintf("Physics Mode: %s\n", modeNames[currentMode]);
	m_playerModeString = Stringf("Physics Mode: %s\n", modeNames[currentMode]);
}

void Player::CycleCameraMode()
{
	if (m_gameCamera)
	{
		m_gameCamera->CycleCameraMode();
	}
}

void Player::CollectNearbyDrops()
{
	// if (!m_world || !m_hotbarSystem)
	// 	return;
	//    
	// CropDropManager* dropMgr = m_world->GetCropDropManager();
	// if (!dropMgr)
	// 	return;
	//    
	// // 1. 收集附近掉落物
	// Vec3 playerPos = m_position;
	// std::vector<PendingDrop> collected = dropMgr->CollectDropsNear(playerPos, 2.0f);
	//    
	// // 2. 添加到快捷栏
	// for (const PendingDrop& drop : collected)
	// {
	// 	m_hotbarSystem->AddItem(drop.m_itemType, drop.m_count);
	// }
	//    
	// // 3. 刷新UI
	// if (!collected.empty())
	// {
	// 	g_theGameUIManager->GetHUD()->RefreshHotbarDisplay();
	// }

}

Direction Player::GetOrthoDirection()
{
	Vec3 forward = GetForwardVector();
	float absX = fabsf(forward.x);
	float absY = fabsf(forward.y);
	float absZ = fabsf(forward.z);
    
	if (absZ > absX && absZ > absY)
	{
		// 上下方向占主导
		return (forward.z >= 0.0f) ? DIRECTION_UP : DIRECTION_DOWN;
	}
	else if (absX > absY)
	{
		return (forward.x >= 0.0f) ? DIRECTION_EAST : DIRECTION_WEST;
	}
	else
	{
		return (forward.y >= 0.0f) ? DIRECTION_NORTH : DIRECTION_SOUTH;
	}
}

Direction Player::GetHorizontalDirection()
{
	Vec3 forward = GetForwardVector();

	forward.z = 0.0f;
	if (forward.GetLengthSquared() < 1e-6f)
	{
		return DIRECTION_NORTH;
	}

	forward = forward.GetNormalized();
	float absX = fabsf(forward.x);
	float absY = fabsf(forward.y);
	if (absX > absY)
	{
		return (forward.x >= 0.0f) ? DIRECTION_EAST : DIRECTION_WEST;
	}
	else
	{
		return (forward.y >= 0.0f) ? DIRECTION_NORTH : DIRECTION_SOUTH;
	}
}

Direction Player::GetOrthoDirectionOpposite()
{
	Direction ortho = GetOrthoDirection();
	switch (ortho)
	{
		case DIRECTION_NORTH:
		return DIRECTION_SOUTH;
		case DIRECTION_SOUTH:
		return DIRECTION_NORTH;
		case DIRECTION_EAST:
		return DIRECTION_WEST;
		case DIRECTION_WEST:
		return DIRECTION_EAST;
	case DIRECTION_UP:
		return DIRECTION_DOWN;
		default:
		return DIRECTION_UP;
	}
}

void Player::SelectHotbarSlot(int slot)
{
	if (m_inventory)
	{
		m_inventory->SetSelectedHotbarSlot(slot);
        
		if (g_theGameUIManager)
		{
			HUD* hud = g_theGameUIManager->GetHUD();
			if (hud)
				hud->UpdateSelectionFrame();
		}
	}
}

int Player::GetSelectedHotbarSlot() const
{
	return m_inventory ? m_inventory->GetSelectedHotbarSlot() : 0;
}

Item const& Player::GetSelectedItem() const
{
	static Item empty;
	return m_inventory ? m_inventory->GetSelectedItem() : empty;
}

void Player::HandleHotbarInput()
{
	if (!m_inventory)
		return;
	if (g_theInput->WasKeyJustPressed('X'))
	{
		m_inventory->SwitchHotbarRow();
        
		if (g_theGameUIManager)
		{
			HUD* hud = g_theGameUIManager->GetHUD();
			if (hud)
			{
				hud->RefreshHotbarDisplay();
				hud->UpdateSelectionFrame();
			}
		}
		return;
	}
	for (int i = 0; i < SLOTS_PER_ROW; i++)
	{
		if (g_theInput->WasKeyJustPressed('1' + i))
		{
			SelectHotbarSlot(i);
			return;
		}
	}
	float wheel = g_theInput->GetMouseWheelDelta();
	if (wheel != 0.0f)
	{
		int current = GetSelectedHotbarSlot();
		int newSlot = (wheel > 0) ? (current + 1) % SLOTS_PER_ROW : (current - 1 + SLOTS_PER_ROW) % SLOTS_PER_ROW;
		SelectHotbarSlot(newSlot);
	}
}


