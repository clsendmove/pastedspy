#include "Networking.h"
#include "../../SDK/Definitions/Interfaces/demo.h"


void CNetworking::CL_Sendmove()
{
	std::byte data[4000];
	const int Chocked = I::ClientState->m_NetChannel->m_nChokedPackets;
	const int NextCommandNumber = I::ClientState->lastoutgoingcommand + Chocked + 1;

	CLC_Move Message;
	Message.m_DataOut.StartWriting(data, sizeof(data));

	Message.m_nNewCommands = 1 + Chocked;
	Message.m_nNewCommands = std::clamp(Message.m_nNewCommands, 0, MAX_NEW_COMMANDS);
	const int ExtraCommands = (Chocked + 1) - Message.m_nNewCommands;
	const int BackupCommands = std::max(2, ExtraCommands);
	Message.m_nBackupCommands = std::clamp(BackupCommands, 0, MAX_BACKUP_COMMANDS);

	const int numcmds = Message.m_nNewCommands + Message.m_nBackupCommands;

	int from = -1;
	bool bOK = true;
	for (int to = NextCommandNumber - numcmds + 1; to <= NextCommandNumber; to++) {
		const bool isnewcmd = to >= NextCommandNumber - Message.m_nNewCommands + 1;
		bOK = bOK && I::BaseClientDLL->WriteUsercmdDeltaToBuffer(&Message.m_DataOut, from, to, isnewcmd);
		from = to;
	}

	if (bOK) {
		if (ExtraCommands)
			I::ClientState->m_NetChannel->m_nChokedPackets -= ExtraCommands;

		U::Memory.GetVFUNC<bool(__thiscall*)(void*, INetMessage*, bool, bool)>(I::ClientState->m_NetChannel, 37)
			(I::ClientState->m_NetChannel, &Message, false, false);
	}
}

MAKE_SIGNATURE(Net_Time, "engine.dll", "F2 0F 10 0D ? ? ? ? F2 0F 5C 0D", 0x0);
MAKE_SIGNATURE(Host_Framerate_unbounded, "engine.dll", "F3 0F 10 05 ? ? ? ? F3 0F 11 45 ? F3 0F 11 4D ? 89 45", 0x0);
MAKE_SIGNATURE(Host_Framerate_stddeviation, "engine.dll", "F3 0F 10 0D ? ? ? ? 48 8D 54 24 ? 0F 57 C0 48 89 44 24 ? 8B 05", 0x0);


void CNetworking::CL_Move(float AccumulatedExtraSamples, bool FinalTick) {
	auto host_limitlocal = I::CVar->FindVar("host_limitlocal");
	auto cl_cmdrate = I::CVar->FindVar("cl_cmdrate");

	if (!I::ClientState->IsConnected())
	{
		return;
	}

	bSendPacket = true;

	double NetTime = *reinterpret_cast<double*>(U::Memory.RelToAbsCustom(S::Net_Time(), 0x4));

	if (I::DemoPlayer->IsPlayingBack())
	{
		if (I::ClientState->ishltv || I::ClientState->isreplay) {
			bSendPacket = false;
		}

		else {
			return;
		}
	}

	if ((!I::ClientState->m_NetChannel->IsLoopback()) &&
		((NetTime < I::ClientState->m_flNextCmdTime) || !I::ClientState->m_NetChannel->CanPacket() || !FinalTick)) {
		bSendPacket = false;
	}

	if (I::ClientState->IsActive()) {
		int nextcommandnr = GetLatestCommandNumber();

		// Have client .dll create and store usercmd structure
		I::BaseClientDLL->CreateMove(nextcommandnr, I::GlobalVars->interval_per_tick - AccumulatedExtraSamples,
			!I::ClientState->IsPaused());

		// Store new usercmd to dem file
		if (I::DemoRecorder->IsRecording()) {
			// Back up one because we've incremented outgoing_sequence each frame by 1 unit
			I::DemoRecorder->RecordUserInput(nextcommandnr);
		}

		if (bSendPacket) {
			CL_Sendmove();
		}
		else {
			// netchanll will increase internal outgoing sequnce number too
			I::ClientState->m_NetChannel->SetChoked();
			// Mark command as held back so we'll send it next time
			I::ClientState->chokedcommands++;
		}

	}
	if (!bSendPacket)
		return;

	// Request non delta compression if high packet loss, show warning message
	bool HasProblem = I::ClientState->m_NetChannel->IsTimingOut() && !I::DemoPlayer->IsPlayingBack() && I::ClientState->IsActive();
	if (HasProblem) {
		// ts is where the game displays that timeout thing.
		// todo: do the timeout display thing
		I::ClientState->m_nDeltaTick = -1;
	}

	float unbounded = *reinterpret_cast<float*>(U::Memory.RelToAbsCustom(S::Host_Framerate_unbounded(), 0x4));
	float stddeviation = *reinterpret_cast<float*>(U::Memory.RelToAbsCustom(S::Host_Framerate_stddeviation(), 0x4));

	if (I::ClientState->IsActive()) {
		NET_Tick mymsg(I::ClientState->m_nDeltaTick, unbounded, stddeviation);
		I::ClientState->m_NetChannel->SendNetMsg(mymsg);
	}

	I::ClientState->lastoutgoingcommand = I::ClientState->m_NetChannel->SendDatagram(NULL);
	I::ClientState->chokedcommands = 0;

	if (I::ClientState->IsActive()) {
		// use full update rate when active
		float commandInterval = 1.0f / cl_cmdrate->GetFloat();
		float maxDelta = std::min(I::GlobalVars->interval_per_tick, commandInterval);
		float delta = std::clamp((float)(NetTime - I::ClientState->m_flNextCmdTime), 0.0f, maxDelta);
		I::ClientState->m_flNextCmdTime = NetTime + commandInterval - delta;
	}
	else {
		// during signon process send only 5 packets/second
		I::ClientState->m_flNextCmdTime = NetTime + (1.0f / 5.0f);
	}
}

int CNetworking::GetLatestCommandNumber()
{
	return (I::ClientState->lastoutgoingcommand + I::ClientState->chokedcommands + 1);
}