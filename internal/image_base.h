#pragma once
#include <cstdint>

// TIB ---> TEB (linear address) ---> PEB (base address)
extern "C" std::uintptr_t __image_base(void);