

#include <plugin.h>
#include <CMessages.h>
#include <injector/injector.hpp>
#include <windows.h>
#include <cstdio>
#include <CPickup.h>
#include <CPickups.h>
#include <CControllerConfigManager.h>
#include <CHud.h>
#include "INIReader.h"

using namespace plugin;

// Structure passed to the function
struct tPickupReference {
	union {
		struct {
			int16_t index;
			int16_t refIndex;
		};

		int32_t num;
	};
};
//class PickupHook;
// Game memory (these are correct)
static int32_t* aPickUpsCollected = (int32_t*)0x978628;   // bool array[620]
static uint8_t* menuID = (uint8_t*)0xBA68A5;
static bool* isPlayerInMenu = (bool*)0xBA67A4;

bool pickUpPickedUp = 0;
bool pickUpJustPickedUp = 0;

//typedef uint32_t KeyCode;




//RsKeyCodes CControllerConfigManager::GetControllerKeyAssociatedWithAction(e_ControllerAction action, eControllerType type)
//{
//    return plugin::CallMethodAndReturn <RsKeyCodes, 0x52F4F0, CControllerConfigManager*, int>(this, action, type);
//}










//void CControllerConfigManager::SaveSettings(int file)
//{
//    plugin::CallMethod <0x52D200, CControllerConfigManager*, int>(this, file);
//}


// -------------------------------------------------------------------
//  CORRECT REPLACEMENT FUNCTION (MATCHES REAL GAME SIGNATURE)
// -------------------------------------------------------------------
bool __cdecl My_IsPickUpPickedUp(tPickupReference ref)
{
	int id = ref.num;
	

	int index = -1;

	for (int i = 0; i < 20; i++) {
		if (aPickUpsCollected[i] == id) {
			aPickUpsCollected[i] = 0;
			index = id;
			pickUpJustPickedUp = 1;
			break;
		}
	}

	bool picked = index != -1;
	
	return picked;
}

// -------------------------------------------------------------------
//  Automatically executed when ASI loads
// -------------------------------------------------------------------
class PickupHook {
	// RsKeyCodes submissionKey = ControlsManager.GetControllerKeyAssociatedWithAction(TOGGLE_SUBMISSIONS, CONTROLLER_PAD);


public:
	size_t m_frame1 = 0;
	size_t m_frame2 = -1;
	bool lastIsSubJustPressed = 0;
	int subKey;
	PickupHook() {

		INIReader reader("save0m0prac.ini");
		subKey = reader.GetInteger("binds", "subkey", 92);

		Events::gameProcessEvent += [] { g_PickupHook.OnGameProcess(); };
		// Console for debugging
		//AllocConsole();
		//freopen("CONOUT$", "w", stdout);
		//printf("[ASI] Hook init\n");

		// Replace full function at 0x454B40
		injector::MakeJMP(0x454B40, My_IsPickUpPickedUp);

		//printf("[ASI] Hook installed at 0x454B40\n");
	}
	void OnGameProcess()
	{
		bool lateEntered = 0;
		bool earlyEntered = 0;

		if (pickUpJustPickedUp) {
			m_frame1 = 0;
			pickUpPickedUp = 1;
			pickUpJustPickedUp = 0;
		}





		bool isSubJustDePressed = !ControlsManager.GetIsKeyboardKeyDown((RsKeyCodes)subKey) && lastIsSubJustPressed;

		if (isSubJustDePressed) {
			m_frame2 = 0;
		}

		if (*isPlayerInMenu && (*menuID == 16)) {
			m_frame1 = 0;
			m_frame2 = -1;
		}

		static char msg[255] = { 0 };

		// Pressed Early Check
		if (!isSubJustDePressed && pickUpPickedUp && m_frame2 != -1) {

			if (m_frame2 == 2) {
				strcpy(msg, "YAY");
			}
			else if (m_frame2 < 2) {
				sprintf_s(msg, "You were %d frame(s) too late", 2 - m_frame2);
			}
			else {
				sprintf_s(msg, "You were %d frame(s) too early", m_frame2 - 2);
			}
			m_frame2 = -1;
			pickUpPickedUp = 0;

		}

		// Pressed Late Check
		if (isSubJustDePressed && pickUpPickedUp) {
			sprintf_s(msg, "You were %d frame(s) too late", m_frame1 + 2);
			pickUpPickedUp = 0;

		}

		if (*msg != 0) {
			CMessages::ClearMessages(false);
			CMessages::AddMessageJumpQ(msg, 2000, 0);
			CHud::SetHelpMessage(msg, true, false, false);
			*msg = 0;
		}

		m_frame1++;
		if (m_frame2 != -1) m_frame2++;
		lastIsSubJustPressed = ControlsManager.GetIsKeyboardKeyDown((RsKeyCodes)subKey);
	}
} g_PickupHook;
