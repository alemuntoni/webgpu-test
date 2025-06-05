#ifndef APPLICATION_H
#define APPLICATION_H

#include "webgpu-utils.h"

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.hpp>

#include <iostream>

class Application
{
    // We put here all the variables that are shared between init and main loop
    // All these can be initialized to nullptr
    GLFWwindow*    m_window   = nullptr;
    wgpu::Instance m_instance = nullptr; // NEW
    wgpu::Device   m_device   = nullptr; // NEW
    wgpu::Queue    m_queue    = nullptr; // NEW
    wgpu::Surface  m_surface  = nullptr; // NEW

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
        m_instance = wgpu::createInstance();

        // Get adapter
        std::cout << "Requesting adapter..." << std::endl;
        m_surface = glfwCreateWindowWGPUSurface(m_instance, m_window);

        wgpu::RequestAdapterOptions adapterOpts = wgpu::Default;
        adapterOpts.compatibleSurface = m_surface;
        //                              ^^^^^^^^^ Use the surface here

        wgpu::Adapter adapter = requestAdapterSync(m_instance, &adapterOpts);
        std::cout << "Got adapter: " << adapter << std::endl;

        std::cout << "Requesting device..." << std::endl;
        wgpu::DeviceDescriptor deviceDesc = wgpu::Default;
        // Any name works here, that's your call
        deviceDesc.label = wgpu::StringView("My Device");
        std::vector<wgpu::FeatureName> features;
        // No required feature for now
        deviceDesc.requiredFeatureCount = features.size();
        deviceDesc.requiredFeatures     = (WGPUFeatureName*)features.data();
        // Make sure 'features' lives until the call to
        // wgpuAdapterRequestDevice!
        wgpu::Limits requiredLimits = wgpu::Default;
        // We leave 'requiredLimits' untouched for now
        deviceDesc.requiredLimits = &requiredLimits;
        // Make sure that the 'requiredLimits' variable lives until the call to
        // wgpuAdapterRequestDevice!
        deviceDesc.defaultQueue.label = wgpu::StringView("The Default Queue");
        auto onDeviceLost             = [](const WGPUDevice*     device,
                               WGPUDeviceLostReason  reason,
                               struct WGPUStringView message,
                               void* /* userdata1 */,
                               void* /* userdata2 */
                            ) {
            // All we do is display a message when the device is lost
            std::cout << "Device " << device << " was lost: reason " << reason
                      << " (" << wgpu::StringView(message) << ")" << std::endl;
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
                      << type << " (" << wgpu::StringView(message) << ")"
                      << std::endl;
        };
        deviceDesc.uncapturedErrorCallbackInfo.callback = onDeviceError;
        // NB: 'device' is now declared at the class level
        m_device = requestDeviceSync(m_instance, adapter, &deviceDesc);
        std::cout << "Got device: " << m_device << std::endl;

        // The variable 'queue' is now declared at the class level
        // (do NOT prefix this line with 'WGPUQueue' otherwise it'd shadow the
        // class attribute)
        m_queue = m_device.getQueue();

        wgpu::SurfaceConfiguration config = wgpu::Default;

        // Configuration of the textures created for the underlying swap chain
        config.width = 640;
        config.height = 480;
        config.device = m_device;
        // We initialize an empty capability struct:
        wgpu::SurfaceCapabilities capabilities = wgpu::Default;

        // We get the capabilities for a pair of (surface, adapter).
        // If it works, this populates the `capabilities` structure
        wgpu::Status status = m_surface.getCapabilities(adapter, &capabilities);
        if (status != wgpu::Status::Success) {
            return false;
        }

        // From the capabilities, we get the preferred format: it is always the first one!
        // (NB: There is always at least 1 format if the GetCapabilities was successful)
        config.format = capabilities.formats[0];

        // We no longer need to access the capabilities, so we release their memory.
        capabilities.freeMembers();
        config.presentMode = wgpu::PresentMode::Fifo;
        config.alphaMode = wgpu::CompositeAlphaMode::Auto;

        m_surface.configure(config); // NEW

        // We no longer need to access the adapter
        adapter.release();

        return true;
    }

    // Uninitialize everything that was initialized
    void terminate()
    {
        m_surface.unconfigure();
        m_queue.release();
        m_surface.release();
        m_device.release();
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    // Draw a frame and handle events
    void mainLoop()
    {
        glfwPollEvents();
        m_instance.processEvents();

        wgpu::TextureView targetView = getNextSurfaceView();
        if (!targetView) return; // no surface texture, we skip this frame
        wgpu::CommandEncoderDescriptor encoderDesc = wgpu::Default;
        encoderDesc.label = wgpu::StringView("My command encoder");
        wgpu::CommandEncoder encoder = m_device.createCommandEncoder(encoderDesc);
        wgpu::RenderPassDescriptor renderPassDesc = wgpu::Default;
        wgpu::RenderPassColorAttachment colorAttachment = wgpu::Default;

        colorAttachment.view = targetView;
        colorAttachment.loadOp = wgpu::LoadOp::Clear;
        colorAttachment.storeOp = wgpu::StoreOp::Store;
        colorAttachment.clearValue = wgpu::Color{ 0.5, 0.5, 0.5, 1.0 };

        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;

        wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
        // Use the render pass here (we do nothing with the render pass for now)
        renderPass.end();
        renderPass.release();
        wgpu::CommandBufferDescriptor cmdBufferDescriptor = wgpu::Default;
        cmdBufferDescriptor.label = wgpu::StringView("Command buffer");
        wgpu::CommandBuffer command = encoder.finish(cmdBufferDescriptor);
        encoder.release();

        // Finally submit the command queue
        m_queue.submit(command);
        command.release();
        // At the end of the frame
        targetView.release();
#ifndef __EMSCRIPTEN__
        wgpuSurfacePresent(m_surface);
#endif
    }

    // Return true as long as the main loop should keep on running
    bool isRunning() const { return !glfwWindowShouldClose(m_window); }

private:
    wgpu::TextureView getNextSurfaceView()
    {
        wgpu::SurfaceTexture surfaceTexture = wgpu::Default;
        m_surface.getCurrentTexture(&surfaceTexture);
        if (
            surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
            surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal
            ) {
            return nullptr;
        }
        wgpu::TextureViewDescriptor viewDescriptor = wgpu::Default;
        viewDescriptor.label = wgpu::StringView("Surface texture view");
        viewDescriptor.dimension = wgpu::TextureViewDimension::_2D; // not to confuse with 2DArray
        wgpu::TextureView targetView = wgpu::Texture(surfaceTexture.texture).createView(viewDescriptor);
        // We no longer need the texture, only its view,
        // so we release it at the end of GetNextSurfaceViewData
        wgpu::Texture(surfaceTexture.texture).release();
        return targetView;
    }
};

#endif // APPLICATION_H
