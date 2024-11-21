//99% of this code is skidded
#include "aim.h"

// used: sdk entity
#include "../../sdk/entity.h"
#include "../../sdk/interfaces/cgameentitysystem.h"
#include "../../sdk/interfaces/iengineclient.h"
// used: cusercmd
#include "../../sdk/datatypes/usercmd.h"
#include "../../sdk/datatypes/vector.h"

// used: activation button
#include "../../utilities/inputsystem.h"

// used: cheat variables
#include "../../core/variables.h"

#include "../../sdk/interfaces/cgametracemanager.h"
#include "../../core/sdk.h"
#include "../../sdk/interfaces/iglobalvars.h"
#include "../../sdk/datatypes/qangle.h"
#include "../../sdk/datatypes/vector.h"
#include "../../sdk/entity_handle.h"
#include "../cstrike/sdk/interfaces/ienginecvar.h"
#include <mutex>
#include <array>
#include "../cstrike/sdk/interfaces/itrace.h"
#include "../../sdk/interfaces/ccsgoinput.h"

static constexpr std::uint32_t PENMASK = 0x1C300Bu; // mask_shot_hull | contents_hitbox?

void F::RAGEBOT::AIM::OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn)
{
	// Check if the legitbot is enabled
	if (!C_GET(bool, Vars.bRagebot))
		return;

	if (!pLocalController->IsPawnAlive())
		return;

	if (ShouldAutoshoot(pBaseCmd, pLocalController, pLocalPawn))

	{
		// Autoshoot logic

		pCmd->nButtons.nValue |= (1 << 0);
	}

	if (ShouldAutowall(pBaseCmd, pLocalController, pLocalPawn))

	{
		C_CSPlayerPawn* pTarget = F::RAGEBOT::AIM::GetTargetEntity(pLocalPawn, pLocalController);

		Vector_t vecTargetPosition = pTarget->GetEyePosition();

		// Calculate the point of impact on the wall

		Vector_t vecImpact = GetWallbangPosition(pLocalPawn, vecTargetPosition);

		// Aim at the point of impact

		AimAtPosition(pBaseCmd, pLocalController, pLocalPawn, vecImpact);

		// Simulate a mouse click

		pCmd->nButtons.nValue |= (1 << 0);
	}

	SilentAim(pCmd, pBaseCmd, pLocalController, pLocalPawn);
}

void F::RAGEBOT::AIM::AimAtPosition(CBaseUserCmdPB* pCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn, Vector_t vecTargetPosition)
{
	// Calculate the direction from the player's eye position to the target position
	Vector_t vecDirection = vecTargetPosition - pLocalPawn->GetEyePosition();
	float length = sqrt(vecDirection.x * vecDirection.x + vecDirection.y * vecDirection.y + vecDirection.z * vecDirection.z);
	if (length > 0.0f)
	{
		vecDirection.x /= length;
		vecDirection.y /= length;
		vecDirection.z /= length;
	}

	// Calculate the pitch and yaw angles
	float flPitch = -atan2f(vecDirection.z, sqrt(vecDirection.x * vecDirection.x + vecDirection.y * vecDirection.y));
	float flYaw = atan2f(vecDirection.y, vecDirection.x);

	// Get the CCSGOInput object
	CCSGOInput* pInput = CCSGOInput::GetCCSGOInput();

	// Set the view angles using the CCSGOInput object
	QAngle_t angView;
	angView.x = flPitch;
	angView.y = flYaw;
	pInput->SetViewAngle(angView);
}

CGameTraceManager g_GameTraceManager;


Vector_t F::RAGEBOT::AIM::GetWallbangPosition(C_CSPlayerPawn* pLocalPawn, Vector_t vecTargetPosition)

{
	// Create a ray from the player's eye position to the target's eye position

	Ray_t ray;

	ray.m_vecStart = pLocalPawn->GetEyePosition();

	ray.m_vecEnd = vecTargetPosition;

	// Create a filter to ignore the target entity

	TraceFilter_t filter(MASK_SHOT_HULL, pLocalPawn, nullptr, 0);

	// Create a game trace to store the result

	GameTrace_t trace;

	// Perform a trace to find the point of impact on the wall

	if (g_GameTraceManager.TraceShape(&ray, ray.m_vecStart, ray.m_vecEnd, &filter, &trace))

	{
		// Calculate the point of impact on the wall

		Vector_t vecImpact = ray.m_vecStart + (ray.m_vecEnd - ray.m_vecStart) * trace.m_flFraction;

		return vecImpact;
	}

	return vecTargetPosition;
}

C_CSPlayerPawn* F::RAGEBOT::AIM::GetTargetEntity(C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController)
{
	// The current best distance
	float flDistance = INFINITY;
	// The target we have chosen
	C_CSPlayerPawn* pTarget = nullptr;

	// Entity loop
	const int iHighestIndex = I::GameResourceService->pGameEntitySystem->GetHighestEntityIndex();

	for (int nIndex = 1; nIndex <= iHighestIndex; nIndex++)
	{
		// Get the entity
		C_BaseEntity* pEntity = I::GameResourceService->pGameEntitySystem->Get(nIndex);
		if (pEntity == nullptr)
			continue;

		// Get the class info
		SchemaClassInfoData_t* pClassInfo = nullptr;
		pEntity->GetSchemaClassInfo(&pClassInfo);
		if (pClassInfo == nullptr)
			continue;

		// Get the hashed name
		const FNV1A_t uHashedName = FNV1A::Hash(pClassInfo->szName);

		// Make sure they're a player controller
		if (uHashedName != FNV1A::HashConst("CCSPlayerController"))
			continue;

		// Cast to player controller
		CCSPlayerController* pPlayer = reinterpret_cast<CCSPlayerController*>(pEntity);
		if (pPlayer == nullptr)
			continue;

		// Check the entity is not us
		if (pPlayer == pLocalController)
			continue;

		// Get the player pawn
		C_CSPlayerPawn* pPawn = I::GameResourceService->pGameEntitySystem->Get<C_CSPlayerPawn>(pPlayer->GetPawnHandle());
		if (pPawn == nullptr)
			continue;

		// Make sure they're alive
		if (!pPlayer->IsPawnAlive())
			continue;

		// Check if they're an enemy
		if (!pLocalPawn->IsOtherEnemy(pPawn))
			continue;

		// Get the position
		CSkeletonInstance* pSkeleton = pPawn->GetGameSceneNode()->GetSkeletonInstance();
		if (pSkeleton == nullptr)
			continue;
		Matrix2x4_t* pBoneCache = pSkeleton->pBoneCache;
		if (pBoneCache == nullptr)
			continue;

		const int iBone = 6; // You may wish to change this dynamically but for now let's target the head.

		// Get the bone's position
		Vector_t vecPos = pBoneCache->GetOrigin(iBone);

		// Get the distance/weight of the move
		float flCurrentDistance = GetAngularDistance(nullptr, vecPos, pLocalPawn);
		if (flCurrentDistance > C_GET(float, Vars.flAimRange)) // Skip if this move out of aim range
			continue;
		if (pTarget && flCurrentDistance > flDistance) // Override if this is the first move or if it is a better move
			continue;

		// Better move found, override.
		pTarget = pPawn;
		flDistance = flCurrentDistance;
	}

	return pTarget;
}


void F::RAGEBOT::AIM::SilentAim(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn)
{
	C_CSPlayerPawn* pEnemy = GetTargetEntity(pLocalPawn, pLocalController);
	// Check if the legitbot is enabled
	if (!C_GET(bool, Vars.bRagebot))
		return;

	if (!pLocalController->IsPawnAlive())
		return;

	// Get the target's position
	Vector_t vecTarget = pEnemy->GetEyePosition();

	// Check if the target is behind a wall
	if (IsWallbang(vecTarget, pLocalPawn))
	{
		// Calculate the adjustment angle based on the wallbang detection
		QAngle_t vAdjustment = CalculateWallbangAdjustment(vecTarget, pLocalPawn);

		// Adjust the shot accordingly
		pBaseCmd->pViewAngles->angValue += vAdjustment;
	}

	// Perform the silent aim
	// ...
}


QAngle_t GetRecoil(CBaseUserCmdPB* pCmd, C_CSPlayerPawn* pLocal)
{
	static QAngle_t OldPunch; //get last tick AimPunch angles
	if (pLocal->GetShotsFired() >= 1) //only update aimpunch while shooting
	{
		QAngle_t viewAngles = pCmd->pViewAngles->angValue;
		QAngle_t delta = viewAngles - (viewAngles + (OldPunch - (pLocal->GetAimPuchAngle() * 2.f))); //get current AimPunch angles delta

		return pLocal->GetAimPuchAngle() * 2.0f; //return correct aimpunch delta
	}
	else
	{
		return QAngle_t{ 0, 0, 0 }; //return 0 if is not shooting
	}
}

QAngle_t F::RAGEBOT::AIM::GetAngularDifference(CBaseUserCmdPB* pCmd, const Vector_t& vecTarget, C_CSPlayerPawn* pLocal)
{
	Vector_t vecCurrent = pLocal->GetEyePosition();
	QAngle_t vNewAngle = (vecTarget - vecCurrent).ToAngles();
	vNewAngle.Normalize();

	QAngle_t vCurAngle = pCmd->pViewAngles->angValue;
	vNewAngle -= vCurAngle;

	return vNewAngle;
}

float F::RAGEBOT::AIM::GetAngularDistance(CBaseUserCmdPB* pCmd, const Vector_t& vecTarget, C_CSPlayerPawn* pLocal)
{
	return GetAngularDifference(pCmd, vecTarget, pLocal).Length2D();
}


QAngle_t F::RAGEBOT::AIM::CalculateWallbangAdjustment(const Vector_t& vecTarget, C_CSPlayerPawn* pLocalPawn)

{
	// Calculate the adjustment angle based on the wallbang detection

	// This will depend on your specific implementation and the game's mechanics

	return QAngle_t{ 0.5f, 0.5f, 0.0f }; // example adjustment angle
}




bool F::RAGEBOT::AIM::IsWallbang(const Vector_t& vecTarget, C_CSPlayerPawn* pLocalPawn)
{
	Ray_t ray;
	ray.m_vecStart = pLocalPawn->GetEyePosition();
	ray.m_vecEnd = vecTarget;

	TraceFilter_t filter(MASK_SHOT_HULL, pLocalPawn, nullptr, 0);

	GameTrace_t trace;

	if (g_GameTraceManager.TraceShape(&ray, ray.m_vecStart, ray.m_vecEnd, &filter, &trace))
	{
		if (trace.m_flFraction < 1.0f && trace.m_uContents & MASK_SHOT_HULL)
		{
			// Calculate the point of impact on the wall
			Vector_t vecImpact = ray.m_vecStart + (ray.m_vecEnd - ray.m_vecStart) * trace.m_flFraction;
			// Adjust the shot accordingly (e.g., adjust the angle, reduce damage, etc.)
			return true;
		}
	}
	return false;
}




bool F::RAGEBOT::AIM::ShouldAutoshoot(CBaseUserCmdPB* pCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn)

{
	if (!C_GET(bool, Vars.bAutoShoot))
		return false;
	C_CSPlayerPawn* pTarget = F::RAGEBOT::AIM::GetTargetEntity(pLocalPawn, pLocalController);

	// Calculate the distance to the target

	float flDistance = GetAngularDistance(pCmd, pTarget->GetEyePosition(), pLocalPawn);

	// Check if the target is within the autoshoot range

	if (flDistance < 500.0f && pTarget && pLocalPawn->IsOtherEnemy(pTarget))

	{
		return true;
	}

	return false;
}



bool F::RAGEBOT::AIM::ShouldAutowall(CBaseUserCmdPB* pCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn)

{
	if (!C_GET(bool, Vars.bAutoWall))
		return false;
	C_CSPlayerPawn* pTarget = F::RAGEBOT::AIM::GetTargetEntity(pLocalPawn, pLocalController);

	// Calculate the distance to the target

	float flDistance = GetAngularDistance(pCmd, pTarget->GetEyePosition(), pLocalPawn);

	// Check if the target is within the autowall range

	if (flDistance < 1000.0f && pTarget && pLocalPawn->IsOtherEnemy(pTarget))

	{
		// Check if there's a wall between the player and the target

		if (IsWallBetween(pLocalPawn, pTarget))

		{
			return true;
		}
	}

	return false;
}

bool F::RAGEBOT::AIM::IsWallBetween(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTarget)

{
	// Create a ray from the player's eye position to the target's eye position

	Ray_t ray;

	ray.m_vecStart = pLocalPawn->GetEyePosition();

	ray.m_vecEnd = pTarget->GetEyePosition();

	// Create a filter to ignore the target entity

	TraceFilter_t filter(MASK_SHOT_HULL, pTarget, nullptr, 0);

	// Create a game trace to store the result

	GameTrace_t trace;

	// Perform a trace to check if there's a wall between the player and the target

	if (g_GameTraceManager.TraceShape(&ray, ray.m_vecStart, ray.m_vecEnd, &filter, &trace))

	{
		// Check if the trace hit a wall

		if (trace.m_flFraction < 1.0f && trace.m_uContents & MASK_SHOT_HULL)

		{
			return true;
		}
	}

	return false;
}
