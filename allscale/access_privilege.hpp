#ifndef ALLSCALE_ACCESS_PRIVILEGE_HPP
#define ALLSCALE_ACCESS_PRIVILEGE_HPP

namespace allscale {
	enum class access_privilege
	{
		read_only,
		write_first,
		read_write,
		write_only,
		accelerator_read_access,
		accelerator_write_access,
		accelerator_read_write_access,
	};
}
#endif
