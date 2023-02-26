#include "hook.h"

extern "C" NTSTATUS FxDriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING reg_path)
{
	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(reg_path);

	hook::call_kernel_function(&hook::hook_handler);
	DbgPrintEx(0, 0, "Entry!");

	return STATUS_SUCCESS;
}