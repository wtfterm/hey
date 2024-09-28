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
	void Autowall(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn);
	C_CSPlayerPawn* GetTargetEntity(C_CSPlayerPawn* pLocalPawn, CCSPlayerController* pLocalController);

}
