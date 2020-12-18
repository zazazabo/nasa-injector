#include "injector_ctx.hpp"
#include "../direct.h"
#include <iostream>

namespace nasa
{
	injector_ctx::injector_ctx(
		mem_ctx& inject_into,
		mem_ctx& inject_from
	)
		:
		inject_into(inject_into),
		inject_from(inject_from)
	{}

	std::pair<void*, void*> injector_ctx::inject(
		std::vector<std::uint8_t>& raw_image,
		const unsigned pml4_index
	)
	{
		nasa::pe_image image(raw_image);
		const auto p_handle = OpenProcess(
			PROCESS_ALL_ACCESS,
			FALSE,
			inject_from.get_pid()
		);

		//
		// allocate rwx memory for dll inside of explorer.exe
		//
		const auto module_base = direct::alloc_virtual_memory(
			p_handle,
			image.size() + 0x1000 * 512, //image size + 2mb (creates a new pde/pt)
			PAGE_EXECUTE_READWRITE
		);

		image.fix_imports(NULL, NULL); // no imports
		image.map();
		image.relocate(reinterpret_cast<std::uintptr_t>(module_base));

		//
		// write dll into explorer.exe
		//
		auto result = direct::write_virtual_memory(
			p_handle,
			module_base,
			image.data(), 
			image.size()
		);

		if (!result)
			return {};

		const auto [ppml4e, pml4e] = inject_from.get_pml4e(module_base);
		if (!ppml4e || !pml4e.value)
			return {};

		const auto inject_into_dirbase = reinterpret_cast<::ppml4e>(inject_into.get_dirbase());
		if (!inject_into_dirbase)
			return {};

		//
		// write in pml4e containing dll at desired index
		//
		inject_into.write_phys<::pml4e>(
			inject_into_dirbase + pml4_index,
			pml4e
		);

		CloseHandle(p_handle);
		virt_addr_t new_addr{ module_base };
		new_addr.pml4_index = pml4_index;
		return { new_addr.value, module_base };
	}

	bool injector_ctx::hook(void* present_hook)
	{
		if (!present_hook)
			return false;

		//
		// get dxgi.dll base address
		//
		const auto dxgi = (std::uintptr_t)util::get_module_base(
			inject_into.get_pid(), 
			"dxgi.dll"
		);

		std::cout << "[+] dxgi.dll base address: " << std::hex << dxgi << std::endl;
		std::cin.get();

		//
		// IAT hook EtwEventWrite to our function
		//
		inject_into.write_virtual<void*>(
			reinterpret_cast<void*>(dxgi + 0x9F000),
			present_hook
		);

		std::cout << "[+] iat hooked: " << std::hex << dxgi + 0x9F000 << " to: " << present_hook << std::endl;
		std::cin.get();

		//
		// enable EtwEventWrite to be called
		//
		{
			inject_into.write_virtual<std::uint32_t>(
				reinterpret_cast<void*>(dxgi + 0xCB024),
				1
			);

			inject_into.write_virtual<std::uint64_t>(
				reinterpret_cast<void*>(dxgi + 0xCB010),
				0xFFFFFFFFFFFFFFFF
			);

			inject_into.write_virtual<std::uint64_t>(
				reinterpret_cast<void*>(dxgi + 0xCB018),
				0x4000000000000000
			);
		}
		return true;
	}
}