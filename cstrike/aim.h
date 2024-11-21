#pragma once
#include "../../common.h"
#include <memory>
// used: draw system
#include "../../utilities/draw.h"
#include "../../sdk/datatypes/vector.h"
#include "../../sdk/datatypes/transform.h"
#include "../../sdk/datatypes/qangle.h"
#include "../cstrike/core/config.h"
#include <array>
class CUserCmd;
class CBaseUserCmdPB;
class CCSGOInputHistoryEntryPB;

class CCSPlayerController;
class C_CSPlayerPawn;

struct QAngle_t;

namespace F::RAGEBOT::AIM
{
	void OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn);
	void SilentAim(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn);
	C_CSPlayerPawn* GetTargetEntity(C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController);
	void AimAtPosition(CBaseUserCmdPB* pCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn, Vector_t vecTargetPosition);
	bool IsWallBetween(C_CSPlayerPawn* pLocalPawn, C_CSPlayerPawn* pTarget);
	bool ShouldAutowall(CBaseUserCmdPB* pCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn);
	bool ShouldAutoshoot(CBaseUserCmdPB* pCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn);
	bool IsWallbang(const Vector_t& vecTarget, C_CSPlayerPawn* pLocalPawn);
	QAngle_t CalculateWallbangAdjustment(const Vector_t& vecTarget, C_CSPlayerPawn* pLocalPawn);
	float GetAngularDistance(CBaseUserCmdPB* pCmd, const Vector_t& vecTarget, C_CSPlayerPawn* pLocal);
	QAngle_t GetAngularDifference(CBaseUserCmdPB* pCmd, const Vector_t& vecTarget, C_CSPlayerPawn* pLocal);
	Vector_t GetWallbangPosition(C_CSPlayerPawn* pLocalPawn, Vector_t vecTargetPosition);
}
