#include <atomic>
#include <Windows.h>
#include "no_gui/no_gui.h"
#include "no_imports.h"

no_gui gui;
std::atomic<bool> draw_first_init = true;
color blue(255, 0, 0, 255);
color red(255, 255, 0, 0);

__declspec(dllexport) PHANDLE event_handle = nullptr;
using EtwEventUnregister = HANDLE(__fastcall*)(HANDLE);

enum event_type
{
	present = 0x500100110000B2,
	resize_buffers = 0x550200110000BD
};

void draw_handler(IDXGISwapChain* swapchain)
{
	if (draw_first_init.exchange(false))
	{
		gui.init(swapchain);
		const auto event_unregister = 
			LI_FN(GetProcAddress)(
				LI_FN(GetModuleHandleA)("ntdll.dll"),
				"EtwEventUnregister"
			);

		reinterpret_cast<EtwEventUnregister>(event_unregister)(*event_handle);
	}

	gui.begin_scene();
	{
		gui.draw_circle(gui.get_width() / 2, gui.get_height() / 2, 100, blue);
		gui.draw_cross(gui.get_width() / 2, gui.get_height() / 2, 100, 100, red);
	}
	gui.end_scene();
}

extern "C" __declspec(dllexport) ULONG event_handler
(
	HANDLE handle,
	event_type* event_des,
	std::uint32_t data_count,
	IDXGISwapChain*** swapchain
)
{
	if (event_des && swapchain)
	{
		// switch on the event, there is a present event
		// a resize buffer event (if the window resizes)
		// there are a bunch of other events you can get too...
		switch (*event_des)
		{
		case present:
			draw_handler(**swapchain);
			break;
		default:
			break;
		}
	}
	return true;
}