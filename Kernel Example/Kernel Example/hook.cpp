#include "hook.h"
#include "renderer.h"
#include "utils.h"

extern "C" void ExampleFunction();

bool hook::call_kernel_function(void* kernel_function_address)
{
	if (!kernel_function_address)
		return false;

	PVOID* function_origin = reinterpret_cast<PVOID*>(get_system_module_export("\\SystemRoot\\System32\\drivers\\dxgkrnl.sys", "NtQueryCompositionSurfaceStatistics"));
	PVOID* function = reinterpret_cast<PVOID*>(get_system_module_export("\\SystemRoot\\System32\\drivers\\dxgkrnl.sys", "NtOpenCompositionSurfaceSectionInfo"));
	
	if (!function)
		return false;

	//Function is 8 bytes!

	//This one works!!!
	/*BYTE orig[] = { 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };*/
	//This one works!!!

	BYTE orig[] = { 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

	//BYTE shell_code[] = { 0x41, 0x57, 0x49, 0xBF }; // 
	//BYTE shell_code_end[] = { 0x49, 0x83, 0xC7, 0x09, 0x49, 0x8D, 0x07, 0x50, 0x48, 0x83, 0xE8, 0x09, 0x58, 0x41, 0x5F, 0xFF, 0xE0 }; //

	//temp
	//BYTE shell_code[] = { 0x41, 0x57, 0x49, 0xBF, };
	//BYTE shell_code_end[] = { 0x49, 0x8D, 0x07, 0x50, 0x49, 0x01, 0xCF, 0x48, 0x29, 0xD8, 0x41, 0x5F, 0x58, 0x48, 0x01, 0xD8, 0x49, 0x29, 0xCF, 0xFF, 0xE0 };

	BYTE shell_code[] = { 0x41, 0x57, 0x49, 0xBF, };
	BYTE shell_code_end[] = { 0x4D, 0x8D, 0x37, 0x41, 0x56, 0x49, 0x01, 0xCF, 0x41, 0x5E, 0x41, 0x5F, 0x49, 0x29, 0xCF, 0x41, 0xFF, 0xE6 };
	/*push r15
		movabs r15, 0x1
		lea r14, [r15]
			push r14
		add r15, rcx
		pop r15
			pop r14
		sub r15, rcx
		jmp r14*/

	//Basic new
	/*BYTE shell_code[] = { 0x41, 0x57, 0x49, 0xBF, };
	BYTE shell_code_end[] = { 0x49, 0x8D, 0x07, 0x50, 0x41, 0x5F, 0x58, 0xFF, 0xE0 };*/
	//Basic new

	//Basic
	//BYTE shell_code[] = { 0x49, 0xBF, }; // 
	//BYTE shell_code_end[] = { 0x49, 0x8D, 0x07, 0xFF, 0xE0 }; //
	//Basic

	//This one works!!!rax
	//BYTE shell_code[] = { 0x48, 0xB8}; // mov rax, ...
	//BYTE shell_code_end[] = { 0x48, 0xFF, 0xC0, 0x48, 0xFF, 0xC8, 0x50, 0x58, 0xFF, 0xE0 }; // push pop jmp rax
	//This one works!!!rax
	
	//This one works 2! R15
	//BYTE shell_code[] = { 0x49, 0xBF }; // This one
	//BYTE shell_code_end[] = { 0x41, 0x57, 0x49, 0xFF, 0xC7, 0x49, 0xFF, 0xCF, 0x41, 0x5F, 0x41, 0xFF, 0xE7 }; // This one
	//This one works 2! R15

	//BYTE i_add[] = { 0x41, 0xFF, 0xE7 };

	//The RtlSecureZeroMemory routine fills a block of memory with zeros in a way that is guaranteed to be secure
	RtlSecureZeroMemory(&orig, sizeof(orig));
	//The memcpy copies bytes between buffers
	memcpy((PVOID)((ULONG_PTR)orig), &shell_code, sizeof(shell_code));
	//The reinterpret_cast allows any pointer to be converted into any other pointer type. Also allows any integral type to be converted into any pointer type and vice versa
	uintptr_t hook_address = reinterpret_cast<uintptr_t>(kernel_function_address);

	memcpy((PVOID)((ULONG_PTR)orig + sizeof(shell_code)), &hook_address, sizeof(void*));
	memcpy((PVOID)((ULONG_PTR)orig + sizeof(shell_code) + sizeof(void*)), &shell_code_end, sizeof(shell_code_end));

	//
	//uintptr_t nothing_address = reinterpret_cast<uintptr_t>(&Nothing);
	//memcpy((PVOID)((ULONG_PTR)orig + sizeof(shell_code) + sizeof(void*) + sizeof(shell_code_end)), &nothing_address, sizeof(void*));
	//memcpy((PVOID)((ULONG_PTR)orig + sizeof(shell_code) + sizeof(void*) + sizeof(shell_code_end) + sizeof(void*)), &i_add, sizeof(i_add));
	//

	write_to_read_only_memory(function, &orig, sizeof(orig));
	
	return true;
}

void hook::Nothing()
{
	
}

NTSTATUS hook::hook_handler(PVOID called_param)
{
	ExampleFunction();
	DbgPrintEx(0, 0, "hook is called!");

	EXMP_MEMORY* instructions = (EXMP_MEMORY*)called_param;

	if (instructions->req_base == TRUE)
	{
		ANSI_STRING AS;
		UNICODE_STRING ModuleName;

		RtlInitAnsiString(&AS, instructions->module_name);
		RtlAnsiStringToUnicodeString(&ModuleName, &AS, TRUE);

		PEPROCESS process;
		PsLookupProcessByProcessId((HANDLE)instructions->pid, &process);
		ULONG64 base_address64 = NULL;
		//base_address64 = get_module_base_x64(process, ModuleName);
		PVOID temp = get_module_base_x64_p(process, ModuleName);
		instructions->address1 = temp;
		instructions->base_address = base_address64;

		RtlFreeUnicodeString(&ModuleName);
	}

	else if (instructions->write == TRUE)
	{
		if (instructions->address < 0x7FFFFFFFFFFF && instructions->address > 0)
		{
			PVOID kernelBuff = ExAllocatePool(NonPagedPool, instructions->size);

			if (!kernelBuff)
			{
				return STATUS_UNSUCCESSFUL;
			}

			if (!memcpy(kernelBuff, instructions->buffer_address, instructions->size))
			{
				return STATUS_UNSUCCESSFUL;
			}

			PEPROCESS process;
			PsLookupProcessByProcessId((HANDLE)instructions->pid, &process);
			write_kernel_memory((HANDLE)instructions->pid, instructions->address, kernelBuff, instructions->size);
			ExFreePool(kernelBuff);
		}
	}

	else if (instructions->read == TRUE)
	{
		if (instructions->address < 0x7FFFFFFFFFFF && instructions->address > 0)
		{
			read_kernel_memory((HANDLE)instructions->pid, instructions->address, instructions->output, instructions->size);
		}
	}

	else if (instructions->draw_box == TRUE)
	{
		if (!render::init()) {
			print("failed to initalize render functions\n");
			return STATUS_UNSUCCESSFUL;
		}

		render::draw_box(50, 50, 50, 50, 1, { 0, 0, 0 });
	}

	return STATUS_SUCCESS;
}
