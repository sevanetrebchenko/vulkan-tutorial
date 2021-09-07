
// Standard includes.
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <set>
#include <algorithm>
#include <fstream>

#include "application.h"

namespace VT {

	Application::Application(int width, int height) : width_(width),
													  height_(height),
													  enableValidationLayers_(true)
													  {
	}

	Application::~Application() {
	}

	void Application::Run() {
		Initialize();
		Update();
		Shutdown();
	}

	VkBool32 Application::DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
		if (severity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			std::cerr << callbackData->pMessage << std::endl;
		}

		return VK_FALSE; // Returns true if the function call that triggered the layer should be aborted. Normally used to test the layers themselves.
	}

	void Application::Initialize() {
		InitializeGLFW();
		InitializeVKInstance();
		InitializeDebugMessenger();
		InitializeVulkanSurface();
		InitializePhysicalDevice();
		InitializeLogicalDevice();
		InitializeSwapChain();
		InitializeImageViews();
		InitializeGraphicsPipeline();
	}

	void Application::Update() {
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
		}
	}

	void Application::Shutdown() {
		vkDestroyPipeline(logicalDevice_, graphicsPipeline_, nullptr);
		vkDestroyPipelineLayout(logicalDevice_, pipelineLayout_, nullptr);
		vkDestroyRenderPass(logicalDevice_, renderPass_, nullptr);

		for (const VkImageView& imageView : swapChainImageViews_) {
		    vkDestroyImageView(logicalDevice_, imageView, nullptr);
		}

		vkDestroySwapchainKHR(logicalDevice_, swapChain_, nullptr); // Needs to be destroyed before the logical device.
		vkDestroyDevice(logicalDevice_, nullptr);

		if (enableValidationLayers_) {
		    auto function = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT");
		    if (!function) {
		        throw std::runtime_error("Failed to find function for destroying debug messenger.");
		    }

		    function(instance_, messenger_, nullptr);
		}

		vkDestroySurfaceKHR(instance_, surface_, nullptr); // Surface needs to be destroyed before instance.
		vkDestroyInstance(instance_, nullptr);

		glfwDestroyWindow(window_);
		glfwTerminate();
	}

	void Application::InitializeGLFW() {
		// Initialize GLFW.
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window_ = glfwCreateWindow(width_, height_, "Vulkan Tutorial", nullptr, nullptr);
	}

	void Application::InitializeVKInstance() {
		// Create Vulkan instance.
		VkApplicationInfo appInfo { };
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan Tutorial";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "N/A";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		// Tells the graphics driver which global extensions and validation layers to use.
		// Global (applies to the entire program).
		VkInstanceCreateInfo createInfo { };
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// Get supported extensions.
		std::vector<VkExtensionProperties> supportedExtensions;
		GetSupportedExtensions(supportedExtensions);

		if (supportedExtensions.empty()) {
		    throw std::runtime_error("No supported Vulkan extensions.");
		}

		// Get desired extensions.
		std::vector<const char*> desiredExtensions;
		GetDesiredInstanceExtensions(desiredExtensions);

		if (!CheckInstanceExtensions(supportedExtensions, desiredExtensions)) {
			throw std::runtime_error("Requested extensions not supported.");
		}

		createInfo.enabledExtensionCount = desiredExtensions.size();
		createInfo.ppEnabledExtensionNames = desiredExtensions.data();

		// Get supported validation layers.
		std::vector<const char*> validationLayerNames {
			"VK_LAYER_KHRONOS_validation"
		};

		if (enableValidationLayers_) {
			if (!CheckValidationLayers(validationLayerNames)) {
				throw std::runtime_error("Requested validation layers not available.");
			}

			validationLayerNames_ = validationLayerNames;

			VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo { };
			SetupDebugMessengerUtils(messengerCreateInfo);

			createInfo.enabledLayerCount = validationLayerNames_.size();
			createInfo.ppEnabledLayerNames = validationLayerNames_.data();
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)(&messengerCreateInfo); // Setup debug messaging during Vulkan instance creation.
		}
		else {
			createInfo.enabledLayerCount = 0; // Don't use any validation layers.
			createInfo.pNext = nullptr; // Don't log debug messages during instance creation.
		}

		// Create Vulkan instance.
		if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan instance.");
		}
	}

	void Application::InitializeDebugMessenger() {
		if (!enableValidationLayers_) {
			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT createInfo { };
		SetupDebugMessengerUtils(createInfo);

		auto function = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT");
		if (!function) {
			throw std::runtime_error("Failed to find function to create debug messenger.");
		}

		bool result = function(instance_, &createInfo, nullptr, &messenger_);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create debug messenger.");
		}
	}

	void Application::InitializePhysicalDevice() {
		std::vector<VkPhysicalDevice> physicalDevices;
		GetSupportedPhysicalDevices(physicalDevices);

		// Check found devices for compatibility.
		if (!CheckPhysicalDevices(physicalDevices)) {
			throw std::runtime_error("Failed to find compatible physical device.");
		}
	}

	void Application::InitializeLogicalDevice() {
		// Queues are something you submit command buffers to.
		// Create structure to hold data for all the desired queue families.
		QueueFamilyIndexData& indexData = physicalDeviceData_.queueIndices_;
		std::set<unsigned> uniqueQueueFamilies = {
            indexData.graphicsFamily_.value(),
            indexData.presentationFamily_.value()
		};
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		float queuePriority = 1.0f;
		for (unsigned queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo { };
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Create logical device.
		VkDeviceCreateInfo createInfo { };
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = queueCreateInfos.size();
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &physicalDeviceData_.deviceFeatures;

		// Enable swapchain.
		createInfo.ppEnabledExtensionNames = physicalDeviceData_.deviceExtensions_.data();
		createInfo.enabledExtensionCount = physicalDeviceData_.deviceExtensions_.size();

		if (enableValidationLayers_) {
			// No distinction between device and instance validation layer specification on up to date Vulkan applications.
			// Specified for compatibility.
			createInfo.enabledLayerCount = validationLayerNames_.size();
			createInfo.ppEnabledLayerNames = validationLayerNames_.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDeviceData_.physicalDevice_, &createInfo, nullptr, &logicalDevice_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical device.");
		}

		// Since only one queue of each type is being used, index is 0.
		unsigned graphicsQueueIndex = 0;
		unsigned presentationQueueIndex = 0;

		// Query the location of the graphics queue to use for rendering.
		vkGetDeviceQueue(logicalDevice_, physicalDeviceData_.queueIndices_.graphicsFamily_.value(), graphicsQueueIndex, &graphicsQueue_);

		// Query the location of the presentation queue to be able to present images to the surface.
		vkGetDeviceQueue(logicalDevice_, physicalDeviceData_.queueIndices_.presentationFamily_.value(), presentationQueueIndex, &presentationQueue_);
	}

	void Application::InitializeVulkanSurface() {
		if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan window surface.");
		}
	}

	void Application::InitializeSwapChain() {
	    SwapChainSupportData swapChainSupportData = QuerySwapChainSupport(physicalDeviceData_.physicalDevice_);

	    VkSurfaceFormatKHR surfaceFormat = ChooseSwapChainSurfaceFormat(swapChainSupportData.formats_);
	    VkPresentModeKHR presentationMode = ChooseSwapChainPresentationMode(swapChainSupportData.presentationModes_);
	    VkExtent2D extent = ChooseSwapChainExtent(swapChainSupportData.surfaceCapabilities_);

        // Request 1 more than minimum to prevent waiting for driver to complete operations before acquiring an image to render to.
        unsigned swapChainImageCount = swapChainSupportData.surfaceCapabilities_.minImageCount + 1;
        swapChainImageCount = std::clamp(swapChainImageCount, swapChainImageCount, swapChainSupportData.surfaceCapabilities_.maxImageCount); // maxImageCount = 0 means no maximum number of images.

        // Create swapchain.
        VkSwapchainCreateInfoKHR createInfo { };
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface_;

        createInfo.minImageCount = swapChainImageCount; // Implementation may create more images, since only the minimum is specified.
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1; // Specifies number of layers each image consists of.
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Color render attachment.

        QueueFamilyIndexData& indexData = physicalDeviceData_.queueIndices_; // Guaranteed to exist due to requirements from physical device construction.
        unsigned graphicsFamilyQueueIndex = indexData.graphicsFamily_.value();
        unsigned presentationFamilyQueueIndex = indexData.presentationFamily_.value();

        unsigned queueFamilyIndexData[] = { graphicsFamilyQueueIndex, presentationFamilyQueueIndex };
        if (graphicsFamilyQueueIndex != presentationFamilyQueueIndex) {
            // Separate queues used for graphics and presentation.
            // Drawing images on the graphics queue, submitting images on the presentation queue.
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Images can be used across multiple queues without explicit ownership transfer.
            // TODO: Look into ownership transferring, as this option offers better performance.

            createInfo.queueFamilyIndexCount = 2; // Number of queue families that have access to swapchain images with VK_SHARING_MODE_CONCURRENT.
            createInfo.pQueueFamilyIndices = queueFamilyIndexData; // Which queue families have access.
        }
        else {
            // Same queue used for garphics and presentation.
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Images are owned by one family queue at a time an ownership must be transferred.
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupportData.surfaceCapabilities_.currentTransform; // Transformation pre-applied to all swapchain images.
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Blending with other images.
        createInfo.presentMode = presentationMode;
        createInfo.clipped = VK_TRUE; // Color of obscured pixels does not matter.
        createInfo.oldSwapchain = nullptr; // Swapping between different swapchains requires the old one to be stored.

        if (vkCreateSwapchainKHR(logicalDevice_, &createInfo, nullptr, &swapChain_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain.");
        }

        // Register final format and extent.
        swapChainImageFormat_ = surfaceFormat.format;
        swapChainExtent_ = extent;

        // Query the number of created images in the swapchain. Implementation may create more images, since only the minimum is specified.
        unsigned imageCount = 0;
        if (vkGetSwapchainImagesKHR(logicalDevice_, swapChain_, &imageCount, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get number of images in swapchain.");
        }
        swapChainImages_.resize(imageCount);

        // Get images.
        if (vkGetSwapchainImagesKHR(logicalDevice_, swapChain_, &imageCount, swapChainImages_.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get swapchain images.");
        }
	}

	void Application::InitializeImageViews() {
	    unsigned numImages = swapChainImages_.size();

        // VkImageView objects describe how to access VkImage objects and which parts to access.
        swapChainImageViews_.resize(numImages);

        for (int i = 0; i < numImages; ++i) {
            VkImageViewCreateInfo createInfo { };
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages_[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Treat the image as a 2D image.
            createInfo.format = swapChainImageFormat_;

            // Use default color channel configuration.
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // Base images without any resource mipmapping levels / multiple layers.
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(logicalDevice_, &createInfo, nullptr, &swapChainImageViews_[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create swap chain image views.");
            }
        }
	}

	void Application::InitializeGraphicsPipeline() {
	    // Shader initialization.
	    std::vector<char> vertexShaderBinary = ReadFile("assets/shaders/bin/triangle_vert.spv");
	    std::vector<char> fragmentShaderBinary = ReadFile("assets/shaders/bin/triangle_frag.spv");

	    VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderBinary);
	    VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderBinary);

	    // Vertex shader.
	    VkPipelineShaderStageCreateInfo vertexShaderStageInfo { };
	    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Tell Vulkan what stage of the pipeline this module is used for.
	    vertexShaderStageInfo.module = vertexShaderModule;
	    vertexShaderStageInfo.pName = "main"; // Function to invoke.
	    vertexShaderStageInfo.pSpecializationInfo = nullptr; // Used to specify shader compile time constants, which is more efficient
	    // because the graphics pipeline can eliminate branching based on these constants
	    // at pipeline creation time (rather than render/runtime).

	    // Fragment shader.
	    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo { };
	    fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	    fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	    fragmentShaderStageInfo.module = fragmentShaderModule;
	    fragmentShaderStageInfo.pName = "main";
	    fragmentShaderStageInfo.pSpecializationInfo = nullptr;

	    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

	    // Vertex data.
	    // Provides details about the format of the vertex data passed to the fragment shader.
	    VkPipelineVertexInputStateCreateInfo vertexInputInfo { };
	    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	    vertexInputInfo.vertexBindingDescriptionCount = 0; // Spacing between the data, whether data is per vertex or per instance.
	    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Types of attributes passed to the vertex shader, which bindings, which offsets to load from.
	    vertexInputInfo.vertexAttributeDescriptionCount = 0;
	    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	    // Input assembly.
	    // Specifies what kind of geometry will be drawn from the provided vertices.
	    VkPipelineInputAssemblyStateCreateInfo inputAssembly { };
	    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	    // VK_PRIMITIVE_TOPOLOGY_POINT_LIST - Points from vertices.
	    // VK_PRIMITIVE_TOPOLOGY_LINE_LIST - Line from every 2 vertices without reuse.
	    // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP - The end vertex of every line is used as start vertex for the next line (index buffer).
	    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST - Triangle from every 3 vertices without reuse.
	    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP - The second and third vertex of every triangle are used as first two vertices of the next triangle (index buffer).
	    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	    inputAssembly.primitiveRestartEnable = VK_FALSE; // VK_TRUE allows breaking up line/triangle topology in the _STRIP topology modes.

	    // Viewport.
	    VkViewport viewport { };
	    viewport.x = 0.0f;
	    viewport.y = 0.0f;
	    viewport.width = static_cast<float>(swapChainExtent_.width);
	    viewport.height = static_cast<float>(swapChainExtent_.height);
	    viewport.minDepth = 0.0f;
	    viewport.maxDepth = 1.0f;

	    // Scissor.
	    // Specifies the region of the framebuffer in which pixels will actually be stored. Pixels outside of this area are discarded by the rasterizer.
	    VkRect2D scissor { };
	    scissor.offset = {0, 0};
	    scissor.extent = swapChainExtent_;

	    VkPipelineViewportStateCreateInfo viewportState { };
	    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	    viewportState.viewportCount = 1;
	    viewportState.pViewports = &viewport; // Possible to use multiple viewports to render to - must be supported on the GPU as an extension.
	    viewportState.scissorCount = 1;
	    viewportState.pScissors = &scissor;

	    // Rasterizer.
	    VkPipelineRasterizationStateCreateInfo rasterizer { };
	    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	    rasterizer.depthClampEnable = VK_FALSE; // Fragments with depth beyond the near/far planes are clamped rather than discarded.
	    rasterizer.rasterizerDiscardEnable = VK_FALSE; // Geometry never passes the rasterizer (enable for disabling output to the framebuffer).

	    // VK_POLYGON_MODE_FILL - Fill the area of the polygon with fragments (default).
	    // VK_POLYGON_MODE_LINE - Polygon edges are drawn as lines (wireframe, GPU extension required).
	    // VK_POLYGON_MODE_POINT Polygon vertices are drawn as points (GPU extension required).
	    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	    rasterizer.lineWidth = 1.0f;
	    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // Back-face culling.
	    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // Vertex ordering to appear front-facing.
	    rasterizer.depthBiasEnable = VK_FALSE;
	    rasterizer.depthBiasConstantFactor = 0.0f;
	    rasterizer.depthBiasClamp = 0.0f;
	    rasterizer.depthBiasSlopeFactor = 0.0f;

	    // Multisampling.
	    // Possible to enable multisampling (running fragment shader on the same fragment multiple times), requires GPU extension.
	    VkPipelineMultisampleStateCreateInfo multisampling { };
	    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	    multisampling.sampleShadingEnable = VK_FALSE;
	    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	    multisampling.minSampleShading = 1.0f;
	    multisampling.pSampleMask = nullptr;
	    multisampling.alphaToCoverageEnable = VK_FALSE;
	    multisampling.alphaToOneEnable = VK_FALSE;

	    // Depth/Stencil buffering.
	    // TODO:

	    // Color blending.
	    // Configured per attached framebuffer.
	    VkPipelineColorBlendAttachmentState colorBlendAttachment { };
	    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; // Determines which channels to pass.
	    colorBlendAttachment.blendEnable = VK_FALSE; // Color will be passed through the pipeline unmodified.
	    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	    // Global color blending settings.
	    VkPipelineColorBlendStateCreateInfo colorBlending { };
	    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	    colorBlending.logicOpEnable = VK_FALSE;
	    colorBlending.logicOp = VK_LOGIC_OP_COPY;
	    colorBlending.attachmentCount = 1;
	    colorBlending.pAttachments = &colorBlendAttachment;
	    colorBlending.blendConstants[0] = 0.0f;
	    colorBlending.blendConstants[1] = 0.0f;
	    colorBlending.blendConstants[2] = 0.0f;
	    colorBlending.blendConstants[3] = 0.0f;

	    // Dynamic state.
	    VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,  // Viewport size.
            VK_DYNAMIC_STATE_LINE_WIDTH // Line width.
	    };

	    VkPipelineDynamicStateCreateInfo dynamicState { };
	    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	    dynamicState.dynamicStateCount = 2;
	    dynamicState.pDynamicStates = dynamicStates;

	    // Pipeline layout.
	    VkPipelineLayoutCreateInfo pipelineLayoutInfo { };
	    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	    pipelineLayoutInfo.setLayoutCount = 0;
	    pipelineLayoutInfo.pSetLayouts = nullptr;
	    pipelineLayoutInfo.pushConstantRangeCount = 0; // Push constants are another way of passing dynamic values into shaders (similar to uniforms).
	    pipelineLayoutInfo.pPushConstantRanges = nullptr;

	    // Pipeline is referenced throughout the lifetime of the program.
	    if (vkCreatePipelineLayout(logicalDevice_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
	        throw std::runtime_error("Failed to create pipeline layout.");
	    }


	    // Create rendering passes.

	    // Single color attachment from one of the images in the swap chain.
	    VkAttachmentDescription colorAttachment { };
	    colorAttachment.format = swapChainImageFormat_;
	    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling.

	    // Applies to color and depth data:

	    // LoadOp: what to do with the data in the attachment before rendering.
	    // VK_ATTACHMENT_LOAD_OP_LOAD - Preserve the existing contents of the attachment.
	    // VK_ATTACHMENT_LOAD_OP_CLEAR - Clear the values to a constant at the start.
	    // VK_ATTACHMENT_LOAD_OP_DONT_CARE - Existing contents are undefined; we don't care about them.

	    // Contents will be cleared to black before rendering.
	    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

	    // StoreOp: what to do with the data in the attachment after rendering.
	    // VK_ATTACHMENT_STORE_OP_STORE - Rendered contents will be stored in memory and can be read later.
	    // VK_ATTACHMENT_STORE_OP_DONT_CARE - Contents of the framebuffer will be undefined after the rendering operation.

	    // Rendering to the screen is going to happen, contents must be valid in order to be displayed.
	    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	    // Applies to stencil data:
	    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	    // Layouts of pixels in image memory. Images need to be transitioned to specific layouts in order to support the desired operations.
	    // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL - Images used as color attachment.
	    // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR - Images to be presented in the swap chain.
	    // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL - Images to be used as destination for a memory copy operation.
	    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Specifies state of the image before rendering begins.
	    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Specifies state of the image after rendering ends.

	    // Information about the above attachment.
	    VkAttachmentReference colorAttachmentRef { };
	    colorAttachmentRef.attachment = 0; // Only 1 attachment - index 0. Shaders reference this index to determine which attachment to output fragment color data to.
	    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Color attachment.

	    // Subpasses.
	    // Subpasses allow for multiple passes on the same framebuffer.
	    VkSubpassDescription subpass { };
	    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	    subpass.colorAttachmentCount = 1;
	    subpass.pColorAttachments = &colorAttachmentRef;
	    subpass.pInputAttachments = nullptr; // Attachments read from a shader.
	    subpass.pResolveAttachments = nullptr; // Multisampled color attachments.
	    subpass.pDepthStencilAttachment = nullptr; // Depth/stencil data attachments.
	    subpass.pPreserveAttachments = nullptr; // Attachments not used by this subpass, but for which data must be preserved.

	    // Render pass.
	    VkRenderPassCreateInfo renderPassInfo { };
	    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	    renderPassInfo.attachmentCount = 1;
	    renderPassInfo.pAttachments = &colorAttachment;
	    renderPassInfo.subpassCount = 1;
	    renderPassInfo.pSubpasses = &subpass;

	    if (vkCreateRenderPass(logicalDevice_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
	        throw std::runtime_error("Failed to create render pass.");
	    }

	    // Create graphics pipeline.
	    VkGraphicsPipelineCreateInfo pipelineInfo { };
	    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	    pipelineInfo.stageCount = 2;
	    pipelineInfo.pStages = shaderStages; // Shader stages.
	    // pipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;

	    // Fixed function state.
	    pipelineInfo.pVertexInputState = &vertexInputInfo;
	    pipelineInfo.pInputAssemblyState = &inputAssembly;
	    pipelineInfo.pViewportState = &viewportState;
	    pipelineInfo.pRasterizationState = &rasterizer;
	    pipelineInfo.pMultisampleState = &multisampling;
	    pipelineInfo.pDepthStencilState = nullptr;
	    pipelineInfo.pColorBlendState = &colorBlending;
	    pipelineInfo.pDynamicState = nullptr;

	    // Pipeline layout.
	    pipelineInfo.layout = pipelineLayout_;

	    // Render pass.
	    pipelineInfo.renderPass = renderPass_;
	    pipelineInfo.subpass = 0;

	    // Creating a derivative pipeline from a similar, already created one (for performance reasons).
	    // Requires VK_PIPELINE_CREATE_DERIVATIVE_BIT to be set in VkGraphicsPipelineCreateInfo.
	    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	    pipelineInfo.basePipelineIndex = -1;

	    // Function is able to take multiple VkGraphicsPipelineCreateInfo objects to create multiple pipelines in one call.
	    // VkPipelineCache (second argument) stores and reuses data that is similar across multiple pipeline creations.
	    if (vkCreateGraphicsPipelines(logicalDevice_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_) != VK_SUCCESS) {
	        throw std::runtime_error("Failed to create graphics pipeline.");
	    }

	    // Compilation and linking of shader module bytecode into machine code does not happen until the graphics pipeline
	    // is created and initialized. Shader modules can be destroyed as soon as pipeline creation is finished.
	    vkDestroyShaderModule(logicalDevice_, fragmentShaderModule, nullptr);
	    vkDestroyShaderModule(logicalDevice_, vertexShaderModule, nullptr);
	}

	// Assumes messenger info has been initialized.
	void Application::SetupDebugMessengerUtils(VkDebugUtilsMessengerCreateInfoEXT& messengerInfo) {
		messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		messengerInfo.pfnUserCallback = DebugMessageCallback;
		messengerInfo.pUserData = nullptr; // No user data.
	}

	void Application::GetSupportedExtensions(std::vector<VkExtensionProperties>& extensionData) {
		unsigned extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); // Get the number of extensions.

		if (extensionCount != 0) {
		    extensionData.resize(extensionCount);

		    // Get extension data.
		    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionData.data()) != VK_SUCCESS) {
		        throw std::runtime_error("Failed to get instance extension properties.");
		    }
		}
	}

	void Application::GetDesiredInstanceExtensions(std::vector<const char*>& extensions) {
		unsigned glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		extensions = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers_) {
			// Emplace debug logger extension.
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
	}

	void Application::GetDesiredPhysicalDeviceExtensions(std::vector<const char*>& deviceExtensions) {
	    // Push back desire for swap chain.
	    // Swap chain is a queue of images that are waiting to be presented on the screen.
	    // Application grabs an image, renders it, then returns it to the queue.
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}

	void Application::GetSupportedPhysicalDevices(std::vector<VkPhysicalDevice>& deviceData) {
		unsigned numDevices = 0;

		// Query number of available physical devices.
		if (vkEnumeratePhysicalDevices(instance_, &numDevices, nullptr) != VK_SUCCESS) {
		    throw std::runtime_error("Failed to get number of available physical devices.");
		}

		if (numDevices <= 0) {
			throw std::runtime_error("No available physical devices for Vulkan to work with.");
		}

		deviceData = std::vector<VkPhysicalDevice>(numDevices);
		if (vkEnumeratePhysicalDevices(instance_, &numDevices, deviceData.data()) != VK_SUCCESS) {
		    throw std::runtime_error("Failed to get available physical devices.");
		}
	}

	void Application::GetSupportedQueueFamilies(const VkPhysicalDevice& physicalDevice, std::vector<VkQueueFamilyProperties>& queueData) {
		unsigned numQueueFamilies = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, nullptr);

		if (numQueueFamilies != 0) {
		    queueData = std::vector<VkQueueFamilyProperties>(numQueueFamilies);
		    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, queueData.data());
		}
	}

	void Application::GetSupportedPhysicalDeviceExtensions(const VkPhysicalDevice& physicalDevice, std::vector<VkExtensionProperties>& extensions) {
		unsigned numExtensions = 0;
		if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExtensions, nullptr) != VK_SUCCESS) {
		    throw std::runtime_error("Failed to get number of logical devices.");
		}

		if (numExtensions != 0) {
		    extensions = std::vector<VkExtensionProperties>(numExtensions);
		    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExtensions, extensions.data()) != VK_SUCCESS) {
		        throw std::runtime_error("Failed to get logical device extension properties.");
		    }
		}
	}

	void Application::GetSupportedSurfaceFormats(const VkPhysicalDevice &physicalDevice, std::vector<VkSurfaceFormatKHR> &surfaceFormatData) {
        unsigned numSurfaceFormats = 0;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface_, &numSurfaceFormats, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get number of physical device surface formats.");
        }

        if (numSurfaceFormats != 0) {
            surfaceFormatData = std::vector<VkSurfaceFormatKHR>(numSurfaceFormats);
            if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface_, &numSurfaceFormats, surfaceFormatData.data()) != VK_SUCCESS) {
                throw std::runtime_error("Failed to get physical device surface formats.");
            }
        }
	}

	void Application::GetSupportedPresentationModes(VkPhysicalDevice const &physicalDevice, std::vector<VkPresentModeKHR> &presentationModeData) {
	    unsigned numPresentationModes = 0;
	    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface_, &numPresentationModes, nullptr) != VK_SUCCESS) {
	        throw std::runtime_error("Failed to get number of physical device presentation modes.");
	    }

	    if (numPresentationModes != 0) {
	        presentationModeData = std::vector<VkPresentModeKHR>(numPresentationModes);
	        if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface_, &numPresentationModes, presentationModeData.data()) != VK_SUCCESS) {
	            throw std::runtime_error("Failed to get physical device presentation modes.");
	        }
	    }
	}

	bool Application::CheckInstanceExtensions(const std::vector<VkExtensionProperties>& supportedExtensions, const std::vector<const char*>& desiredExtensions) {
		// Verify extensions exist with supported extensions.
		bool complete = true;

		for (const char* desiredExtension : desiredExtensions) {
			bool found = false;

			for (const VkExtensionProperties& supportedExtension : supportedExtensions) {
				if (strcmp(desiredExtension, supportedExtension.extensionName) == 0) {
					found = true;
					break;
				}
			}

			if (!found) {
				complete = false;
				std::cout << "Encountered unsupported instance extension: " << desiredExtension << std::endl;
			}
		}

		return complete;
	}

	bool Application::CheckValidationLayers(std::vector<const char*>& validationLayerNames) {
		// Get the number of validation layers
		unsigned numValidationLayers = 0;
		if (vkEnumerateInstanceLayerProperties(&numValidationLayers, nullptr)) {
		    throw std::runtime_error("Failed to get number of instance layer properties.");
		}

		// Get validation layer objects.
		std::vector<VkLayerProperties> availableValidationLayers(numValidationLayers);
		if (vkEnumerateInstanceLayerProperties(&numValidationLayers, availableValidationLayers.data()) != VK_SUCCESS) {
		    throw std::runtime_error("Failed to get instance layer properties.");
		}

		bool complete = true;

		// Compare supported validation layers with desired.
		for (const char* desired : validationLayerNames) {
			bool found = false;

			for (const VkLayerProperties& layerProperties : availableValidationLayers) {
				if (strcmp(desired, layerProperties.layerName) == 0) {
					found = true;
					break;
				}
			}

			if (!found) {
				// Not all desired layers are supported.
				complete = false;
				std::cout << "Encountered unsupported validation layer: " << desired << std::endl;
			}
		}

		return complete;
	}

	bool Application::CheckPhysicalDevices(const std::vector<VkPhysicalDevice>& physicalDevices) {
		bool found = false;

		for (const VkPhysicalDevice& physicalDevice : physicalDevices) {
			if (CheckPhysicalDevice(physicalDevice)) {
				found = true;
				break;
			}
		}

		return found;
	}

	bool Application::CheckPhysicalDevice(const VkPhysicalDevice& physicalDevice) {
		VkPhysicalDeviceProperties deviceProperties; // Used for basic device properties, like name, type, and supported Vulkan versions.
		VkPhysicalDeviceFeatures deviceFeatures; // Used for optional features like texture compression, 64-bit floats, and multi vieport rendering.

		// Query device info.
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		std::cout << "Found physical device: " << deviceProperties.deviceName << std::endl;

		QueueFamilyIndexData queueFamilyIndices = FindQueueFamilies(physicalDevice);

		// Check for physical device extensions.
		std::vector<const char*> desiredDeviceExtensions;
		GetDesiredPhysicalDeviceExtensions(desiredDeviceExtensions);

		std::vector<VkExtensionProperties> supportedDeviceExtensions;
		GetSupportedPhysicalDeviceExtensions(physicalDevice, supportedDeviceExtensions);

		bool extensionsSupported = CheckPhysicalDeviceExtensionSupport(physicalDevice, supportedDeviceExtensions, desiredDeviceExtensions);

		// Check for (adequate) support of the swap chain on the physical device.
		bool swapChainSupported = false;
		if (extensionsSupported) {
		    SwapChainSupportData swapChainSupportData = QuerySwapChainSupport(physicalDevice);
		    swapChainSupported = !swapChainSupportData.formats_.empty() && !swapChainSupportData.presentationModes_.empty();
		}

		if (queueFamilyIndices.IsComplete() && extensionsSupported && swapChainSupported) {
			// Found compatible device.
			physicalDeviceData_.physicalDevice_ = physicalDevice;
			physicalDeviceData_.deviceProperties = deviceProperties;
			physicalDeviceData_.deviceFeatures = deviceFeatures;
			physicalDeviceData_.queueIndices_ = queueFamilyIndices;
			physicalDeviceData_.deviceExtensions_ = desiredDeviceExtensions;

			return true;
		}

		return false;
	}

	bool Application::CheckPhysicalDeviceExtensionSupport(const VkPhysicalDevice& physicalDevice, const std::vector<VkExtensionProperties>& supportedDeviceExtensions, const std::vector<const char*>& desiredDeviceExtensions) {
		// Make sure all desired extensions are present in the device extensions.
		bool complete = true;

		for (const char* desiredExtension : desiredDeviceExtensions) {
			bool found = false;

			for (const VkExtensionProperties& deviceExtension : supportedDeviceExtensions) {
				if (strcmp(deviceExtension.extensionName, desiredExtension) == 0) {
					found = true;
					break;
				}
			}

			if (!found) {
				complete = false;
				std::cout << "Encountered unsupported physical device extension: " << desiredExtension << std::endl;
			}
		}

		return complete;
	}

	// Every operation in Vulkan requires commands to be submitted to a queue.
	// There are different families of queues, each used for a specific purpose.
	// This function checks for the existence of the required queue families and the location of the queue that supports graphical operations - the graphics queue.
	QueueFamilyIndexData Application::FindQueueFamilies(const VkPhysicalDevice& physicalDevice) {
		QueueFamilyIndexData queueFamilyIndices;

		// VkQueueFamilyProperties contains details about the queue family, including supported operations and maximum number of queues that can be created.
		std::vector<VkQueueFamilyProperties> queueData;
		GetSupportedQueueFamilies(physicalDevice, queueData);

		// Find queue family that supports graphics.
		for (unsigned i = 0; i < queueData.size(); ++i) {
			const VkQueueFamilyProperties& queueProperties = queueData[i];

			if (queueProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				// Found suitable device that supports graphics.
				queueFamilyIndices.graphicsFamily_ = i;

				VkBool32 presentationSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface_, &presentationSupport);

				if (presentationSupport) {
					queueFamilyIndices.presentationFamily_ = i;
					break;
				}
			}
		}

		return queueFamilyIndices;
	}

    SwapChainSupportData Application::QuerySwapChainSupport(const VkPhysicalDevice& physicalDevice) {
        SwapChainSupportData data;

        // Basic surface capabilities.
        // Surface is initialized at this point.
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface_, &data.surfaceCapabilities_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to get physical device surface capabilities.");
        }

        GetSupportedSurfaceFormats(physicalDevice, data.formats_);
        GetSupportedPresentationModes(physicalDevice, data.presentationModes_);

        return data;
    }

    std::vector<char> Application::ReadFile(const std::string &filename) {
	    std::ifstream file(filename.c_str(), std::ios::ate | std::ios::binary);

	    if (!file.is_open()) {
	        throw std::runtime_error("Failed to open shader file: " + filename);
	    }

	    std::streamsize fileSize = file.tellg();
	    std::vector<char> data(fileSize);

	    // Read data.
	    file.seekg(0);
	    file.read(data.data(), fileSize);

	    file.close();
	    return data;
	}

    VkSurfaceFormatKHR Application::ChooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
	    // VkSurfaceFormatKHR contains format and colorSpace members.
	    // Format - color channels and types.
	    // colorSpace - indicates color space

	    for (const VkSurfaceFormatKHR& surfaceFormat : availableFormats) {
	        if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
	            // Found desired SRGB color format + color space.
	            return surfaceFormat;
	        }
	    }

	    // Also possible to rank other available formats based on specification.
	    return availableFormats[0];
    }

    VkPresentModeKHR Application::ChooseSwapChainPresentationMode(const std::vector<VkPresentModeKHR> &availablePresentationModes) {
	    // VK_PRESENT_MODE_IMMEDIATE_KHR - Images submitted are transferred to the screen immediately. Can result in tearing.
	    // VK_PRESENT_MODE_FIFO_KHR - Swap chain is a queue (FIFO) that takes the next image from the front. Images are submitted to the back
	    //                            of the queue. If the queue is full, program has to wait (vsync).
	    // VK_PRESENT_MODE_FIFO_RELAXED_KHR - Same as above. If the queue is empty at the beginning of the refresh, submitted image gets
	    //                                    rendered immediately. Can result in tearing.
        // VK_PRESENT_MODE_MAILBOX_KHR - Same as #2, except when the queue is full, older images are replaced with newer ones (triple buffering).

        for (const VkPresentModeKHR& presentationMode : availablePresentationModes) {
            if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                // Attempt to find triple buffering mode.
                return presentationMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR; // Guaranteed to exist.
    }

    VkExtent2D Application::ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
	    // Special case: width and height of surfaceCapabilities is (unsigned)-1, which means application window size is not
	    // determined by swap chain size. Should not be modified.
	    if (surfaceCapabilities.currentExtent.width != UINT32_MAX && surfaceCapabilities.currentExtent.height != UINT32_MAX) {
            return surfaceCapabilities.currentExtent;
	    }

	    // Can configure the extent to the size we want (taking into account min/max constraints).
	    int width = -1;
	    int height = -1;

        glfwGetFramebufferSize(window_, &width, &height); // Framebuffer size gets window dimensions in pixels.
                                                          // GLFW uses screen coordinates that do not necessarily translate 1-1.
        VkExtent2D extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        extent.width = std::clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

        return extent;
    }

    VkShaderModule Application::CreateShaderModule(const std::vector<char> &shaderCode) {
	    VkShaderModuleCreateInfo createInfo { };
	    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	    createInfo.codeSize = shaderCode.size();
	    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

	    VkShaderModule shaderModule;
	    if (vkCreateShaderModule(logicalDevice_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
	        throw std::runtime_error("Failed to create shader module.");
	    }

	    return shaderModule;
    }

}
