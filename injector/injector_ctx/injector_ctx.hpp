#include <map>
#include "../mem_ctx/mem_ctx.hpp"
#include "../pe_image/pe_image.h"

namespace nasa
{
	class injector_ctx
	{
	public:
		injector_ctx(
			nasa::mem_ctx& inject_into,
			nasa::mem_ctx& inject_from
		);
		std::pair<void*, void*> inject(std::vector<std::uint8_t>& raw_image, const unsigned pml4_index = 179);
		bool hook(void* present_hook);
	private:

		nasa::mem_ctx inject_into;
		nasa::mem_ctx inject_from;
	};
}