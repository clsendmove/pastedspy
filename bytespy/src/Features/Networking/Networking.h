#pragma once
#include "../../SDK/SDK.h"

class CNetworking {
public:
	void CL_Move(float AccumulatedExtraSamples, bool FinalTick);
	bool bSendPacket = false;
	void CL_Sendmove();
	int GetLatestCommandNumber();
};

ADD_FEATURE(CNetworking, Networking);