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
static constexpr std::uint32_t PENMASK = 0x1C300Bu; // mask_shot_hull | contents_hitbox?

void F::RAGEBOT::AIM::OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn)
{
	// Check if the legitbot is enabled
	if (!C_GET(bool, Vars.bRagebot))
		return;

	if (!pLocalController->IsPawnAlive())
		return;

	SilentAim(pCmd, pBaseCmd, pLocalController, pLocalPawn);
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

bool ShouldAutoshoot(CBaseUserCmdPB* pCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pEnemy)
{
	// Calculate the distance to the target
	Vector_t vecTarget = pEnemy->GetEyePosition();
	float flDistance = GetAngularDistance(pCmd, vecTarget, pLocalPawn);

	// Check if the target is within the autoshoot range
	if (flDistance < 500.0f && pEnemy && pLocalPawn->IsOtherEnemy(pEnemy))
	{
		// Check if the target is behind a wall
		if (IsWallbang(vecTarget, pLocalPawn))
		{
			// Calculate the adjustment angle based on the wallbang detection
			QAngle_t vAdjustment = CalculateWallbangAdjustment(vecTarget, pLocalPawn);

			// Adjust the shot accordingly
			pCmd->pViewAngles->angValue += vAdjustment;
		}

		return true;
	}

	return false;
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

QAngle_t GetAngularDifference(CBaseUserCmdPB* pCmd, const Vector_t& vecTarget, C_CSPlayerPawn* pLocal)
{
	Vector_t vecCurrent = pLocal->GetEyePosition();
	QAngle_t vNewAngle = (vecTarget - vecCurrent).ToAngles();
	vNewAngle.Normalize();

	QAngle_t vCurAngle = pCmd->pViewAngles->angValue;
	vNewAngle -= vCurAngle;

	return vNewAngle;
}

float GetAngularDistance(CBaseUserCmdPB* pCmd, const Vector_t& vecTarget, C_CSPlayerPawn* pLocal)
{
	return GetAngularDifference(pCmd, vecTarget, pLocal).Length2D();
}


QAngle_t CalculateWallbangAdjustment(const Vector_t& vecTarget, C_CSPlayerPawn* pLocalPawn)

{
	// Calculate the adjustment angle based on the wallbang detection

	// This will depend on your specific implementation and the game's mechanics

	return QAngle_t{ 0.5f, 0.5f, 0.0f }; // example adjustment angle
}

bool IsWallbang(const Vector_t& vecTarget, C_CSPlayerPawn* pLocalPawn)
{
	Ray_t ray;
	ray.m_vecStart = pLocalPawn->GetEyePosition();
	ray.m_vecEnd = vecTarget;

	TraceFilter_t filter(MASK_SHOT_HULL, pLocalPawn, nullptr, 0);

	GameTrace_t trace;

	if (pGameTraceManager->ClipRayToEntity(&ray, ray.m_vecStart, ray.m_vecEnd, pLocalPawn, &filter, &trace))
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

bool ShouldAutoshoot(CBaseUserCmdPB* pCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn)

{
	// Calculate the distance to the target

	float flDistance = GetAngularDistance(pCmd, vecBestPosition, pLocalPawn);

	// Check if the target is within the autoshoot range

	if (flDistance < 500.0f && pTarget && pLocalPawn->IsOtherEnemy(pTarget))

	{
		return true;
	}

	return false;
}

