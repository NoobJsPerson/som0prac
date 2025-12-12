

#include <plugin.h>
#include <injector/injector.hpp>
#include <windows.h>
#include <CControllerConfigManager.h>
#include <CHud.h>
#include <CPad.h>
#include <CPickups.h>
#include <CMessages.h>
//#include <CTimer.h>
#include "extensions/ScriptCommands.h"
#include "INIReader.h"

using namespace plugin;

struct tPickupReference {
	union {
		struct {
			int16_t index;
			int16_t refIndex;
		};

		int32_t num;
	};
};

//int32_t* aPickUpsCollected = (int32_t*)0x978628;
uint8_t* menuID = (uint8_t*)0xBA68A5;
bool* isPlayerInMenu = (bool*)0xBA67A4;

bool pickUpPickedUp = 0;
bool pickUpJustPickedUp = 0;
int pickUpId = -1;
//unsigned int saveTimeStamp = -1;


// -------------------------------------------------------------------
//  IsPickUpPickedUp Replacement Function
// -------------------------------------------------------------------
bool __cdecl My_IsPickUpPickedUp(tPickupReference ref)
{
	int id = ref.num;


	int index = -1;

	for (int i = 0; i < 20; i++) {
		if (CPickups::aPickUpsCollected[i] == id) {
			CPickups::aPickUpsCollected[i] = 0;
			pickUpId = ref.index;
			index = id;
			pickUpJustPickedUp = 1;
			//if (saveTimeStamp != -1) saveTimeStamp = CTimer::m_snTimeInMilliseconds;
			break;
		}
	}

	bool picked = index != -1;

	return picked;
}

auto IsCharInAnyPoliceVehicle(CPed& ped) {
	if (!(ped.bInVehicle && ped.m_pVehicle)) {
		return false;
	}
	const auto veh = ped.m_pVehicle;
	return veh->IsLawEnforcementVehicle() && veh->m_nModelIndex != eModelID::MODEL_PREDATOR;
}

class PickupHook {


public:
	int m_frame1 = 0;
	int m_frame2 = -1;
	int m_frameTest = 0;
	bool lastIsSubKeyPressed = 0;
	bool lastIsSubMousePressed = 0;
	bool lastSubWasMouse = 0;
	bool lastPlayerInVehicle = 0;
	int subKey1;
	int subKey2;
	int subMouse;
	//bool playerInVehicle = 0;
	//bool nextPlayerInVehicle = 0;

	PickupHook() {

		INIReader reader("save0m0prac.ini");
		subKey1 = reader.GetInteger("binds", "subkey1", 50);
		subKey2 = reader.GetInteger("binds", "subkey2", -1);
		subMouse = reader.GetInteger("binds", "submouse", -1);


		Events::gameProcessEvent += [] { g_PickupHook.OnGameProcess(); };
		//Console for debugging
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		printf("[ASI] Hook init\n");

		// Replace full function at 0x454B40
		injector::MakeJMP(0x454B40, My_IsPickUpPickedUp);

		//printf("[ASI] Hook installed at 0x454B40\n");
	}

	bool GetIsMouseButtonDown() const {
		switch (subMouse) {
		case VK_LBUTTON:
			return CPad::NewMouseControllerState.lmb;
		case VK_RBUTTON:
			return CPad::NewMouseControllerState.rmb;
		case VK_MBUTTON:
			return CPad::NewMouseControllerState.mmb;
		case VK_XBUTTON1:
			return CPad::NewMouseControllerState.bmx1;
		case VK_XBUTTON2:
			return CPad::NewMouseControllerState.bmx2;
		}
		return false;
	}

	bool IsSubKeyPressed() const {
		return ControlsManager.GetIsKeyboardKeyDown((RsKeyCodes)subKey1)
			|| ControlsManager.GetIsKeyboardKeyDown((RsKeyCodes)subKey2);
	}

	void OnGameProcess()
	{
		//unsigned int cycleTime = (CTimer::m_snTimeInMilliseconds - saveTimeStamp) % 500;
		if (pickUpJustPickedUp) {

			m_frame1 = 0;
			pickUpPickedUp = 1;
			//pickUpJustPickedUp = 0;
		}

		

		
		//CMessages::ClearMessages(false);
		//CMessages::ClearAllMessagesDisplayedByGame();
		//bool pickUpInvisible = CPickups::aPickUps[pickUpId].m_pObject == nullptr;




		bool subKeyPressed = IsSubKeyPressed();
		bool mouseButtonDown = GetIsMouseButtonDown();

		bool isSubJustDePressed = (!subKeyPressed && !mouseButtonDown) && (lastIsSubKeyPressed || lastIsSubMousePressed);

		CPed* playa;
		Command<Commands::GET_PLAYER_CHAR>(0, &playa);

		if (Command<Commands::IS_CHAR_IN_ANY_POLICE_VEHICLE>(playa) && !lastPlayerInVehicle) {
			m_frameTest = 0;
		}

		/*if (mouseButtonDown && !subKeyPressed) {
			playerInVehicle = nextPlayerInVehicle;
			nextPlayerInVehicle = Command<Commands::IS_CHAR_IN_ANY_POLICE_VEHICLE>(playa);
		}

		if (subKeyPressed && !mouseButtonDown) {
			playerInVehicle = Command<Commands::IS_CHAR_IN_ANY_POLICE_VEHICLE>(playa);
		}*/

		if (isSubJustDePressed) {
			lastSubWasMouse = lastIsSubMousePressed && !lastIsSubKeyPressed;
			/*if (lastSubWasMouse) {
				playerInVehicle = Command<Commands::IS_CHAR_IN_ANY_POLICE_VEHICLE>(playa);
			}*/
			//else {
			//	lastSubWasMouse = 0;
			//}
			m_frame2 = 0;
		}

		if (*isPlayerInMenu && (*menuID == 16)) {
			m_frame1 = 0;
			m_frame2 = -1;
		}


		static char msg[255] = { 0 };

		// Pressed Early Check
		if (!isSubJustDePressed && pickUpPickedUp && m_frame2 != -1) {
			static char notInVehMsg[255];
			if (m_frame2 == (2 -  lastSubWasMouse)) {
				sprintf_s(notInVehMsg, "You didn't get on the vehicle fast enough, got in %d frame(s) before pickup (minimum 3)", m_frameTest);
				sprintf_s(msg, m_frameTest >= 3 ? "Reminder to not do this with keyboard" : notInVehMsg);
			}
			else if (m_frame2 < (2 -  lastSubWasMouse)) {
				sprintf_s(notInVehMsg, " and you didn't get on the vehicle fast enough, got in %d frame(s) before pickup (minimum 3)", m_frameTest);
				sprintf_s(msg, "You were %d frame(s) too late%s", (2 -  lastSubWasMouse) - m_frame2, m_frameTest >= 3 ? "" : notInVehMsg);
			}
			else {
				sprintf_s(notInVehMsg, " and you didn't get on the vehicle fast enough, got in %d frame(s) before pickup (minimum 3)", m_frameTest);
				sprintf_s(msg, "You were %d frame(s) too early%s", m_frame2 - (2 -  lastSubWasMouse), m_frameTest >= 3 ? "" : notInVehMsg);
			}
			m_frame2 = -1;
			pickUpPickedUp = 0;

		}

		if (pickUpJustPickedUp) {
			m_frameTest = 0;
			pickUpJustPickedUp = 0;
		}



		// Pressed Late Check
		if (isSubJustDePressed && pickUpPickedUp) {
			sprintf_s(msg, "You were %d frame(s) too late", m_frame1 + (2 -  lastSubWasMouse));
			pickUpPickedUp = 0;

		}

		if (*msg != 0) {
			CMessages::ClearMessages(false);
			CHud::SetHelpMessage(msg, false, false, false);
			printf("%s\n", msg);
			*msg = 0;
		}

		m_frame1++;
		m_frameTest++;
		if (m_frame2 != -1) m_frame2++;
		lastIsSubKeyPressed = IsSubKeyPressed();
		lastIsSubMousePressed = GetIsMouseButtonDown();
		lastPlayerInVehicle = Command<Commands::IS_CHAR_IN_ANY_POLICE_VEHICLE>(playa);
	}
} g_PickupHook;
