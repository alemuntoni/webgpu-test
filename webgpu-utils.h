#ifndef WEBGPU_UTILS_H
#define WEBGPU_UTILS_H

#include <webgpu/webgpu.h>

#include <string_view>

/**
 * Convert a WebGPU string view into a C++ std::string_view.
 */
std::string_view toStdStringView(WGPUStringView wgpuStringView);

/**
 * Sleep for a given number of milliseconds.
 * This works with both native builds and emscripten, provided that -sASYNCIFY
 * compile option is provided when building with emscripten.
 */
void sleepForMilliseconds(unsigned int milliseconds);

/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapter(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const * options);

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUAdapter device = requestDevice(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
WGPUDevice requestDeviceSync(WGPUInstance instance, WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor);

/**
 * An example of how we can inspect the capabilities of the hardware through
 * the adapter object.
 */
void inspectAdapter(WGPUAdapter adapter);
/**
 * Display information about a device
 */
void inspectDevice(WGPUDevice device);
#include <functional>

/**
 * Fetch data from a GPU buffer back to the CPU.
 * This function blocks until the data is available on CPU, then calls the
 * `processBufferData` callback, and finally unmap the buffer.
 */
void fetchBufferDataSync(
    WGPUInstance instance,
    WGPUBuffer buffer,
    std::function<void(const void*)> processBufferData
    );
/**
 * Divides p / q and ceil up to the next integer value
 */
uint32_t divideAndCeil(uint32_t p, uint32_t q);

#endif // WEBGPU_UTILS_H
