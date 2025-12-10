

#include <plugin.h>
#include <injector/injector.hpp>
#include <windows.h>
#include <CControllerConfigManager.h>
#include <CHud.h>
#include <CPad.h>
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

int32_t* aPickUpsCollected = (int32_t*)0x978628;
uint8_t* menuID = (uint8_t*)0xBA68A5;
bool* isPlayerInMenu = (bool*)0xBA67A4;

bool pickUpPickedUp = 0;
bool pickUpJustPickedUp = 0;


// -------------------------------------------------------------------
//  IsPickUpPickedUp Replacement Function
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




class PickupHook {


public:
	size_t m_frame1 = 0;
	size_t m_frame2 = -1;
	bool lastIsSubJustPressed = 0;
	int subKey1;
	int subKey2;
	int subMouse;
	PickupHook() {

		INIReader reader("save0m0prac.ini");
		subKey1 = reader.GetInteger("binds", "subkey1", 50);
		subKey2 = reader.GetInteger("binds", "subkey2", -1);
		subMouse = reader.GetInteger("binds", "submouse", -1);


		Events::gameProcessEvent += [] { g_PickupHook.OnGameProcess(); };
		// Console for debugging
		//AllocConsole();
		//freopen("CONOUT$", "w", stdout);
		//printf("[ASI] Hook init\n");

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

	bool IsSubPressed() const {
		return ControlsManager.GetIsKeyboardKeyDown((RsKeyCodes)subKey1)
			|| ControlsManager.GetIsKeyboardKeyDown((RsKeyCodes)subKey2)
			|| GetIsMouseButtonDown();
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





		bool isSubJustDePressed = !IsSubPressed() && lastIsSubJustPressed;

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
				bool playerInVehicle = FindPlayerPed()->bInVehicle;
				sprintf_s(msg, "YAY, in vehicle: %d", playerInVehicle);
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
			// CMessages::ClearMessages(false);
			// CMessages::AddMessageJumpQ(msg, 2000, 0);
			CHud::SetHelpMessage(msg, true, false, false);
			*msg = 0;
		}

		m_frame1++;
		if (m_frame2 != -1) m_frame2++;
		lastIsSubJustPressed = IsSubPressed();
	}
} g_PickupHook;
