#include "context.h"
#include "device.h"

using namespace cl::sycl;

error_handler& context::default_error = helper::error::handler::default;

refc::ptr<cl_context> context::reserve(cl_context c) {
	return refc::allocate(c, clReleaseContext);
}

// TODO: Auto load devices
VECTOR_CLASS<device> context::load_devices() {
	return {};
}

// Master constructor
context::context(
	cl_context c,
	const cl_context_properties* properties,
	VECTOR_CLASS<device> target_devices_,
	error_handler& handler,
	context_notify* ctx_notify,
	device_selector& dev_sel
) : ctx(reserve(c)), handler(handler), target_devices(target_devices_) {
	if(c == nullptr) {
		cl_uint num_devices = target_devices.size();

		if(num_devices == 0) {
			target_devices = load_devices();
		}
		best_device_id = helper::select_best_device(dev_sel, target_devices);
		if(best_device_id < 0) {
			// TODO: Maybe an exception?
		}

		VECTOR_CLASS<cl_device_id> devices;
		devices.reserve(num_devices);
		for(auto& device_ptr : target_devices) {
			devices.push_back(device_ptr.get());
		}

		auto pfn_notify = (ctx_notify == nullptr ? nullptr : &context_notify::forward);
		cl_int error_code;
		c = clCreateContext(properties, num_devices, devices.data(), pfn_notify, ctx_notify, &error_code);
		if(pfn_notify == nullptr) {
			this->handler.report(this, error_code);
		}
		ctx = reserve(c);
	}
	else {
		auto error_code = clRetainContext(c);
		this->handler.report(this, error_code);
	}
}

// Error handling via error_handler&
context::context(cl_context c, error_handler& handler)
	: context(c, nullptr, {}, handler) {}
context::context(device_selector& dev_sel, error_handler& handler)
	: context(nullptr, dev_sel, handler) {}
context::context(const cl_context_properties* properties, device_selector& dev_sel, error_handler& handler)
	: context(nullptr, properties, {}, handler, nullptr, dev_sel) {}
context::context(const cl_context_properties* properties, VECTOR_CLASS<device> target_devices, error_handler& handler)
	: context(nullptr, properties, target_devices, handler) {}
context::context(const cl_context_properties* properties, device target_device, error_handler& handler)
	: context(nullptr, properties, { target_device }, handler) {}

// Error handling via context_notify&
context::context(context_notify& handler)
	: context(nullptr, nullptr, {}, default_error, &handler) {}
context::context(cl_context c, context_notify& handler)
	: context(c, nullptr, {}, default_error, &handler) {}
context::context(device_selector& dev_sel, context_notify& handler)
	: context(nullptr, dev_sel, handler) {}
context::context(const cl_context_properties* properties, device_selector& dev_sel, context_notify& handler)
	: context(nullptr, properties, {}, default_error, &handler, dev_sel) {}
context::context(const cl_context_properties* properties, VECTOR_CLASS<device> target_devices, context_notify& handler)
	: context(nullptr, properties, target_devices, default_error, &handler) {}
context::context(const cl_context_properties* properties, device target_device, context_notify& handler)
	: context(nullptr, properties, { target_device }, default_error, &handler) {}

cl_context context::get() {
	return ctx.get();
}
