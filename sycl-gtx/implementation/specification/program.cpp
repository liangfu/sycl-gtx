#include "program.h"

#include "kernel.h"
#include "queue.h"
#include "../debug.h"

using namespace cl::sycl;

program::program(cl_program clProgram, const context& context, vector_class<device> deviceList)
	: prog(clProgram), ctx(context), devices(deviceList) {}

program::program(const context& context)
	: program(context, context.get_devices()) {}

program::program(const context& context, vector_class<device> deviceList)
	: program(nullptr, context, deviceList) {}

program::program(const context& context, cl_program clProgram)
	: program(clProgram, context, context.get_devices()) {}


void program::compile(string_class compile_options, shared_ptr_class<kernel> kern) {
	kernels.push_back(kern);
	auto& src = kern->src;
	auto code = src.get_code();

	debug() << "Compiled kernel:";
	debug() << code;

	const char* code_p = code.c_str();
	size_t length = code.size();
	cl_int error_code;

	auto p = clCreateProgramWithSource(ctx.get(), 1, &code_p, &length, &error_code);
	detail::error::report(error_code);
	kern->set(ctx, p);

	auto device_pointers = detail::get_cl_array(devices);

	error_code = clCompileProgram(
		kern->prog.get(), devices.size(), device_pointers.data(), compile_options.c_str(),
		0, nullptr, nullptr, nullptr, nullptr
	);

	try {
		detail::error::report(error_code);
	}
	catch(::cl::sycl::exception& e) {
		for(auto& d : devices) {
			report_compile_error(kern, d);
		}
		throw e;
	}
}

void program::report_compile_error(shared_ptr_class<kernel> kern, device& dev) const {
	// http://stackoverflow.com/a/9467325/793006

	// Determine the size of the log
	size_t log_size;
	clGetProgramBuildInfo(kern->prog.get(), dev.get(), CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);

	// Allocate memory for the log
	auto log = new char[log_size];

	// Get the log
	clGetProgramBuildInfo(kern->prog.get(), dev.get(), CL_PROGRAM_BUILD_LOG, log_size, log, nullptr);

	debug() << "Error while compiling program for device" << dev.get_info<info::device::name>() << "\n" << log;

	delete[] log;
}

void program::init_kernels() {
	for(auto& kern : kernels) {
		// The extra kernel parameter is required because of complex dependencies
		kern->src.init_kernel(*this, kern);
	}
}

void program::link(string_class linking_options) {
	if(linked) {
		// TODO: Error?
		return;
	}

	auto device_pointers = detail::get_cl_array(devices);
	auto p = prog.get();
	cl_int error_code;

	prog = clLinkProgram(
		ctx.get(),
		device_pointers.size(),
		device_pointers.data(),
		linking_options.c_str(),
		1,
		&p,
		nullptr,
		nullptr,
		&error_code
	);
	detail::error::report(error_code);

	// Can only initialize after program successfully built
	init_kernels();

	linked = true;
}
