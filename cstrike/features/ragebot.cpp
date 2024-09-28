#include "ragebot.h"

// used: movement callback
#include "ragebot/aim.h"

void F::RAGEBOT::OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn)
{
	AIM::OnMove(pCmd, pBaseCmd, pLocalController, pLocalPawn);
}
