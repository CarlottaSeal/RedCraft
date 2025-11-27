#pragma once
#include <string>

#include "PhysicsUtils.h"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Renderer/Camera.hpp"

class Entity;
class World;

enum class GameCameraMode
{
	FIRST_PERSON,           // Camera at entity eye, oriented with entity
	OVER_SHOULDER,          // Camera behind entity, oriented with entity
	FIXED_ANGLE_TRACKING,   // Camera at fixed angle (yaw=40, pitch=30), 10m away, facing entity
	SPECTATOR,              // Free camera, controls move camera, player dispossessed
	SPECTATOR_XY,           // Free camera, XY movement only
	INDEPENDENT             // Player dispossessed, controls move player, camera static
};

class GameCamera : public Camera
{
public:
	GameCamera();
	~GameCamera();

	void Update(float deltaSeconds, Entity* entity, World* world);
	
	// Mode updates
	void UpdateFirstPerson(Entity* entity);
	void UpdateOverShoulder(Entity* entity, World* world);
	void UpdateFixedAngleTracking(Entity* entity);
	void UpdateSpectator(float deltaSeconds);
	void UpdateSpectatorXY(float deltaSeconds);
	void UpdateIndependent(Entity* entity);
	
	// Camera shake
	void AddCameraShake(float intensity, float duration);
	void UpdateCameraShake(float deltaSeconds);
	
	// Collision for attached cameras
	Vec3 ResolveThirdPersonCollision(Vec3 const& targetPos, Vec3 const& pivotPos, World* world);
	
	// Mode control
	void SetCameraMode(GameCameraMode mode);
	void CycleCameraMode();
	GameCameraMode GetCameraMode() const { return m_cameraMode; }
	
	// Getters
	Vec3 GetPosition() const { return m_position; }
	EulerAngles GetOrientation() const { return m_orientation; }

public:
	std::string m_cameraModeString;
	
private:
	GameCameraMode m_cameraMode = GameCameraMode::FIRST_PERSON;
	
	// Camera shake
	bool m_isCameraShaking = false;
	float m_shakeIntensity = 0.0f;
	float m_shakeDuration = 0.0f;
	float m_shakeTimer = 0.0f;
	Vec3 m_shakeOffset = Vec3();
	
	// Third person settings
	float m_overShoulderDistance = CAMERA_OVER_SHOULDER_DIST;
	Vec3 m_overShoulderOffset = Vec3(0.0f, 0.0f, 0.5f); // Pivot offset from entity position
	float m_minOverShoulderDistance = 0.5f;
	
	// Fixed angle tracking settings
	float m_fixedTrackingDistance = 10.0f;
	float m_fixedTrackingYaw = 40.0f;
	float m_fixedTrackingPitch = 30.0f;
	
	// Spectator settings
	float m_spectatorSpeed = 10.0f;
	float m_spectatorRunSpeed = 30.0f;
};