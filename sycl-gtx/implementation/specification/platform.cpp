#include "platform.h"
#include "device.h"

#include <utility>
#include "../debug.h"

using namespace cl::sycl;


platform::platform(cl_platform_id platform_id, error_handler& handler)
	: platform_id(refc::allocate(platform_id)), handler(handler) {}

platform::platform(error_handler& handler)
	: platform(nullptr, handler) {}

platform::platform(int& error_code)
	: platform(nullptr, error_code) {}

platform::platform(cl_platform_id platform_id, int& error_code)
	: platform_id(refc::allocate(platform_id)), handler(error_code) {}

cl_platform_id platform::get() const {
	return platform_id.get();
}

VECTOR_CLASS<platform> platform::get_platforms(int& error_code) {
	return get_platforms(detail::error::handler(error_code));
}
VECTOR_CLASS<platform> platform::get_platforms(error_handler& handler) {
	return get_platforms(detail::error::handler(handler));
}

VECTOR_CLASS<platform> platform::get_platforms(detail::error::handler& handler) {
	static const int MAX_PLATFORMS = 1024;
	cl_platform_id platform_ids[MAX_PLATFORMS];
	cl_uint num_platforms;
	auto error_code = clGetPlatformIDs(MAX_PLATFORMS, platform_ids, &num_platforms);
	handler.report(error_code);
	return VECTOR_CLASS<platform>(platform_ids, platform_ids + num_platforms);
}

VECTOR_CLASS<device> platform::get_devices(cl_device_type device_type) {
	return detail::get_devices(device_type, platform_id, handler);
}

// TODO: How to check for this?
bool platform::is_host() {
	DSELF() << "not implemented";
	return true;
}

bool platform::has_extension(const STRING_CLASS extension_name) {
	return detail::has_extension<CL_PLATFORM_EXTENSIONS>(this, extension_name);
}
