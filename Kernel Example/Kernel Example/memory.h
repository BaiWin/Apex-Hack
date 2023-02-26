#pragma once
#include "imports.h"

PVOID get_system_module_base(const char* module_name);
PVOID get_system_module_export(const char* module_name, LPCSTR routine_name);
bool write_memory(void* address, void* buffer, size_t size);
bool write_to_read_only_memory(void* address, void* buffer, size_t size);
ULONG64 get_module_base_x64(PEPROCESS proc, UNICODE_STRING module_name);
bool read_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, SIZE_T size);
bool write_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, SIZE_T size);

//
PVOID get_module_base_x64_p(PEPROCESS proc, UNICODE_STRING module_name);

typedef struct _EXMP_MEMORY
{
	void* buffer_address;
	UINT_PTR address;
	ULONGLONG size;
	ULONG pid;
	BOOLEAN read;
	BOOLEAN write;
	BOOLEAN req_base;
	BOOLEAN draw_box;
	void* output;
	const char* module_name;
	ULONG64 base_address;
	PVOID address1;
}EXMP_MEMORY;