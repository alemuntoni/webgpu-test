#ifndef APPLICATION_H
#define APPLICATION_H

#include "webgpu-utils.h"

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#include <iostream>

class Application
{
    // We put here all the variables that are shared between init and main loop
    // All these can be initialized to nullptr
    GLFWwindow*  m_window   = nullptr;
    WGPUInstance m_instance = nullptr;
    WGPUDevice   m_device   = nullptr;
    WGPUQueue    m_queue    = nullptr;
    WGPUSurface  m_surface  = nullptr;

public:
    // Initialize everything and return true if it went all right
    bool initialize()
    {
        // Open window
        glfwInit();
        glfwWindowHint(
            GLFW_CLIENT_API,
            GLFW_NO_API); // <-- extra info for glfwCreateWindow
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);

        // Create instance ('instance' is now declared at the class level)
        m_instance = wgpuCreateInstance(nullptr);

        // Get adapter
        std::cout << "Requesting adapter..." << std::endl;
        m_surface = glfwCreateWindowWGPUSurface(m_instance, m_window);

        WGPURequestAdapterOptions adapterOpts =
            WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
        adapterOpts.compatibleSurface = m_surface;
        //                              ^^^^^^^^^ Use the surface here

        WGPUAdapter adapter = requestAdapterSync(m_instance, &adapterOpts);
        std::cout << "Got adapter: " << adapter << std::endl;

        std::cout << "Requesting device..." << std::endl;
        WGPUDeviceDescriptor deviceDesc = WGPU_DEVICE_DESCRIPTOR_INIT;
        // Any name works here, that's your call
        deviceDesc.label = toWgpuStringView("My Device");
        std::vector<WGPUFeatureName> features;
        // No required feature for now
        deviceDesc.requiredFeatureCount = features.size();
        deviceDesc.requiredFeatures     = features.data();
        // Make sure 'features' lives until the call to
        // wgpuAdapterRequestDevice!
        WGPULimits requiredLimits = WGPU_LIMITS_INIT;
        // We leave 'requiredLimits' untouched for now
        deviceDesc.requiredLimits = &requiredLimits;
        // Make sure that the 'requiredLimits' variable lives until the call to
        // wgpuAdapterRequestDevice!
        deviceDesc.defaultQueue.label = toWgpuStringView("The Default Queue");
        auto onDeviceLost             = [](const WGPUDevice*     device,
                               WGPUDeviceLostReason  reason,
                               struct WGPUStringView message,
                               void* /* userdata1 */,
                               void* /* userdata2 */
                            ) {
            // All we do is display a message when the device is lost
            std::cout << "Device " << device << " was lost: reason " << reason
                      << " (" << toStdStringView(message) << ")" << std::endl;
        };
        deviceDesc.deviceLostCallbackInfo.callback = onDeviceLost;
        deviceDesc.deviceLostCallbackInfo.mode =
            WGPUCallbackMode_AllowProcessEvents;
        auto onDeviceError = [](const WGPUDevice*     device,
                                WGPUErrorType         type,
                                struct WGPUStringView message,
                                void* /* userdata1 */,
                                void* /* userdata2 */
                             ) {
            std::cout << "Uncaptured error in device " << device << ": type "
                      << type << " (" << toStdStringView(message) << ")"
                      << std::endl;
        };
        deviceDesc.uncapturedErrorCallbackInfo.callback = onDeviceError;
        // NB: 'device' is now declared at the class level
        m_device = requestDeviceSync(m_instance, adapter, &deviceDesc);
        std::cout << "Got device: " << m_device << std::endl;

        // The variable 'queue' is now declared at the class level
        // (do NOT prefix this line with 'WGPUQueue' otherwise it'd shadow the
        // class attribute)
        m_queue = wgpuDeviceGetQueue(m_device);

        WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;

        // Configuration of the textures created for the underlying swap chain
        config.width = 640;
        config.height = 480;
        config.device = m_device;
        // We initialize an empty capability struct:
        WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;

        // We get the capabilities for a pair of (surface, adapter).
        // If it works, this populates the `capabilities` structure
        WGPUStatus status = wgpuSurfaceGetCapabilities(m_surface, adapter, &capabilities);
        if (status != WGPUStatus_Success) {
            return false;
        }

        // From the capabilities, we get the preferred format: it is always the first one!
        // (NB: There is always at least 1 format if the GetCapabilities was successful)
        config.format = capabilities.formats[0];

        // We no longer need to access the capabilities, so we release their memory.
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
        config.presentMode = WGPUPresentMode_Fifo;
        config.alphaMode = WGPUCompositeAlphaMode_Auto;

        wgpuSurfaceConfigure(m_surface, &config);

        // We no longer need to access the adapter
        wgpuAdapterRelease(adapter);

        return true;
    }

    // Uninitialize everything that was initialized
    void terminate()
    {
        wgpuSurfaceUnconfigure(m_surface);
        wgpuQueueRelease(m_queue);
        wgpuSurfaceRelease(m_surface);
        wgpuDeviceRelease(m_device);
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    // Draw a frame and handle events
    void mainLoop()
    {
        glfwPollEvents();
        wgpuInstanceProcessEvents(m_instance);

        WGPUTextureView targetView = getNextSurfaceView();
        if (!targetView) return; // no surface texture, we skip this frame
        WGPUCommandEncoderDescriptor encoderDesc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
        encoderDesc.label = toWgpuStringView("My command encoder");
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoderDesc);
        WGPURenderPassDescriptor renderPassDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;
        WGPURenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;

        colorAttachment.view = targetView;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = WGPUColor{ 0.5, 0.5, 0.5, 1.0 };

        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;

        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        // Use the render pass here (we do nothing with the render pass for now)
        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);
        WGPUCommandBufferDescriptor cmdBufferDescriptor = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
        cmdBufferDescriptor.label = toWgpuStringView("Command buffer");
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
        wgpuCommandEncoderRelease(encoder); // release encoder after it's finished

        // Finally submit the command queue
        wgpuQueueSubmit(m_queue, 1, &command);
        wgpuCommandBufferRelease(command);
        // At the end of the frame
        wgpuTextureViewRelease(targetView);
#ifndef __EMSCRIPTEN__
        wgpuSurfacePresent(m_surface);
#endif
    }

    // Return true as long as the main loop should keep on running
    bool isRunning() const { return !glfwWindowShouldClose(m_window); }

private:
    WGPUTextureView getNextSurfaceView()
    {
        WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
        wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);
        if (
            surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
            surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal
            ) {
            return nullptr;
        }
        WGPUTextureViewDescriptor viewDescriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
        viewDescriptor.label = toWgpuStringView("Surface texture view");
        viewDescriptor.dimension = WGPUTextureViewDimension_2D; // not to confuse with 2DArray
        WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
        // We no longer need the texture, only its view,
        // so we release it at the end of GetNextSurfaceViewData
        wgpuTextureRelease(surfaceTexture.texture);
        return targetView;
    }
};

#endif // APPLICATION_H
