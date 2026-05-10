#include "Minimap.h"
#include <fstream>
#include <windows.h>
#include <string>
#include <ctime>
#include <iostream>
#include <vector>
#include "Hooking.h"
#include "Hooking.Patterns.h"
#include <MinHook.h>
#include "spdlog/spdlog.h"

namespace minimap
{
	// Radar Coords
	std::vector<float> LibertyCityGFXPos = { 5490.0f, -2697.0f, 0.0f };

	bool IsPlayerInsideInterior()
	{
		Ped PlayerPed = PLAYER::PLAYER_PED_ID();
		int InteriorID = INTERIOR::GET_INTERIOR_FROM_ENTITY(PlayerPed);

		if (InteriorID != 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	static void CreateMinimap()
	{
		// SET_MINIMAP_COMPONENT(14, ...) toggles the LC-specific minimap
		// component on. Has to run every frame because the game resets
		// component visibility constantly. Logging here would spam the
		// file at ~30+ Hz, so we only log the first transition into LC
		// and the first transition out.
		static bool wasInLC = false;
		const bool isInLC = worldtravel::IsLibertyCity();
		if (isInLC)
		{
			UI::SET_MINIMAP_COMPONENT(14, true, -1);
			if (!wasInLC)
			{
				spdlog::info("CreateMinimap: entered Liberty City, SET_MINIMAP_COMPONENT(14, true, -1) active");
				wasInLC = true;
			}
		}
		else if (wasInLC)
		{
			spdlog::info("CreateMinimap: left Liberty City, component-14 trigger inactive");
			wasInLC = false;
		}
	}

	// Reimplementation of the v_fakelibertycity trigger that was present in
	// the January 2025 binary release but stripped from the May 2025 source.
	// Recovered by disassembling the legacy WorldTravel.asi (Dec 2024 build)
	// at VMA 0x1800229e0. The original function was a thread-loop that, every
	// frame the player was in Liberty City, told the radar to render the
	// fake interior "v_fakelibertycity" centered on the LC GFX coords.
	// Without this call the radar (both live HUD minimap and pause map)
	// stays transparent in LC even after the LS-bitmap-suppression hooks
	// fire, because nothing tells the renderer to substitute the LC tiles.
	static void TriggerLibertyCityRadarInterior()
	{
		static bool loggedOnce = false;
		if (!worldtravel::IsLibertyCity()) return;

		UI::SET_RADAR_AS_EXTERIOR_THIS_FRAME();
		Hash interiorHash = GAMEPLAY::GET_HASH_KEY(const_cast<char*>("v_fakelibertycity"));
		UI::SET_RADAR_AS_INTERIOR_THIS_FRAME(
			interiorHash,
			LibertyCityGFXPos[0],   // 5490.0f
			LibertyCityGFXPos[1],   // -2697.0f
			0,                       // heading
			0);                      // zoom

		if (!loggedOnce)
		{
			char buf[160];
			sprintf_s(buf, sizeof(buf),
				"TriggerLibertyCityRadarInterior: SET_RADAR_AS_INTERIOR_THIS_FRAME(0x%08X, %.1f, %.1f, 0, 0) active",
				(unsigned)interiorHash,
				LibertyCityGFXPos[0],
				LibertyCityGFXPos[1]);
			spdlog::info(buf);
			loggedOnce = true;
		}
	}

	static void MinimapMain()
	{
		while (true)
		{
			CreateMinimap();
			TriggerLibertyCityRadarInterior();
			WAIT(0);
		}
	}
}

// Hooks to disable rendering of the los santos minimap

static void (*gImpl_CMiniMap_RenderThread__RenderBitmaps)();
static void lcImpl_CMiniMap_RenderThread__RenderBitmaps()
{
	if (worldtravel::gameversion::GetGameVersion())
	{

	}
	else
	{
		if (worldtravel::IsLosSantos() || worldtravel::IsNorthYankton() || worldtravel::IsCayoPerico())
			gImpl_CMiniMap_RenderThread__RenderBitmaps();
	}
}

static void (*gImpl_CSupertiles__Render)(void* self);
static void lcImpl_CSupertiles__Render(void* self)
{
	if (worldtravel::gameversion::GetGameVersion())
	{

	}
	else
	{
		if (worldtravel::IsLosSantos() || worldtravel::IsNorthYankton() || worldtravel::IsCayoPerico())
			gImpl_CSupertiles__Render(self);
	}
}

void Minimap_Hooks()
{
	char msg[256];

	if (worldtravel::gameversion::GetGameVersion())
	{
		spdlog::warn("Minimap_Hooks: GTA V Enhanced detected - hooks not implemented for this build branch, minimap will not be patched");
		return;
	}

	// Legacy GTA5.exe path. Each pattern is logged with a match count so a
	// stale pattern (Rockstar shifted bytes in a build update) is visible
	// in the log instead of silently no-op-ing or crashing on get_pattern's
	// assert.
	{
		spdlog::info("Minimap_Hooks: scanning CMiniMap_RenderThread::RenderBitmaps pattern");
		auto p = hook::pattern("48 8B C4 55 53 56 57 41 56 41 57 48 8D 68 ? 48 81 EC B8 00 00 00");
		size_t n = p.size();
		sprintf_s(msg, sizeof(msg), "Minimap_Hooks: RenderBitmaps pattern found %zu matches", n);
		spdlog::info(msg);
		if (n != 1)
		{
			spdlog::error("Minimap_Hooks: RenderBitmaps pattern BROKEN, expected 1 match - skipping this hook");
		}
		else
		{
			void* location = p.get(0).get<void>(0);
			MH_STATUS s = MH_CreateHook(location, lcImpl_CMiniMap_RenderThread__RenderBitmaps, (void**)&gImpl_CMiniMap_RenderThread__RenderBitmaps);
			if (s != MH_OK)
			{
				sprintf_s(msg, sizeof(msg), "Minimap_Hooks: MH_CreateHook(RenderBitmaps) failed with status %d", (int)s);
				spdlog::error(msg);
			}
			else
			{
				MH_EnableHook(location);
				sprintf_s(msg, sizeof(msg), "Minimap_Hooks: RenderBitmaps hook installed at 0x%p", location);
				spdlog::info(msg);
			}
		}
	}

	{
		spdlog::info("Minimap_Hooks: scanning CSupertiles::Render pattern");
		auto p = hook::pattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 56 48 83 EC 20 4C 8B F1 33 DB");
		size_t n = p.size();
		sprintf_s(msg, sizeof(msg), "Minimap_Hooks: Supertiles::Render pattern found %zu matches", n);
		spdlog::info(msg);
		if (n != 1)
		{
			spdlog::error("Minimap_Hooks: Supertiles::Render pattern BROKEN, expected 1 match - skipping this hook");
		}
		else
		{
			void* location = p.get(0).get<void>(0);
			MH_STATUS s = MH_CreateHook(location, lcImpl_CSupertiles__Render, (void**)&gImpl_CSupertiles__Render);
			if (s != MH_OK)
			{
				sprintf_s(msg, sizeof(msg), "Minimap_Hooks: MH_CreateHook(Supertiles::Render) failed with status %d", (int)s);
				spdlog::error(msg);
			}
			else
			{
				MH_EnableHook(location);
				sprintf_s(msg, sizeof(msg), "Minimap_Hooks: Supertiles::Render hook installed at 0x%p", location);
				spdlog::info(msg);
			}
		}
	}
}

void Minimap()
{
	srand(GetTickCount());
	minimap::MinimapMain();
}
