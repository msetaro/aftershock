#include "../../engine/client/cl_checkpoint.cpp"
#include <cassert>

clientActive_t cl;
clientStatic_t cls;
static int menus;
void NativeUI_SetActiveMenu( int ) {
	++menus;
}

int main() {
	unsigned char bytes[8192];
	cls.state = CA_ACTIVE;
	cls.realtime = 10000;
	cl.snap.valid = qtrue;
	cl.serverTime = cl.oldServerTime = 4600;
	cl.oldFrameServerTime = 4550;
	cl.serverTimeDelta = -5350; // Paused clock is ahead of the last sent command.
	cl.extrapolatedSnapshot = cl.newSnapshots = qtrue;
	cl.viewangles[1] = 91.25f;
	cl.cgameUserCmdValue = 4;
	cl.cgameSensitivity = 0.75f;
	cl.cmdNumber = 5;
	cl.cmds[5].serverTime = 4600;
	cl.cmds[5].weapon = 4;
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(CL_WriteCheckpoint(&writer));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	cls.realtime = 90000;
	cl.serverTime = 7000;
	assert(CL_ReadCheckpoint(reader,false,false) && cl.serverTime==7000 && !menus);
	assert(CL_ReadCheckpoint(reader,true,true));
	assert(cl.serverTime==4600 && cl.oldServerTime==4600 && cl.oldFrameServerTime==4550);
	assert(cls.realtime+cl.serverTimeDelta==4650 && cl.extrapolatedSnapshot && cl.newSnapshots);
	assert(cl.viewangles[1]==91.25f && cl.cgameUserCmdValue==4 && cl.cgameSensitivity==0.75f);
	assert(cl.cmds[0].serverTime==4600 && cl.cmds[CMD_MASK].weapon==4 && menus==1);
	puts( "PASS: local checkpoint input clock rebases while preserving pause lead and pending command" );
}
