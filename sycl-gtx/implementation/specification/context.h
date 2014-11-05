#pragma once

#include "refc.h"
#include "device_selector.h"
#include "error_handler.h"
#include "param_traits.h"
#include "../common.h"
#include "../debug.h"

namespace cl {
namespace sycl {

// Forward declarations
class device;
class program;

// Used as the notification function for contexts.
class context_notify {
private:
	friend class context;
	static void CL_CALLBACK forward(const char* errinfo, const void* private_info, size_t cb, void* caller) {
		static_cast<context_notify*>(caller)->operator()(errinfo, private_info, cb);
	}
public:
	virtual void operator()(const STRING_CLASS errinfo, const void* private_info, size_t cb) = 0;
	virtual void operator()(program t_program) = 0;
};

// 2.3.1, point 2
// Any OpenCL resource that is acquired by the user is attached to a context.
// A context contains a collection of devices that the host can use
// and manages memory objects that can be shared between the devices.
// Data movement between devices within a context may be efficient and hidden by the underlying runtime
// while data movement between contexts must involve the host.
// A given context can only wrap devices owned by a single platform.
class context {
private:
	refc::ptr<cl_context> ctx;
	VECTOR_CLASS<device> target_devices;
	unsigned int best_device_id = 0;

	helper::error::handler handler;
	static error_handler& default_error;

	static refc::ptr<cl_context> reserve(cl_context c = nullptr);
	static VECTOR_CLASS<device> load_devices();

	// Master constructor
	context(
		cl_context c,
		const cl_context_properties* properties,
		VECTOR_CLASS<device> target_devices,
		error_handler& handler,
		context_notify* ctx_notify = nullptr,
		device_selector& dev_sel = *(device_selector::default)
	);
public:
	// Error handling via error_handler&
	context(cl_context c = nullptr, error_handler& handler = default_error);
	context(device_selector& dev_sel, error_handler& handler = default_error);
	context(const cl_context_properties* properties, device_selector& dev_sel, error_handler& handler = default_error);
	context(const cl_context_properties* properties, VECTOR_CLASS<device> target_devices, error_handler& handler = default_error);
	context(const cl_context_properties* properties, device target_device, error_handler& handler = default_error);

	// Error handling via context_notify&
	context(context_notify& handler);
	context(cl_context c, context_notify& handler);
	context(device_selector& dev_sel, context_notify& handler);
	context(const cl_context_properties* properties, device_selector& dev_sel, context_notify& handler);
	context(const cl_context_properties* properties, VECTOR_CLASS<device> target_devices, context_notify& handler);
	context(const cl_context_properties* properties, device target_device, context_notify& handler);

	context(const context&) = default;
	context& operator=(const context&) = default;

#if MSVC_LOW
	SYCL_MOVE_OPS(context, {
		SYCL_MOVE(ctx);
		SYCL_MOVE(target_devices);
		SYCL_COPY(best_device_id);
		SYCL_MOVE(handler);
	})
#else
	context(context&&) = default;
	context& operator=(context&& copy) = default;
#endif

public:
	cl_context get();

private:
	template<class return_type, cl_int name>
	struct hidden {
		using real_return = return_type;
		static real_return get_info(context* contex) {
			auto c = contex->ctx.get();
			real_return param_value;
			auto error_code = clGetContextInfo(c, name, sizeof(real_return), &param_value, nullptr);
			contex->handler.report(contex, error_code);
			return param_value;
		}
	};
	template<class return_type, cl_int name>
	struct hidden<return_type[], name> {
		using real_return = VECTOR_CLASS<return_type>;
		static real_return get_info(context* contex) {
			auto c = contex->ctx.get();
			static const int BUFFER_SIZE = 1024;
			return_type param_value[BUFFER_SIZE];
			std::size_t actual_size;
			auto error_code = clGetContextInfo(c, name, BUFFER_SIZE, &param_value, &actual_size);
			contex->handler.report(contex, error_code);
			return helper::to_vector<return_type>(param_value, actual_size);
		}
	};
public:
	template<cl_int name>
	using param = typename param_traits<cl_context_info, name>::param_type;
	template<cl_int name>
	typename hidden<param<name>, name>::real_return get_info() {
		return hidden<param<name>, name>::get_info(this);
	}
};

} // namespace sycl
} // namespace cl
