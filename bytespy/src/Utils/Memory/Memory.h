#pragma once
#include "../Feature/Feature.h"
#include <Windows.h>
#include <cstdint>
#include <vector>

class CMemory
{
public:
	std::vector<byte> PatternToByte(const char* szPattern);
	std::vector<int> PatternToInt(const char* szPattern);
	uintptr_t FindSignature(const char* szModule, const char* szPattern);
	PVOID FindInterface(const char* szModule, const char* szObject);

	inline void* GetVFunc(void* instance, size_t index)
	{
		const auto vtable = *static_cast<void***>(instance);
		return vtable[index];
	}

	inline uintptr_t RelToAbs(const uintptr_t address)
	{
		return *reinterpret_cast<std::int32_t*>(address + 0x3) + address + 0x7;
	}

	inline uintptr_t RelToAbsCustom(uintptr_t Address, uintptr_t RVAOffset)
	{
		// learn more: https://www.unknowncheats.me/forum/4432156-post17.html        
		//64 bit relative addresses: (address of ptr in instruction) + instruction + (size of instruction)

		return *reinterpret_cast<int32_t*>(Address + RVAOffset) + Address + (RVAOffset + sizeof(int32_t));
	}

	/* vfunc */
	inline void**& get_vtable(void* inst, const unsigned int offset)
	{
		return *reinterpret_cast<void***>(reinterpret_cast<uintptr_t>(inst) + offset);
	}

	inline const void** get_vtable(const void* inst, const unsigned int offset)
	{
		return *reinterpret_cast<const void***>(reinterpret_cast<uintptr_t>(inst) + offset);
	}

	template<typename T>
	inline T GetVFUNC(void* inst, const unsigned int index, const unsigned int offset = 0u) {
		return reinterpret_cast<T>(get_vtable(inst, offset)[index]);
	}

	template <typename T>
	inline T GetModuleExport(const char* szModule, const char* szExport)
	{
		if (auto hModule = GetModuleHandle(szModule))
			return reinterpret_cast<T>(GetProcAddress(hModule, szExport));
		return reinterpret_cast<T>(nullptr);
	}
};

ADD_FEATURE_CUSTOM(CMemory, Memory, U)