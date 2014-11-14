#pragma once

// 3.3.1 Buffers

#include "access.h"
#include "event.h"
#include "ranges.h"
#include "refc.h"
#include "queue.h"
#include "../common.h"
#include "../debug.h"
#include <vector>

namespace cl {
namespace sycl {

// Forward declaration.
template <typename dataType, int dimensions, access::mode mode, access::target target>
class accessor;

namespace detail {

template <typename DataType, int dimensions>
class buffer {
private:
	range<dimensions> rang;
	refc::ptr<cl_mem> data;

public:
	// TODO: As a convenience for the user, any constructor that takes a range argument
	// can instead be passed range values as 1, 2 or 3 arguments of type size_t.

	// No associated storage.
	// The storage for this type of buffer is entirely handled by the SYCL system.
	// The destructor for this type of buffer never blocks, even if work on the buffer has not completed.
	// Instead, the SYCL system frees any storage required for the buffer asynchronously when it is no longer in use in queues.
	// The initial contents of the buffer are undefined.
	buffer(range<dimensions> range);

	// Associated host memory.
	// The buffer will use this host memory for its full lifetime,
	// but the contents of this host memory are undefined for the lifetime of the buffer.
	// If the host memory is modified by the host, or mapped to another buffer or image during the lifetime of this buffer,
	// then the results are undefined.
	// The initial contents of the buffer will be the contents of the host memory at the time of construction.
	// When the buffer is destroyed, the destructor will block until all work in queues on the buffer has completed,
	// then copy the contents of the buffer back to the host memory (if required) and then return.
	buffer(DataType* host_data, range<dimensions> range)
		: rang(range) {
		DSELF() << "not implemented";
	}

	// Associated storage object.
	// The storage object must not be destroyed by the user until after the buffer has been destroyed.
	// The synchronization and copying behavior of the storage is determined by the storage object.
	//buffer(storage<DataType>& store, range<dimensions>);

	// Creates a sub-buffer object, which is a sub-range reference to a base buffer.
	// This sub-buffer can be used to create accessors to the base buffer,
	// but which only have access to the range specified at time of construction of the sub-buffer.
	//buffer(buffer, index<dimensions> base_index, range<dimensions> sub_range);

	// Creates and initializes buffer from the OpenCL memory object.
	// The SYCL system may copy the data to another device and/or context,
	// but must copy it back (if modified) at the point of destruction of the buffer.
	// The memory object is assumed to only be available to the SYCL scheduler after the event has signaled
	// and is assumed to be currently resident on the context and device signified by the queue.
	buffer(cl_mem mem_object, queue from_queue, event available_event);

	range<dimensions> get_range() {
		return rang;
	}

	// Total number of elements in the buffer
	size_t get_count();

	// Total number of bytes in the buffer
	size_t get_size();

	template<access::mode mode, access::target target = access::global_buffer>
	accessor<DataType, dimensions, mode, target> get_access() {
		DSELF() << "not implemented";
	}
};

} // namespace detail

// Defines a shared array that can be used by kernels in queues and has to be accessed using accessor classes.
template <typename DataType, int dimensions = 1>
struct buffer : public detail::buffer<DataType, dimensions> {
#if MSVC_LOW
	buffer(range<dimensions> range)
		: detail::buffer<DataType, dimensions>(range) {}
	buffer(DataType* host_data, range<dimensions> range)
		: detail::buffer<DataType, dimensions>(host_data, range) {}
	//buffer(storage<DataType> &store, range<dimensions>);
	//buffer(buffer, index<dimensions> base_index, range<dimensions> sub_range);
	buffer(cl_mem mem_object, queue from_queue, event available_event)
		: detail::buffer<DataType, dimensions>(mem_object, from_queue, available_event) {}
#else
	using detail::buffer<DataType, dimensions>::buffer;
#endif
};

template <typename DataType>
struct buffer<DataType, 1> : public detail::buffer<DataType, 1> {
#if MSVC_LOW
	buffer(range<1> range)
		: detail::buffer<DataType, 1>(range) {}
	buffer(DataType* host_data, range<1> range)
		: detail::buffer<DataType, 1>(host_data, range) {}
	//buffer(storage<DataType> &store, range<1>);
	//buffer(buffer, index<dimensions> base_index, range<dimensions> sub_range);
	buffer(cl_mem mem_object, queue from_queue, event available_event)
		: detail::buffer<DataType, 1>(mem_object, from_queue, available_event) {}
#else
	using detail::buffer<DataType, 1>::buffer;
#endif
	// Construction via copy constructor from any structure that has contiguous storage.
	// The original structure or class does not get updated by the buffer after the processing.
	// The buffer allocates a new host storage object,
	// where the values of the the copy constructor are used as an initialization.
	buffer(DataType* startIterator, DataType* endIterator);

	// TODO: Used by the Codeplay SYCL example
	buffer(VECTOR_CLASS<DataType> host_data)
		: detail::buffer<DataType, 1>(host_data.data(), host_data.size()) {
		DSELF() << "not implemented";
	}
};

} // namespace sycl
} // namespace cl
