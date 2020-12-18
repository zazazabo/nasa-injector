#include <iostream>
#include "kernel_ctx/kernel_ctx.h"
#include "mem_ctx/mem_ctx.hpp"
#include "injector_ctx/injector_ctx.hpp"
#include "util/util.hpp"

int __cdecl main(int argc, char** argv)
{
	if (argc < 2)
	{
		MessageBoxA(NULL, "ERROR", "please provide dll", NULL);
		return -1;
	}
	std::vector<std::uint8_t> raw_image;
	util::open_binary_file(argv[1], raw_image);

	nasa::load_drv();
	nasa::kernel_ctx kernel_ctx;
	nasa::mem_ctx syscall_fix(kernel_ctx, GetCurrentProcessId());
	kernel_ctx.fix_syscall(syscall_fix);

	//
	// shoot the tires off piddb cache entry.
	//
	{
		const auto drv_timestamp = util::get_file_header((void*)raw_driver)->TimeDateStamp;
		printf("[+] clearing piddb cache for driver: %s, with timestamp 0x%x\n", nasa::drv_key.c_str(), drv_timestamp);

		if (!kernel_ctx.clear_piddb_cache(nasa::drv_key, drv_timestamp))
		{
			// this is because the signature might be broken on these versions of windows.
			perror("[-] failed to clear PiDDBCacheTable.\n");
			return -1;
		}
		printf("[+] cleared piddb cache...\n");
	}
	nasa::unload_drv();

	//
	// load the dll but dont actually call dll main
	//
	auto temp_module_base = LoadLibraryEx(argv[1], NULL, DONT_RESOLVE_DLL_REFERENCES);
	std::cout << "[+] temp_module_base: " << temp_module_base << std::endl;
	if (!temp_module_base)
		return -1;

	//
	// get dll export RVA's
	//
	auto present_rva = (uint64_t)GetProcAddress(temp_module_base, "?event_handler@@YAKPEAXPEAW4event_type@@IPEAPEAPEAUIDXGISwapChain@@@Z") - (uint64_t)temp_module_base;
	std::cout << "[+] present_rva: " << present_rva << std::endl;

	if (!present_rva)
		return -1;

	auto pid = util::get_pid("RainbowSix.exe");
	auto base_addr = util::get_module_base(pid, "RainbowSix.exe");
	std::cout << "[+] pid: " << pid << " base address: " << base_addr << std::endl;

	if (!pid || !base_addr)
		return -1;

	//
	// create memory contexts for the game and this process
	//
	nasa::mem_ctx explorer(kernel_ctx, util::get_pid("explorer.exe"));
	nasa::mem_ctx r6(kernel_ctx, pid);

	//
	// inject into the game
	//
	nasa::injector_ctx inject(r6, explorer);
	const auto [inject_addr, inject_from_addr] = inject.inject(raw_image);

	std::cout << "[+] injected into address: " << inject_addr << std::endl;
	std::cout << "[+] (module base inside of injector) inject_from_addr: " << inject_from_addr << std::endl;
	std::cin.get();

	//
	// install hooks on the game
	//
	auto result = inject.hook(
		(void*)(((uint64_t)inject_addr) + present_rva)
	);

	std::cout << "[+] hook result: " << result << std::endl;
	std::cin.get();

	explorer.~mem_ctx();
	r6.~mem_ctx();
	syscall_fix.~mem_ctx();
}
 