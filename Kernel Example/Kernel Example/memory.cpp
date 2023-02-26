#include "memory.h"
#include "utils.h"

PVOID get_system_module_base(const char* module_name)
{
	ULONG bytes = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, 0, bytes, &bytes);

	if (!bytes)
		return 0;

	//The ExAllocatePool2 routine allocates pool memory of the specified type and returns a pointer to the allocated block.
	//PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)ExAllocatePool2(POOL_FLAG_NON_PAGED, bytes, 0x4e554c4c); // 0x... == Null
	PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, bytes, 0x454E4F45);

	status = ZwQuerySystemInformation(SystemModuleInformation, modules, bytes, &bytes);

	if (!NT_SUCCESS(status))
		return 0;

	PRTL_PROCESS_MODULE_INFORMATION module = modules->Modules;
	PVOID module_base = 0, module_size = 0;
	
	for (ULONG i = 0; i < modules->NumberOfModules; i++)
	{
		if (strcmp((char*)module[i].FullPathName, module_name) == 0)
		{
			module_base = module[i].ImageBase;
			module_size = (PVOID)module[i].ImageSize;
			break;
		}
	}

	if (modules) {
		//The ExFreePoolWithTag routine deallocates a block of pool memory allocated with the specified tag
		ExFreePoolWithTag(modules, 0);
	}

	if (module_base <= 0)
		return 0;

	return module_base;
}

PVOID get_system_module_export(const char* module_name, LPCSTR routine_name)
{
	PVOID lpModule = get_system_module_base(module_name);

	if (!lpModule)
		return NULL;

	return RtlFindExportedRoutineByName(lpModule, routine_name);
}

bool write_memory(void* address, void* buffer, size_t size)
{
	//The RtlCopyMemory routine copies the contents of a source memory block to a destination memory block
	if (!RtlCopyMemory(address, buffer, size))
		return false;
	else
		return true;
}

bool write_to_read_only_memory(void* address, void* buffer, size_t size)
{
	//The IoAllocateMdl routine allocates a memory descriptor list (MDL) large enough to map a buffer, given the buffer's starting address and length. Optionally, this routine associates the MDL with an IRP
	PMDL Mdl = IoAllocateMdl(address, size, FALSE, FALSE, NULL);

	if (!Mdl)
		return false;

	//The MmProbeAndLockPages routine probes the specified virtual memory pages, makes them resident, and locks them in memory (say for a DMA transfer). This ensures the pages cannot be freed and reallocated while a device driver (or hardware) is still using them
	MmProbeAndLockPages(Mdl, KernelMode, IoReadAccess);
	//The MmMapLockedPagesSpecifyCache routine maps the physical pages that are described by an MDL to a virtual address, and enables the caller to specify the cache attribute that is used to create the mapping
	PVOID Mapping = MmMapLockedPagesSpecifyCache(Mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
	//The MmProtectMdlSystemAddress routine sets the protection type for a memory address range
	MmProtectMdlSystemAddress(Mdl, PAGE_READWRITE);

	write_memory(Mapping, buffer, size);

	MmUnmapLockedPages(Mapping, Mdl);
	MmUnlockPages(Mdl);
	IoFreeMdl(Mdl);

	return true;
}

//
//ULONG64 get_module_base_x64_import(PEPROCESS proc) {
//	return (ULONG64)PsGetProcessSectionBaseAddress(proc);
//}

//

ULONG64 get_module_base_x64(PEPROCESS proc, UNICODE_STRING module_name) 
{
	PPEB pPeb = PsGetProcessPeb(proc);

	if (!pPeb)
		return NULL;

	KAPC_STATE state;

	KeStackAttachProcess(proc, &state);

	PPEB_LDR_DATA pLdr = (PPEB_LDR_DATA)pPeb->Ldr;

	if (!pLdr)
	{
		KeUnstackDetachProcess(&state);
		return NULL;
	}

	for (PLIST_ENTRY list = (PLIST_ENTRY)pLdr->ModuleListLoadOrder.Flink; list != &pLdr->ModuleListLoadOrder; list = (PLIST_ENTRY)list->Flink)
	{
		PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);

		if (RtlCompareUnicodeString(&pEntry->BaseDllName, &module_name, TRUE) == NULL)
		{
			PVOID temp = pEntry->DllBase;
			//ULONG64 baseAddr = (ULONG64)pEntry->DllBase;
			ULONG64* pd = static_cast<ULONG64*>(temp);
			ULONG64 baseAddr = 0;
			//ULONG64 baseAddr = *((ULONG64*)temp);
			KeUnstackDetachProcess(&state);
			return baseAddr;
		}
	}

	KeUnstackDetachProcess(&state);
	return NULL;
}

PVOID get_module_base_x64_p(PEPROCESS proc, UNICODE_STRING module_name)
{
	PPEB pPeb = PsGetProcessPeb(proc);

	if (!pPeb)
		return NULL;

	KAPC_STATE state;

	KeStackAttachProcess(proc, &state);

	PPEB_LDR_DATA pLdr = (PPEB_LDR_DATA)pPeb->Ldr;

	if (!pLdr)
	{
		KeUnstackDetachProcess(&state);
		return NULL;
	}

	for (PLIST_ENTRY list = (PLIST_ENTRY)pLdr->ModuleListLoadOrder.Flink; list != &pLdr->ModuleListLoadOrder; list = (PLIST_ENTRY)list->Flink)
	{
		PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);

		if (RtlCompareUnicodeString(&pEntry->BaseDllName, &module_name, TRUE) == NULL)
		{
			PVOID baseAddr = pEntry->DllBase;
			KeUnstackDetachProcess(&state);
			return baseAddr;
		}
	}

	KeUnstackDetachProcess(&state);
	return NULL;
}

bool read_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, SIZE_T size)
{
	if (!address || !buffer || !size)
		return false;

	SIZE_T bytes = 0;
	NTSTATUS status = STATUS_SUCCESS;
	PEPROCESS process;
	PsLookupProcessByProcessId((HANDLE)pid, &process);

	status = MmCopyVirtualMemory(process, (void*)address, (PEPROCESS)PsGetCurrentProcess(), (void*)buffer, size, KernelMode, &bytes);

	if (!NT_SUCCESS(status))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool write_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, SIZE_T size)
{
	if (!address || !buffer || !size)
		return false;

	NTSTATUS status = STATUS_SUCCESS;
	PEPROCESS process;
	PsLookupProcessByProcessId((HANDLE)pid, &process);

	KAPC_STATE state;
	KeStackAttachProcess((PEPROCESS)process, &state);

	MEMORY_BASIC_INFORMATION info;

	status = ZwQueryVirtualMemory(ZwCurrentProcess(), (PVOID)address, MemoryBasicInformation, &info, sizeof(info), NULL);

	if (!NT_SUCCESS(status))
	{
		KeUnstackDetachProcess(&state);
		return false;
	}

	if (((uintptr_t)info.BaseAddress + info.RegionSize) < (address + size))
	{
		KeUnstackDetachProcess(&state);
		return false;
	}

	if (!(info.State & MEM_COMMIT) || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
	{
		KeUnstackDetachProcess(&state);
		return false;
	}

	if ((info.Protect & PAGE_EXECUTE_READWRITE) || (info.Protect & PAGE_EXECUTE_WRITECOPY)
		|| (info.Protect & PAGE_READWRITE) || (info.Protect & PAGE_WRITECOPY))
	{
		RtlCopyMemory((void*)address, buffer, size);
	}
	KeUnstackDetachProcess(&state);
	return true;
}