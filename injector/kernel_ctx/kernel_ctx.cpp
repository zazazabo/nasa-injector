#include "kernel_ctx.h"
#include "../mem_ctx/mem_ctx.hpp"

namespace nasa
{
	kernel_ctx::kernel_ctx()
	{
		if (psyscall_func.load() || nt_page_offset || ntoskrnl_buffer)
			return;

		nt_rva = reinterpret_cast<std::uint32_t>(
			util::get_kernel_export(
				"ntoskrnl.exe",
				syscall_hook.first.data(),
				true
			));

		nt_page_offset = nt_rva % PAGE_SIZE;
		ntoskrnl_buffer = reinterpret_cast<std::uint8_t*>(LoadLibraryA(ntoskrnl_path));

		DBG_PRINT("[+] page offset of %s is 0x%llx\n", syscall_hook.first.data(), nt_page_offset);
		DBG_PRINT("[+] ntoskrnl_buffer: 0x%p\n", ntoskrnl_buffer);
		DBG_PRINT("[!] ntoskrnl_buffer was 0x%p, nt_rva was 0x%p\n", ntoskrnl_buffer, nt_rva);

		std::vector<std::thread> search_threads;
		//--- for each physical memory range, make a thread to search it
		for (auto ranges : util::pmem_ranges)
			search_threads.emplace_back(std::thread(
				&kernel_ctx::map_syscall,
				this,
				ranges.first,
				ranges.second
			));

		for (std::thread& search_thread : search_threads)
			search_thread.join();

		DBG_PRINT("[+] psyscall_func: 0x%p\n", psyscall_func.load());
	}

	void kernel_ctx::map_syscall(std::uintptr_t begin, std::uintptr_t end) const
	{
		//if the physical memory range is less then or equal to 2mb
		if (begin + end <= 0x1000 * 512)
		{
			auto page_va = nasa::map_phys(begin + nt_page_offset, end);
			if (page_va)
			{
				// scan every page of the physical memory range
				for (auto page = page_va; page < page_va + end; page += 0x1000)
					if (!psyscall_func.load()) // keep scanning until its found
						if (!memcmp(reinterpret_cast<void*>(page), ntoskrnl_buffer + nt_rva, 32))
						{
							psyscall_func.store((void*)page);
							return;
						}
				nasa::unmap_phys(page_va, end);
			}
		}
		else // else the range is bigger then 2mb
		{
			auto remainder = (begin + end) % (0x1000 * 512);

			// loop over 2m chunks
			for (auto range = begin; range < begin + end; range += 0x1000 * 512)
			{
				auto page_va = nasa::map_phys(range + nt_page_offset, 0x1000 * 512);
				if (page_va)
				{
					// loop every page of 2mbs (512)
					for (auto page = page_va; page < page_va + 0x1000 * 512; page += 0x1000)
					{
						if (!memcmp(reinterpret_cast<void*>(page), ntoskrnl_buffer + nt_rva, 32))
						{
							psyscall_func.store((void*)page);
							return;
						}
					}
					nasa::unmap_phys(page_va, 0x1000 * 512);
				}
			}

			// map the remainder and check each page of it
			auto page_va = nasa::map_phys(begin + end - remainder + nt_page_offset, remainder);
			if (page_va)
			{
				for (auto page = page_va; page < page_va + remainder; page += 0x1000)
				{
					if (!memcmp(reinterpret_cast<void*>(page), ntoskrnl_buffer + nt_rva, 32))
					{
						psyscall_func.store((void*)page);
						return;
					}
				}
				nasa::unmap_phys(page_va, remainder);
			}
		}
	}

	PEPROCESS kernel_ctx::get_peprocess(DWORD pid)
	{
		if (!pid)
			return NULL;

		PEPROCESS proc;
		static auto get_peprocess_from_pid =
			util::get_kernel_export(
				"ntoskrnl.exe",
				"PsLookupProcessByProcessId"
			);
		
		syscall<PsLookupProcessByProcessId>(
			get_peprocess_from_pid,
			(HANDLE)pid,
			&proc
		);
		return proc;
	}

	void kernel_ctx::rkm(void* buffer, void* address, std::size_t size)
	{
		if (!buffer || !address || !size)
			return;

		size_t amount_copied;
		static auto mm_copy_memory =
			util::get_kernel_export(
				"ntoskrnl.exe",
				"memcpy"
			);

		if (mm_copy_memory)
			syscall<decltype(&memcpy)>(
				mm_copy_memory,
				buffer,
				address,
				size
			);
	}

	void kernel_ctx::wkm(void* buffer, void* address, std::size_t size)
	{
		if (!buffer || !address || !size)
			return;

		size_t amount_copied;
		static auto mm_copy_memory =
			util::get_kernel_export(
				"ntoskrnl.exe",
				"memcpy"
			);

		if (mm_copy_memory)
			syscall<decltype(&memcpy)>(
				mm_copy_memory,
				address,
				buffer,
				size
			);
	}

	void* kernel_ctx::get_physical(void* virt_addr)
	{
		if (!virt_addr)
			return NULL;

		static auto mm_get_physical =
			util::get_kernel_export(
				"ntoskrnl.exe",
				"MmGetPhysicalAddress"
			);

		return syscall<MmGetPhysicalAddress>(
			mm_get_physical,
			virt_addr
			);
	}

	void* kernel_ctx::get_virtual(void* addr)
	{
		if (!addr)
			return NULL;

		static auto mm_get_virtual =
			util::get_kernel_export(
				"ntoskrnl.exe",
				"MmGetVirtualForPhysical"
			);

		PHYSICAL_ADDRESS phys_addr;
		memcpy(&phys_addr, &addr, sizeof(addr));
		return syscall<MmGetVirtualForPhysical>(
			mm_get_virtual,
			phys_addr
		);
	}

	bool kernel_ctx::clear_piddb_cache(const std::string& file_name, const std::uint32_t timestamp)
	{
		static const auto piddb_lock =
			util::memory::get_piddb_lock();

		static const auto piddb_table =
			util::memory::get_piddb_table();

		if (!piddb_lock || !piddb_table)
			return false;

		static const auto ex_acquire_resource =
			util::get_kernel_export(
				"ntoskrnl.exe",
				"ExAcquireResourceExclusiveLite"
			);

		static const auto lookup_element_table =
			util::get_kernel_export(
				"ntoskrnl.exe",
				"RtlLookupElementGenericTableAvl"
			);

		static const auto release_resource =
			util::get_kernel_export(
				"ntoskrnl.exe",
				"ExReleaseResourceLite"
			);

		static const auto delete_table_entry =
			util::get_kernel_export(
				"ntoskrnl.exe",
				"RtlDeleteElementGenericTableAvl"
			);

		if (!ex_acquire_resource || !lookup_element_table || !release_resource)
			return false;

		PiDDBCacheEntry cache_entry;
		const auto drv_name = std::wstring(file_name.begin(), file_name.end());
		cache_entry.time_stamp = timestamp;
		RtlInitUnicodeString(&cache_entry.driver_name, drv_name.data());

		//
		// ExAcquireResourceExclusiveLite
		//
		if (!syscall<ExAcquireResourceExclusiveLite>(ex_acquire_resource, piddb_lock, true))
			return false;

		//
		// RtlLookupElementGenericTableAvl
		//
		PIDCacheobj* found_entry_ptr =
			syscall<RtlLookupElementGenericTableAvl>(
				lookup_element_table,
				piddb_table,
				reinterpret_cast<void*>(&cache_entry)
				);

		if (found_entry_ptr)
		{

			//
			// unlink entry.
			//
			PIDCacheobj found_entry = rkm<PIDCacheobj>(found_entry_ptr);
			LIST_ENTRY NextEntry = rkm<LIST_ENTRY>(found_entry.list.Flink);
			LIST_ENTRY PrevEntry = rkm<LIST_ENTRY>(found_entry.list.Blink);

			PrevEntry.Flink = found_entry.list.Flink;
			NextEntry.Blink = found_entry.list.Blink;

			wkm<LIST_ENTRY>(found_entry.list.Blink, PrevEntry);
			wkm<LIST_ENTRY>(found_entry.list.Flink, NextEntry);

			//
			// delete entry.
			//
			syscall<RtlDeleteElementGenericTableAvl>(delete_table_entry, piddb_table, found_entry_ptr);

			//
			// ensure the entry is 0
			//
			auto result = syscall<RtlLookupElementGenericTableAvl>(
				lookup_element_table,
				piddb_table,
				reinterpret_cast<void*>(&cache_entry)
				);

			syscall<ExReleaseResourceLite>(release_resource, piddb_lock);
			return !result;
		}
		syscall<ExReleaseResourceLite>(release_resource, piddb_lock);
		return false;
	}

	void kernel_ctx::fix_syscall(nasa::mem_ctx& ctx)
	{
		pt_entries syscall_entries; // not used.
		nasa::psyscall_func =
			ctx.set_page(
				ctx.virt_to_phys(
					syscall_entries,
					nasa::psyscall_func
				));

		nasa::unmap_all();
	}
}