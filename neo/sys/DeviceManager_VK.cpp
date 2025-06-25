/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
* Copyright (C) 2022 Stephen Pridham (id Tech 4x integration)
* Copyright (C) 2023-2024 Stephen Saunders (id Tech 4x integration)
* Copyright (C) 2023 Robert Beckebans (id Tech 4x integration)
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

// SRS - Can now enable PCH here due to updated nvrhi CMakeLists.txt that makes Vulkan-Headers private
#include <precompiled.h>
#pragma hdrstop

#include <string>
#include <queue>
#include <unordered_set>

#include "renderer/RenderCommon.h"
#include "framework/Common_local.h"
#include <sys/DeviceManager.h>

#include <nvrhi/vulkan.h>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#if defined(__APPLE__)
	#if defined( USE_MoltenVK )
		// SRS - Needed for using MoltenVK's advanced performance statistics
		#include <MoltenVK/mvk_private_api.h>
	#endif

	// SRS - Disable MoltenVK's Synchronous Queue Submits for better performance, and Metal Argument Buffers to avoid HiZ compute shader issues on Apple Silicon
	idCVar r_mvkSynchronousQueueSubmits( "r_mvkSynchronousQueueSubmits", "0", CVAR_BOOL | CVAR_INIT | CVAR_NEW, "Use MoltenVK's synchronous queue submits option." );
	idCVar r_mvkUseMetalArgumentBuffers( "r_mvkUseMetalArgumentBuffers", "0", CVAR_BOOL | CVAR_INIT | CVAR_NEW, "Use MoltenVK's Metal argument buffers option." );
	idCVar r_mvkUseMTLHeap( "r_mvkUseMTLHeap", "0", CVAR_INTEGER | CVAR_INIT | CVAR_NEW, "Use MoltenVK's MTLHeap option (may cause issues on AMD GPUs).", 0, 2 );
#endif
#include <nvrhi/validation.h>
#include <libs/optick/optick.h>

#if defined( USE_AMD_ALLOCATOR )
	#define VMA_IMPLEMENTATION
	#define VMA_STATIC_VULKAN_FUNCTIONS 0
	#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
	#include "vk_mem_alloc.h"

	VmaAllocator m_VmaAllocator = nullptr;

	idCVar r_vmaDeviceLocalMemoryMB( "r_vmaDeviceLocalMemoryMB", "256", CVAR_INTEGER | CVAR_INIT | CVAR_NEW, "Size of VMA allocation block for gpu memory." );
#endif

idCVar r_vkPreferFastSync( "r_vkPreferFastSync", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL | CVAR_NEW, "Prefer Fast Sync/no-tearing in place of VSync off/tearing" );

// Define the Vulkan dynamic dispatcher - this needs to occur in exactly one cpp file in the program.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#if defined(__APPLE__) && defined( USE_MoltenVK )
#if MVK_VERSION >= MVK_MAKE_VERSION( 1, 2, 9 ) && USE_OPTICK
static bool optickCapturing = false;

// SRS - Optick callback function for notification of state changes
static bool optickStateChangedCallback( Optick::State::Type state )
{
	switch( state )
	{
		case Optick::State::START_CAPTURE:
			optickCapturing = true;
			break;

		case Optick::State::STOP_CAPTURE:
		case Optick::State::CANCEL_CAPTURE:
			optickCapturing = false;
			break;

		default:
			break;
	}

	return true;
}
#endif
#endif

class DeviceManager_VK : public DeviceManager
{
public:
	[[nodiscard]] nvrhi::IDevice* GetDevice() const override
	{
		if( m_ValidationLayer )
		{
			return m_ValidationLayer;
		}

		return m_NvrhiDevice;
	}

	[[nodiscard]] nvrhi::GraphicsAPI GetGraphicsAPI() const override
	{
		return nvrhi::GraphicsAPI::VULKAN;
	}

	int GetGraphicsFamilyIndex() const override
	{
		return m_GraphicsQueueFamily;
	}

protected:
	bool CreateDeviceAndSwapChain() override;
	void DestroyDeviceAndSwapChain() override;

	void ResizeSwapChain() override
	{
		if( m_VulkanDevice )
		{
			destroySwapChain();
			createSwapChain();
		}
	}

	nvrhi::ITexture* GetCurrentBackBuffer() override
	{
		return m_SwapChainImages[m_SwapChainIndex].rhiHandle;
	}
	nvrhi::ITexture* GetBackBuffer( uint32_t index ) override
	{
		if( index < m_SwapChainImages.size() )
		{
			return m_SwapChainImages[index].rhiHandle;
		}
		return nullptr;
	}
	uint32_t GetCurrentBackBufferIndex() override
	{
		return m_SwapChainIndex;
	}
	uint32_t GetBackBufferCount() override
	{
		return uint32_t( m_SwapChainImages.size() );
	}

	void BeginFrame() override;
	void EndFrame() override;
	void Present() override;

	const char* GetRendererString() const override
	{
		return m_RendererString.c_str();
	}

	bool IsVulkanInstanceExtensionEnabled( const char* extensionName ) const override
	{
		return enabledExtensions.instance.find( extensionName ) != enabledExtensions.instance.end();
	}

	bool IsVulkanDeviceExtensionEnabled( const char* extensionName ) const override
	{
		return enabledExtensions.device.find( extensionName ) != enabledExtensions.device.end();
	}

	bool IsVulkanLayerEnabled( const char* layerName ) const override
	{
		return enabledExtensions.layers.find( layerName ) != enabledExtensions.layers.end();
	}

	void GetEnabledVulkanInstanceExtensions( std::vector<std::string>& extensions ) const override
	{
		for( const auto& ext : enabledExtensions.instance )
		{
			extensions.push_back( ext );
		}
	}

	void GetEnabledVulkanDeviceExtensions( std::vector<std::string>& extensions ) const override
	{
		for( const auto& ext : enabledExtensions.device )
		{
			extensions.push_back( ext );
		}
	}

	void GetEnabledVulkanLayers( std::vector<std::string>& layers ) const override
	{
		for( const auto& ext : enabledExtensions.layers )
		{
			layers.push_back( ext );
		}
	}

private:
	bool createInstance();
	bool createWindowSurface();
	void installDebugCallback();
	bool pickPhysicalDevice();
	bool findQueueFamilies( vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface );
	bool createDevice();
	bool createSwapChain();
	void destroySwapChain();

	struct VulkanExtensionSet
	{
		std::unordered_set<std::string> instance;
		std::unordered_set<std::string> layers;
		std::unordered_set<std::string> device;
	};

	// minimal set of required extensions
	VulkanExtensionSet enabledExtensions =
	{
		// instance
		{
			VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
		},
		// layers
		{ },
		// device
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_MAINTENANCE1_EXTENSION_NAME,
#if defined(__APPLE__) && defined( VK_KHR_portability_subset )
			// SRS - This is required for using the MoltenVK portability subset implementation on macOS
			VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
		},
	};

	// optional extensions
	VulkanExtensionSet optionalExtensions =
	{
		// instance
		{
#if defined(__APPLE__)
#if defined( VK_KHR_portability_enumeration )
			// SRS - This is needed for enumerating the MoltenVK portability implementation on macOS
			VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
			VK_EXT_LAYER_SETTINGS_EXTENSION_NAME,
#endif
			VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME
		},
		// layers
		{ },
		// device
		{
			VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_NV_MESH_SHADER_EXTENSION_NAME,
			VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
#if USE_OPTICK
			VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME,
#endif
#if defined( VK_KHR_format_feature_flags2 )
			VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME,
#endif
			VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
			VK_EXT_MEMORY_BUDGET_EXTENSION_NAME
		},
	};

	std::unordered_set<std::string> m_RayTracingExtensions =
	{
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
		VK_KHR_RAY_QUERY_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
	};

	std::string m_RendererString;

	vk::Instance m_VulkanInstance;
	vk::DebugReportCallbackEXT m_DebugReportCallback;

	vk::PhysicalDevice m_VulkanPhysicalDevice;
	int m_GraphicsQueueFamily = -1;
	int m_ComputeQueueFamily = -1;
	int m_TransferQueueFamily = -1;
	int m_PresentQueueFamily = -1;

	vk::Device m_VulkanDevice;
	vk::Queue m_GraphicsQueue;
	vk::Queue m_ComputeQueue;
	vk::Queue m_TransferQueue;
	vk::Queue m_PresentQueue;

	vk::SurfaceKHR m_WindowSurface;

	vk::SurfaceFormatKHR m_SwapChainFormat;
	vk::SwapchainKHR m_SwapChain;

	struct SwapChainImage
	{
		vk::Image image;
		nvrhi::TextureHandle rhiHandle;
	};

	std::vector<SwapChainImage> m_SwapChainImages;
	uint32_t m_SwapChainIndex = uint32_t( -1 );

	nvrhi::vulkan::DeviceHandle m_NvrhiDevice;
	nvrhi::DeviceHandle m_ValidationLayer;

	//nvrhi::CommandListHandle m_BarrierCommandList;		// SRS - no longer needed
	std::queue<vk::Semaphore> m_PresentSemaphoreQueue;
	vk::Semaphore m_PresentSemaphore;

	nvrhi::EventQueryHandle m_FrameWaitQuery;

	// SRS - flags indicating support for various Vulkan surface presentation modes
	bool enablePModeMailbox = false;		// r_swapInterval = 0 (defaults to eImmediate if not available)
	bool enablePModeImmediate = false;		// r_swapInterval = 0 (defaults to eFifo if not available)
	bool enablePModeFifoRelaxed = false;	// r_swapInterval = 1 (defaults to eFifo if not available)

	// SRS - flag indicating support for presentation timing via VK_GOOGLE_display_timing extension
	bool displayTimingEnabled = false;

	// SRS - slot for Vulkan device API version at runtime (initialize to Vulkan build version)
	uint32_t m_DeviceApiVersion = VK_HEADER_VERSION_COMPLETE;

	// SRS - function pointer for initing Vulkan DynamicLoader, VMA, Optick, and MoltenVK functions
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;

#if defined(__APPLE__) && defined( USE_MoltenVK )
	// SRS - function pointer for retrieving MoltenVK advanced performance statistics
	PFN_vkGetPerformanceStatisticsMVK vkGetPerformanceStatisticsMVK = nullptr;

#if MVK_VERSION >= MVK_MAKE_VERSION( 1, 2, 9 ) && USE_OPTICK
	// SRS - Optick event storage for MoltenVK's Vulkan-to-Metal encoding thread
	Optick::EventStorage* mvkAcquireEventStorage;
	Optick::EventStorage* mvkSubmitEventStorage;
	Optick::EventStorage* mvkEncodeEventStorage;
	Optick::EventDescription* mvkAcquireEventDesc;
	Optick::EventDescription* mvkSubmitEventDesc;
	Optick::EventDescription* mvkEncodeEventDesc;
	int64_t mvkLatestSubmitTime = 0;
	int64_t mvkPreviousSubmitTime = 0;
	int64_t mvkPreviousSubmitWaitTime = 0;
	double mvkPreviousAcquireHash = 0.0;
#endif
#endif

private:
	static VKAPI_ATTR vk::Bool32 VKAPI_CALL vulkanDebugCallback(
		vk::DebugReportFlagsEXT flags,
		vk::DebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData )
	{
		const DeviceManager_VK* manager = ( const DeviceManager_VK* )userData;

		if( manager )
		{
			const auto& ignored = manager->m_DeviceParams.ignoredVulkanValidationMessageLocations;
			const auto found = std::find( ignored.begin(), ignored.end(), location );
			if( found != ignored.end() )
			{
				return VK_FALSE;
			}
		}

		if( flags & vk::DebugReportFlagBitsEXT::eError )
		{
			idLib::Printf( "[Vulkan] ERROR location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg );
		}
		else if( flags & vk::DebugReportFlagBitsEXT::eWarning )
		{
			idLib::Printf( "[Vulkan] WARNING location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg );
		}
		else if( flags & vk::DebugReportFlagBitsEXT::ePerformanceWarning )
		{
			idLib::Printf( "[Vulkan] PERFORMANCE WARNING location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg );
		}
		else if( flags & vk::DebugReportFlagBitsEXT::eInformation )
		{
			idLib::Printf( "[Vulkan] INFO location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg );
		}
		else if( flags & vk::DebugReportFlagBitsEXT::eDebug )
		{
			idLib::Printf( "[Vulkan] DEBUG location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg );
		}

		return VK_FALSE;
	}
};

static std::vector<const char*> stringSetToVector( const std::unordered_set<std::string>& set )
{
	std::vector<const char*> ret;
	for( const auto& s : set )
	{
		ret.push_back( s.c_str() );
	}

	return ret;
}

template <typename T>
static std::vector<T> setToVector( const std::unordered_set<T>& set )
{
	std::vector<T> ret;
	for( const auto& s : set )
	{
		ret.push_back( s );
	}

	return ret;
}

bool DeviceManager_VK::createInstance()
{
#if defined( VULKAN_USE_PLATFORM_SDL )
	// SRS - Populate enabledExtensions with required SDL instance extensions
	auto sdl_instanceExtensions = get_required_extensions();
	for( auto instanceExtension : sdl_instanceExtensions )
	{
		enabledExtensions.instance.insert( instanceExtension );
	}
#elif defined( VK_USE_PLATFORM_WIN32_KHR )
	enabledExtensions.instance.insert( VK_KHR_SURFACE_EXTENSION_NAME );
	enabledExtensions.instance.insert( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#endif

	// add instance extensions requested by the user
	for( const std::string& name : m_DeviceParams.requiredVulkanInstanceExtensions )
	{
		enabledExtensions.instance.insert( name );
	}
	for( const std::string& name : m_DeviceParams.optionalVulkanInstanceExtensions )
	{
		optionalExtensions.instance.insert( name );
	}

	// add layers requested by the user
	for( const std::string& name : m_DeviceParams.requiredVulkanLayers )
	{
		enabledExtensions.layers.insert( name );
	}
	for( const std::string& name : m_DeviceParams.optionalVulkanLayers )
	{
		optionalExtensions.layers.insert( name );
	}

	std::unordered_set<std::string> requiredExtensions = enabledExtensions.instance;

	// figure out which optional extensions are supported
	for( const auto& instanceExt : vk::enumerateInstanceExtensionProperties() )
	{
		const std::string name = instanceExt.extensionName;
		if( optionalExtensions.instance.find( name ) != optionalExtensions.instance.end() )
		{
			enabledExtensions.instance.insert( name );
		}

		requiredExtensions.erase( name );
	}

	if( !requiredExtensions.empty() )
	{
		std::stringstream ss;
		ss << "Cannot create a Vulkan instance because the following required extension(s) are not supported:";
		for( const auto& ext : requiredExtensions )
		{
			ss << std::endl << "  - " << ext;
		}

		common->FatalError( "%s", ss.str().c_str() );
		return false;
	}

	common->Printf( "Enabled Vulkan instance extensions:\n" );
	for( const auto& ext : enabledExtensions.instance )
	{
		common->Printf( "    %s\n", ext.c_str() );
	}

	std::unordered_set<std::string> requiredLayers = enabledExtensions.layers;

	auto instanceVersion = vk::enumerateInstanceVersion();
	for( const auto& layer : vk::enumerateInstanceLayerProperties() )
	{
		const std::string name = layer.layerName;
		if( optionalExtensions.layers.find( name ) != optionalExtensions.layers.end() )
		{
			enabledExtensions.layers.insert( name );
		}

		requiredLayers.erase( name );
	}

	if( !requiredLayers.empty() )
	{
		std::stringstream ss;
		ss << "Cannot create a Vulkan instance because the following required layer(s) are not supported:";
		for( const auto& ext : requiredLayers )
		{
			ss << std::endl << "  - " << ext;
		}

		common->FatalError( "%s", ss.str().c_str() );
		return false;
	}

	common->Printf( "Enabled Vulkan layers:\n" );
	for( const auto& layer : enabledExtensions.layers )
	{
		common->Printf( "    %s\n", layer.c_str() );
	}

	auto instanceExtVec = stringSetToVector( enabledExtensions.instance );
	auto layerVec = stringSetToVector( enabledExtensions.layers );

	auto applicationInfo = vk::ApplicationInfo()
						   .setApiVersion( VK_MAKE_VERSION( 1, 2, 0 ) );

	// create the vulkan instance
	vk::InstanceCreateInfo info = vk::InstanceCreateInfo()
								  .setEnabledLayerCount( uint32_t( layerVec.size() ) )
								  .setPpEnabledLayerNames( layerVec.data() )
								  .setEnabledExtensionCount( uint32_t( instanceExtVec.size() ) )
								  .setPpEnabledExtensionNames( instanceExtVec.data() )
								  .setPApplicationInfo( &applicationInfo );

#if defined(__APPLE__)
#if defined( VK_KHR_portability_enumeration )
	if( enabledExtensions.instance.find( VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME ) != enabledExtensions.instance.end() )
	{
		info.setFlags( vk::InstanceCreateFlagBits( VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR ) );
	}
#endif

	std::vector<vk::LayerSettingEXT> layerSettings;
	vk::LayerSettingsCreateInfoEXT layerSettingsCreateInfo;

	const vk::Bool32 valueTrue = vk::True, valueFalse = vk::False;
	const int32_t useMTLHeap = r_mvkUseMTLHeap.GetInteger();
	const float timestampPeriodLowPassAlpha = 1.0;
	const int32_t logLevelErrorsOnly = 1; // MVK_CONFIG_LOG_LEVEL_ERROR

	// SRS - Use MoltenVK layer for configuration via standardized VK_EXT_layer_settings extension
	vk::LayerSettingEXT layerSetting = { "MoltenVK", "", vk::LayerSettingTypeEXT( 0 ), 1, nullptr };

	// SRS - Set MoltenVK's synchronous queue submit option for vkQueueSubmit() & vkQueuePresentKHR()
	layerSetting.pSettingName = "MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS";
	layerSetting.type = vk::LayerSettingTypeEXT::eBool32;
	layerSetting.pValues = r_mvkSynchronousQueueSubmits.GetBool() ? &valueTrue : &valueFalse;
	layerSettings.push_back( layerSetting );

	// SRS - Enable MoltenVK's image view swizzle feature in case we don't have native image view swizzle
	layerSetting.pSettingName = "MVK_CONFIG_FULL_IMAGE_VIEW_SWIZZLE";
	layerSetting.type = vk::LayerSettingTypeEXT::eBool32;
	layerSetting.pValues = &valueTrue;
	layerSettings.push_back( layerSetting );

	// SRS - Set MoltenVK's Metal argument buffer option for controlling descriptor resource scaling
	layerSetting.pSettingName = "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS";
	layerSetting.type = vk::LayerSettingTypeEXT::eBool32;
	layerSetting.pValues = r_mvkUseMetalArgumentBuffers.GetBool() ? &valueTrue : &valueFalse;
	layerSettings.push_back( layerSetting );

	// SRS - Set MoltenVK's MTLHeap option for GPU memory suballocations (may cause issues on AMD GPUs)
	layerSetting.pSettingName = "MVK_CONFIG_USE_MTLHEAP";
	layerSetting.type = vk::LayerSettingTypeEXT::eInt32;
	layerSetting.pValues = &useMTLHeap;
	layerSettings.push_back( layerSetting );

	// SRS - Disable MoltenVK's timestampPeriod filter for HUD / Optick profiler timing calibration
	layerSetting.pSettingName = "MVK_CONFIG_TIMESTAMP_PERIOD_LOWPASS_ALPHA";
	layerSetting.type = vk::LayerSettingTypeEXT::eFloat32;
	layerSetting.pValues = &timestampPeriodLowPassAlpha;
	layerSettings.push_back( layerSetting );

#if defined( USE_MoltenVK )
	// SRS - Enable MoltenVK's performance tracking for display of Metal encoding timer on macOS
	layerSetting.pSettingName = "MVK_CONFIG_PERFORMANCE_TRACKING";
	layerSetting.type = vk::LayerSettingTypeEXT::eBool32;
	layerSetting.pValues = &valueTrue;
	layerSettings.push_back( layerSetting );

#if !defined( _DEBUG )
	// SRS - Set MoltenVK's log level to "errors only" for release builds to avoid verbose messages
	layerSetting.pSettingName = "MVK_CONFIG_LOG_LEVEL";
	layerSetting.type = vk::LayerSettingTypeEXT::eInt32;
	layerSetting.pValues = &logLevelErrorsOnly;
	layerSettings.push_back( layerSetting );
#endif
#endif

	layerSettingsCreateInfo.settingCount = uint32_t( layerSettings.size() );
	layerSettingsCreateInfo.pSettings = layerSettings.data();

	info.setPNext( &layerSettingsCreateInfo );
#endif

	const vk::Result res = vk::createInstance( &info, nullptr, &m_VulkanInstance );
	if( res != vk::Result::eSuccess )
	{
		common->FatalError( "Failed to create a Vulkan instance, error code = %s", nvrhi::vulkan::resultToString( ( VkResult )res ) );
		return false;
	}

	VULKAN_HPP_DEFAULT_DISPATCHER.init( m_VulkanInstance );

	return true;
}

void DeviceManager_VK::installDebugCallback()
{
	auto info = vk::DebugReportCallbackCreateInfoEXT()
				.setFlags( vk::DebugReportFlagBitsEXT::eError |
						   vk::DebugReportFlagBitsEXT::eWarning |
						   //   vk::DebugReportFlagBitsEXT::eInformation |
						   vk::DebugReportFlagBitsEXT::ePerformanceWarning )
#if VK_HEADER_VERSION >= 304
				.setPfnCallback( vulkanDebugCallback )
#else
				.setPfnCallback( reinterpret_cast<PFN_vkDebugReportCallbackEXT>( vulkanDebugCallback ) )
#endif
				.setPUserData( this );

	const vk::Result res = m_VulkanInstance.createDebugReportCallbackEXT( &info, nullptr, &m_DebugReportCallback );
	assert( res == vk::Result::eSuccess );
}

bool DeviceManager_VK::pickPhysicalDevice()
{
	vk::Format requestedFormat = vk::Format( nvrhi::vulkan::convertFormat( m_DeviceParams.swapChainFormat ) );
	vk::Extent2D requestedExtent( m_DeviceParams.backBufferWidth, m_DeviceParams.backBufferHeight );

	auto devices = m_VulkanInstance.enumeratePhysicalDevices();

	// Start building an error message in case we cannot find a device.
	std::stringstream errorStream;
	errorStream << "Cannot find a Vulkan device that supports all the required extensions and properties.";

	// build a list of GPUs
	std::vector<vk::PhysicalDevice> discreteGPUs;
	std::vector<vk::PhysicalDevice> otherGPUs;
	for( const auto& dev : devices )
	{
		auto prop = dev.getProperties();

		errorStream << std::endl << prop.deviceName.data() << ":";

		// check that all required device extensions are present
		std::unordered_set<std::string> requiredExtensions = enabledExtensions.device;
		auto deviceExtensions = dev.enumerateDeviceExtensionProperties();
		for( const auto& ext : deviceExtensions )
		{
			requiredExtensions.erase( std::string( ext.extensionName.data() ) );
		}

		bool deviceIsGood = true;

		if( !requiredExtensions.empty() )
		{
			// device is missing one or more required extensions
			for( const auto& ext : requiredExtensions )
			{
				errorStream << std::endl << "  - missing " << ext;
			}
			deviceIsGood = false;
		}

		auto deviceFeatures = dev.getFeatures();
		if( !deviceFeatures.samplerAnisotropy )
		{
			// device is a toaster oven
			errorStream << std::endl << "  - does not support samplerAnisotropy";
			deviceIsGood = false;
		}
		if( !deviceFeatures.textureCompressionBC )
		{
			errorStream << std::endl << "  - does not support textureCompressionBC";
			deviceIsGood = false;
		}

		// check that this device supports our intended swap chain creation parameters
		auto surfaceCaps = dev.getSurfaceCapabilitiesKHR( m_WindowSurface );
		auto surfaceFmts = dev.getSurfaceFormatsKHR( m_WindowSurface );
		auto surfacePModes = dev.getSurfacePresentModesKHR( m_WindowSurface );

		// SRS/Ricardo Garcia rg3 - clamp swapChainBufferCount to the min/max capabilities of the surface
		m_DeviceParams.swapChainBufferCount = Max( surfaceCaps.minImageCount, m_DeviceParams.swapChainBufferCount );
		m_DeviceParams.swapChainBufferCount = surfaceCaps.maxImageCount > 0 ? Min( m_DeviceParams.swapChainBufferCount, surfaceCaps.maxImageCount ) : m_DeviceParams.swapChainBufferCount;

		/* SRS - Don't check extent here since window manager surfaceCaps may restrict extent to something smaller than requested
			   - Instead, check and clamp extent to window manager surfaceCaps during swap chain creation inside createSwapChain()
		if( surfaceCaps.minImageExtent.width > requestedExtent.width ||
				surfaceCaps.minImageExtent.height > requestedExtent.height ||
				surfaceCaps.maxImageExtent.width < requestedExtent.width ||
				surfaceCaps.maxImageExtent.height < requestedExtent.height )
		{
			errorStream << std::endl << "  - cannot support the requested swap chain size:";
			errorStream << " requested " << requestedExtent.width << "x" << requestedExtent.height << ", ";
			errorStream << " available " << surfaceCaps.minImageExtent.width << "x" << surfaceCaps.minImageExtent.height;
			errorStream << " - " << surfaceCaps.maxImageExtent.width << "x" << surfaceCaps.maxImageExtent.height;
			deviceIsGood = false;
		}
		*/

		bool surfaceFormatPresent = false;
		for( const vk::SurfaceFormatKHR& surfaceFmt : surfaceFmts )
		{
			if( surfaceFmt.format == requestedFormat )
			{
				surfaceFormatPresent = true;
				break;
			}
		}

		if( !surfaceFormatPresent )
		{
			// can't create a swap chain using the format requested
			errorStream << std::endl << "  - does not support the requested swap chain format";
			deviceIsGood = false;
		}

		if( find( surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eFifo ) == surfacePModes.end() )
		{
			// this should never happen since eFifo is mandatory according to the Vulkan spec
			errorStream << std::endl << "  - does not support the required surface present modes";
			deviceIsGood = false;
		}

		if( !findQueueFamilies( dev, m_WindowSurface ) )
		{
			// device doesn't have all the queue families we need
			errorStream << std::endl << "  - does not support the necessary queue types";
			deviceIsGood = false;
		}

		// check that we can present from the graphics queue
		uint32_t canPresent = dev.getSurfaceSupportKHR( m_GraphicsQueueFamily, m_WindowSurface );
		if( !canPresent )
		{
			errorStream << std::endl << "  - cannot present";
			deviceIsGood = false;
		}

		if( !deviceIsGood )
		{
			continue;
		}

		if( prop.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
		{
			discreteGPUs.push_back( dev );
		}
		else
		{
			otherGPUs.push_back( dev );
		}
	}

	// pick the first discrete GPU if it exists, otherwise the first integrated GPU
	if( !discreteGPUs.empty() )
	{
		glConfig.vendor = getGPUVendor( discreteGPUs[0].getProperties().vendorID );
		glConfig.gpuType = GPU_TYPE_DISCRETE;
		m_VulkanPhysicalDevice = discreteGPUs[0];
		return true;
	}

	if( !otherGPUs.empty() )
	{
		glConfig.vendor = getGPUVendor( otherGPUs[0].getProperties().vendorID );
		glConfig.gpuType = GPU_TYPE_OTHER;
		m_VulkanPhysicalDevice = otherGPUs[0];
		return true;
	}

	common->FatalError( "%s", errorStream.str().c_str() );

	return false;
}

bool DeviceManager_VK::findQueueFamilies( vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface )
{
	auto props = physicalDevice.getQueueFamilyProperties();

	for( int i = 0; i < int( props.size() ); i++ )
	{
		const auto& queueFamily = props[i];

		if( m_GraphicsQueueFamily == -1 )
		{
			if( queueFamily.queueCount > 0 &&
					( queueFamily.queueFlags & vk::QueueFlagBits::eGraphics ) )
			{
				m_GraphicsQueueFamily = i;
			}
		}

		if( m_ComputeQueueFamily == -1 )
		{
			if( queueFamily.queueCount > 0 &&
					( queueFamily.queueFlags & vk::QueueFlagBits::eCompute ) &&
					!( queueFamily.queueFlags & vk::QueueFlagBits::eGraphics ) )
			{
				m_ComputeQueueFamily = i;
			}
		}

		if( m_TransferQueueFamily == -1 )
		{
			if( queueFamily.queueCount > 0 &&
					( queueFamily.queueFlags & vk::QueueFlagBits::eTransfer ) &&
					!( queueFamily.queueFlags & vk::QueueFlagBits::eCompute ) &&
					!( queueFamily.queueFlags & vk::QueueFlagBits::eGraphics ) )
			{
				m_TransferQueueFamily = i;
			}
		}

		if( m_PresentQueueFamily == -1 )
		{
			vk::Bool32 presentSupported;
			// SRS - Use portable implmentation for detecting presentation support vs. Windows-specific Vulkan call
			if( queueFamily.queueCount > 0 &&
					physicalDevice.getSurfaceSupportKHR( i, surface, &presentSupported ) == vk::Result::eSuccess )
			{
				if( presentSupported )
				{
					m_PresentQueueFamily = i;
				}
			}
		}
	}

	if( m_GraphicsQueueFamily == -1 ||
			m_PresentQueueFamily == -1 ||
			( m_ComputeQueueFamily == -1 && m_DeviceParams.enableComputeQueue ) ||
			( m_TransferQueueFamily == -1 && m_DeviceParams.enableCopyQueue ) )
	{
		return false;
	}

	return true;
}

bool DeviceManager_VK::createDevice()
{
	// figure out which optional extensions are supported
	auto deviceExtensions = m_VulkanPhysicalDevice.enumerateDeviceExtensionProperties();
	for( const auto& ext : deviceExtensions )
	{
		const std::string name = ext.extensionName;
		if( optionalExtensions.device.find( name ) != optionalExtensions.device.end() )
		{
			enabledExtensions.device.insert( name );
		}

		if( m_DeviceParams.enableRayTracingExtensions && m_RayTracingExtensions.find( name ) != m_RayTracingExtensions.end() )
		{
			enabledExtensions.device.insert( name );
		}
	}

	bool accelStructSupported = false;
	bool bufferAddressSupported = false;
	bool rayPipelineSupported = false;
	bool rayQuerySupported = false;
	bool meshletsSupported = false;
	bool vrsSupported = false;
	bool sync2Supported = false;

	common->Printf( "Enabled Vulkan device extensions:\n" );
	for( const auto& ext : enabledExtensions.device )
	{
		common->Printf( "    %s\n", ext.c_str() );

		if( ext == VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME )
		{
			accelStructSupported = true;
		}
		else if( ext == VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME )
		{
			// RB: only makes problems at the moment
			bufferAddressSupported = true;
		}
		else if( ext == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME )
		{
			rayPipelineSupported = true;
		}
		else if( ext == VK_KHR_RAY_QUERY_EXTENSION_NAME )
		{
			rayQuerySupported = true;
		}
		else if( ext == VK_NV_MESH_SHADER_EXTENSION_NAME )
		{
			meshletsSupported = true;
		}
		else if( ext == VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME )
		{
			vrsSupported = true;
		}
		else if( ext == VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME )
		{
			sync2Supported = true;
		}
		else if( ext == VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME )
		{
			displayTimingEnabled = true;
		}
	}

	std::unordered_set<int> uniqueQueueFamilies =
	{
		m_GraphicsQueueFamily,
		m_PresentQueueFamily
	};

	if( m_DeviceParams.enableComputeQueue )
	{
		uniqueQueueFamilies.insert( m_ComputeQueueFamily );
	}

	if( m_DeviceParams.enableCopyQueue )
	{
		uniqueQueueFamilies.insert( m_TransferQueueFamily );
	}

	float priority = 1.f;
	std::vector<vk::DeviceQueueCreateInfo> queueDesc;
	for( int queueFamily : uniqueQueueFamilies )
	{
		queueDesc.push_back( vk::DeviceQueueCreateInfo()
							 .setQueueFamilyIndex( queueFamily )
							 .setQueueCount( 1 )
							 .setPQueuePriorities( &priority ) );
	}

	auto accelStructFeatures = vk::PhysicalDeviceAccelerationStructureFeaturesKHR()
							   .setAccelerationStructure( true );
	auto rayPipelineFeatures = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR()
							   .setRayTracingPipeline( true )
							   .setRayTraversalPrimitiveCulling( true );
	auto rayQueryFeatures = vk::PhysicalDeviceRayQueryFeaturesKHR()
							.setRayQuery( true );
	auto meshletFeatures = vk::PhysicalDeviceMeshShaderFeaturesNV()
						   .setTaskShader( true )
						   .setMeshShader( true );

	// SRS - get/set shading rate features which are detected individually by nvrhi (not just at extension level)
	vk::PhysicalDeviceFeatures2 actualDeviceFeatures2;
	vk::PhysicalDeviceFragmentShadingRateFeaturesKHR fragmentShadingRateFeatures;
	actualDeviceFeatures2.pNext = &fragmentShadingRateFeatures;
	m_VulkanPhysicalDevice.getFeatures2( &actualDeviceFeatures2 );

	auto vrsFeatures = vk::PhysicalDeviceFragmentShadingRateFeaturesKHR()
					   .setPipelineFragmentShadingRate( fragmentShadingRateFeatures.pipelineFragmentShadingRate )
					   .setPrimitiveFragmentShadingRate( fragmentShadingRateFeatures.primitiveFragmentShadingRate )
					   .setAttachmentFragmentShadingRate( fragmentShadingRateFeatures.attachmentFragmentShadingRate );

	auto sync2Features = vk::PhysicalDeviceSynchronization2FeaturesKHR()
						 .setSynchronization2( true );

#if defined(__APPLE__) && defined( VK_KHR_portability_subset )
	auto portabilityFeatures = vk::PhysicalDevicePortabilitySubsetFeaturesKHR()
#if USE_OPTICK
							   .setEvents( true )
#endif
							   .setImageViewFormatSwizzle( true );

	void* pNext = &portabilityFeatures;
#else
	void* pNext = nullptr;
#endif
#define APPEND_EXTENSION(condition, desc) if (condition) { (desc).pNext = pNext; pNext = &(desc); }  // NOLINT(cppcoreguidelines-macro-usage)
	APPEND_EXTENSION( accelStructSupported, accelStructFeatures )
	APPEND_EXTENSION( rayPipelineSupported, rayPipelineFeatures )
	APPEND_EXTENSION( rayQuerySupported, rayQueryFeatures )
	APPEND_EXTENSION( meshletsSupported, meshletFeatures )
	APPEND_EXTENSION( vrsSupported, vrsFeatures )
	APPEND_EXTENSION( sync2Supported, sync2Features )
#undef APPEND_EXTENSION

	auto deviceFeatures = vk::PhysicalDeviceFeatures()
						  .setShaderImageGatherExtended( true )
						  .setShaderStorageImageReadWithoutFormat( actualDeviceFeatures2.features.shaderStorageImageReadWithoutFormat )
						  .setSamplerAnisotropy( true )
						  .setTessellationShader( true )
						  .setTextureCompressionBC( true )
#if !defined(__APPLE__)
						  .setGeometryShader( true )
#endif
						  .setFillModeNonSolid( true )
						  .setImageCubeArray( true )
						  .setDualSrcBlend( true );

	auto vulkan12features = vk::PhysicalDeviceVulkan12Features()
							.setDescriptorIndexing( true )
							.setRuntimeDescriptorArray( true )
							.setDescriptorBindingPartiallyBound( true )
							.setDescriptorBindingVariableDescriptorCount( true )
							.setTimelineSemaphore( true )
							.setShaderSampledImageArrayNonUniformIndexing( true )
							.setBufferDeviceAddress( bufferAddressSupported )
#if USE_OPTICK
							.setHostQueryReset( true )
#endif
							.setPNext( pNext );

	auto layerVec = stringSetToVector( enabledExtensions.layers );
	auto extVec = stringSetToVector( enabledExtensions.device );

	auto deviceDesc = vk::DeviceCreateInfo()
					  .setPQueueCreateInfos( queueDesc.data() )
					  .setQueueCreateInfoCount( uint32_t( queueDesc.size() ) )
					  .setPEnabledFeatures( &deviceFeatures )
					  .setEnabledExtensionCount( uint32_t( extVec.size() ) )
					  .setPpEnabledExtensionNames( extVec.data() )
					  .setEnabledLayerCount( uint32_t( layerVec.size() ) )
					  .setPpEnabledLayerNames( layerVec.data() )
					  .setPNext( &vulkan12features );

	const vk::Result res = m_VulkanPhysicalDevice.createDevice( &deviceDesc, nullptr, &m_VulkanDevice );
	if( res != vk::Result::eSuccess )
	{
		common->FatalError( "Failed to create a Vulkan physical device, error code = %s", nvrhi::vulkan::resultToString( ( VkResult )res ) );
		return false;
	}

	m_VulkanDevice.getQueue( m_GraphicsQueueFamily, 0, &m_GraphicsQueue );
	if( m_DeviceParams.enableComputeQueue )
	{
		m_VulkanDevice.getQueue( m_ComputeQueueFamily, 0, &m_ComputeQueue );
	}
	if( m_DeviceParams.enableCopyQueue )
	{
		m_VulkanDevice.getQueue( m_TransferQueueFamily, 0, &m_TransferQueue );
	}
	m_VulkanDevice.getQueue( m_PresentQueueFamily, 0, &m_PresentQueue );

	VULKAN_HPP_DEFAULT_DISPATCHER.init( m_VulkanDevice );

	// SRS - Determine if preferred image depth/stencil format D24S8 is supported (issue with Vulkan on AMD GPUs)
	vk::ImageFormatProperties imageFormatProperties;
	const vk::Result ret = m_VulkanPhysicalDevice.getImageFormatProperties( vk::Format::eD24UnormS8Uint,
						   vk::ImageType::e2D,
						   vk::ImageTiling::eOptimal,
						   vk::ImageUsageFlags( vk::ImageUsageFlagBits::eDepthStencilAttachment ),
						   vk::ImageCreateFlags( 0 ),
						   &imageFormatProperties );
	m_DeviceParams.enableImageFormatD24S8 = ( ret == vk::Result::eSuccess );

	// SRS/rg3 - Determine which Vulkan surface present modes are supported by device and surface
	auto surfacePModes = m_VulkanPhysicalDevice.getSurfacePresentModesKHR( m_WindowSurface );
	enablePModeMailbox = find( surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eMailbox ) != surfacePModes.end();
	enablePModeImmediate = find( surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eImmediate ) != surfacePModes.end();
	enablePModeFifoRelaxed = find( surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eFifoRelaxed ) != surfacePModes.end();

	// stash the device renderer string and api version
	auto prop = m_VulkanPhysicalDevice.getProperties();
	m_RendererString = std::string( prop.deviceName.data() );
	m_DeviceApiVersion = prop.apiVersion;

#if defined( USE_AMD_ALLOCATOR )
	// SRS - initialize the vma allocator
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = ( PFN_vkGetDeviceProcAddr )vkGetInstanceProcAddr( m_VulkanInstance, "vkGetDeviceProcAddr" );

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	allocatorCreateInfo.physicalDevice = m_VulkanPhysicalDevice;
	allocatorCreateInfo.device = m_VulkanDevice;
	allocatorCreateInfo.instance = m_VulkanInstance;
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
	allocatorCreateInfo.flags = bufferAddressSupported ? VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT : 0;
	allocatorCreateInfo.preferredLargeHeapBlockSize = ( VkDeviceSize )r_vmaDeviceLocalMemoryMB.GetInteger() * 1024 * 1024;
	vmaCreateAllocator( &allocatorCreateInfo, &m_VmaAllocator );
#endif

	common->Printf( "Created Vulkan device: %s\n", m_RendererString.c_str() );

	return true;
}

/*
* Vulkan Example base class
*
* Copyright (C) by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
bool DeviceManager_VK::createWindowSurface()
{
	// Create the platform-specific surface
#if defined( VULKAN_USE_PLATFORM_SDL )
	// SRS - Support generic SDL platform for linux and macOS
	auto res = vk::Result( CreateSDLWindowSurface( ( VkInstance )m_VulkanInstance, ( VkSurfaceKHR* )&m_WindowSurface ) );

#elif defined( VK_USE_PLATFORM_WIN32_KHR )
	auto surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR()
							 .setHinstance( ( HINSTANCE )windowInstance )
							 .setHwnd( ( HWND )windowHandle );

	auto res = m_VulkanInstance.createWin32SurfaceKHR( &surfaceCreateInfo, nullptr, &m_WindowSurface );
#endif

	if( res != vk::Result::eSuccess )
	{
		common->FatalError( "Failed to create a Vulkan window surface, error code = %s", nvrhi::vulkan::resultToString( ( VkResult )res ) );
		return false;
	}

	return true;
}

void DeviceManager_VK::destroySwapChain()
{
	if( m_VulkanDevice )
	{
		m_VulkanDevice.waitIdle();
	}

	while( !m_SwapChainImages.empty() )
	{
		auto sci = m_SwapChainImages.back();
		m_SwapChainImages.pop_back();
		sci.rhiHandle = nullptr;
	}

	if( m_SwapChain )
	{
		m_VulkanDevice.destroySwapchainKHR( m_SwapChain );
		m_SwapChain = nullptr;
	}
}

bool DeviceManager_VK::createSwapChain()
{
	m_SwapChainFormat =
	{
		vk::Format( nvrhi::vulkan::convertFormat( m_DeviceParams.swapChainFormat ) ),
		vk::ColorSpaceKHR::eSrgbNonlinear
	};

	// SRS - Clamp swap chain extent within the range supported by the device / window surface
	auto surfaceCaps = m_VulkanPhysicalDevice.getSurfaceCapabilitiesKHR( m_WindowSurface );
	m_DeviceParams.backBufferWidth = idMath::ClampInt( surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width, m_DeviceParams.backBufferWidth );
	m_DeviceParams.backBufferHeight = idMath::ClampInt( surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height, m_DeviceParams.backBufferHeight );

	vk::Extent2D extent = vk::Extent2D( m_DeviceParams.backBufferWidth, m_DeviceParams.backBufferHeight );

	std::unordered_set<uint32_t> uniqueQueues =
	{
		uint32_t( m_GraphicsQueueFamily ),
		uint32_t( m_PresentQueueFamily )
	};

	std::vector<uint32_t> queues = setToVector( uniqueQueues );

	const bool enableSwapChainSharing = queues.size() > 1;

	// SRS/rg3 - set up Vulkan present mode based on vsync setting and available surface features
	vk::PresentModeKHR presentMode;
	switch( m_DeviceParams.vsyncEnabled )
	{
		case 0:
			presentMode = enablePModeMailbox && r_vkPreferFastSync.GetBool() ? vk::PresentModeKHR::eMailbox :
						  ( enablePModeImmediate ? vk::PresentModeKHR::eImmediate : vk::PresentModeKHR::eFifo );
			break;
		case 1:
			presentMode = enablePModeFifoRelaxed ? vk::PresentModeKHR::eFifoRelaxed : vk::PresentModeKHR::eFifo;
			break;
		case 2:
		default:
			presentMode = vk::PresentModeKHR::eFifo;	// eFifo always supported according to Vulkan spec
	}

	auto desc = vk::SwapchainCreateInfoKHR()
				.setSurface( m_WindowSurface )
				.setMinImageCount( m_DeviceParams.swapChainBufferCount )
				.setImageFormat( m_SwapChainFormat.format )
				.setImageColorSpace( m_SwapChainFormat.colorSpace )
				.setImageExtent( extent )
				.setImageArrayLayers( 1 )
				.setImageUsage( vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled )
				.setImageSharingMode( enableSwapChainSharing ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive )
				.setQueueFamilyIndexCount( enableSwapChainSharing ? uint32_t( queues.size() ) : 0 )
				.setPQueueFamilyIndices( enableSwapChainSharing ? queues.data() : nullptr )
				.setPreTransform( vk::SurfaceTransformFlagBitsKHR::eIdentity )
				.setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque )
				.setPresentMode( presentMode )
				.setClipped( true )
				.setOldSwapchain( nullptr );

	const vk::Result res = m_VulkanDevice.createSwapchainKHR( &desc, nullptr, &m_SwapChain );
	if( res != vk::Result::eSuccess )
	{
		common->FatalError( "Failed to create a Vulkan swap chain, error code = %s", nvrhi::vulkan::resultToString( ( VkResult )res ) );
		return false;
	}

	// retrieve swap chain images
	auto images = m_VulkanDevice.getSwapchainImagesKHR( m_SwapChain );
	for( auto image : images )
	{
		SwapChainImage sci;
		sci.image = image;

		nvrhi::TextureDesc textureDesc;
		textureDesc.width = m_DeviceParams.backBufferWidth;
		textureDesc.height = m_DeviceParams.backBufferHeight;
		textureDesc.format = m_DeviceParams.swapChainFormat;
		textureDesc.debugName = "Swap chain image";
		textureDesc.initialState = nvrhi::ResourceStates::Present;
		textureDesc.keepInitialState = true;
		textureDesc.isRenderTarget = true;

		sci.rhiHandle = m_NvrhiDevice->createHandleForNativeTexture( nvrhi::ObjectTypes::VK_Image, nvrhi::Object( sci.image ), textureDesc );
		m_SwapChainImages.push_back( sci );
	}

	m_SwapChainIndex = 0;

	return true;
}

bool DeviceManager_VK::CreateDeviceAndSwapChain()
{
	// RB: control these through the cmdline
	m_DeviceParams.enableNvrhiValidationLayer = r_useValidationLayers.GetInteger() > 0;
	m_DeviceParams.enableDebugRuntime = r_useValidationLayers.GetInteger() > 1;

	// SRS - DynamicLoader is in a separate namespace in newer versions of Vulkan-Hpp
#if VK_HEADER_VERSION >= 301
	using VulkanDynamicLoader = vk::detail::DynamicLoader;
#else
	using VulkanDynamicLoader = vk::DynamicLoader;
#endif

	if( m_DeviceParams.enableDebugRuntime )
	{
#if defined(__APPLE__) && defined( USE_MoltenVK )
	}

	// SRS - when USE_MoltenVK defined, load libMoltenVK vs. the default libvulkan
	static const VulkanDynamicLoader dl( "libMoltenVK.dylib" );
#else
		enabledExtensions.layers.insert( "VK_LAYER_KHRONOS_validation" );

		// SRS - Suppress specific [ WARNING-Shader-OutputNotConsumed ] validation warnings which are by design:
		// 0xc81ad50e: vkCreateGraphicsPipelines(): pCreateInfos[0].pVertexInputState Vertex attribute at location X not consumed by vertex shader.
		// 0x9805298c: vkCreateGraphicsPipelines(): pCreateInfos[0] fragment shader writes to output location X with no matching attachment.
		// SRS - Suppress similar [ UNASSIGNED-CoreValidation-Shader-OutputNotConsumed ] warnings for older Vulkan SDKs:
		// 0x609a13b: vertex shader writes to output location X.0 which is not consumed by fragment shader...
		// 0x609a13b: Vertex attribute at location X not consumed by vertex shader.
		// 0x609a13b: fragment shader writes to output location X with no matching attachment.

		// SRS - Suppress false-positive descriptor count warning for MipMapGen pass which is by design:
		// 0x3af3126a: vkCreateComputePipelines(): pCreateInfos[0].stage uses ... with a descriptorCount of X, but requires at least Y in the SPIR-V.

#ifdef _WIN32
		SetEnvironmentVariable( "VK_LAYER_MESSAGE_ID_FILTER", "0xc81ad50e;0x9805298c;0x609a13b;0x3af3126a" );
#else
		setenv( "VK_LAYER_MESSAGE_ID_FILTER", "0xc81ad50e:0x9805298c:0x609a13b:0x3af3126a", 1 );
#endif
	}

	// SRS - make static so ~DynamicLoader() does not prematurely unload vulkan dynamic lib
	static const VulkanDynamicLoader dl;
#endif
	vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" );
	VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );

#define CHECK(a) if (!(a)) { return false; }

	CHECK( createInstance() );

	if( m_DeviceParams.enableDebugRuntime )
	{
		installDebugCallback();
	}

	if( m_DeviceParams.swapChainFormat == nvrhi::Format::SRGBA8_UNORM )
	{
		m_DeviceParams.swapChainFormat = nvrhi::Format::SBGRA8_UNORM;
	}
	else if( m_DeviceParams.swapChainFormat == nvrhi::Format::RGBA8_UNORM )
	{
		m_DeviceParams.swapChainFormat = nvrhi::Format::BGRA8_UNORM;
	}

	// add device extensions requested by the user
	for( const std::string& name : m_DeviceParams.requiredVulkanDeviceExtensions )
	{
		enabledExtensions.device.insert( name );
	}
	for( const std::string& name : m_DeviceParams.optionalVulkanDeviceExtensions )
	{
		optionalExtensions.device.insert( name );
	}

	CHECK( createWindowSurface() );
	CHECK( pickPhysicalDevice() );
	CHECK( findQueueFamilies( m_VulkanPhysicalDevice, m_WindowSurface ) );

#if defined(__APPLE__) && defined( USE_MoltenVK )
	// SRS - Get function pointer for retrieving MoltenVK advanced performance statistics in DeviceManager_VK::BeginFrame()
	vkGetPerformanceStatisticsMVK = ( PFN_vkGetPerformanceStatisticsMVK )vkGetInstanceProcAddr( m_VulkanInstance, "vkGetPerformanceStatisticsMVK" );

	#if MVK_VERSION >= MVK_MAKE_VERSION( 1, 2, 9 ) && USE_OPTICK
		// SRS - Initialize Optick event storage and descriptions for MoltenVK events
		mvkAcquireEventStorage = Optick::RegisterStorage( "Mvk_ImageAcquire", uint64_t( -1 ), Optick::ThreadMask::Main );
		mvkSubmitEventStorage = Optick::RegisterStorage( "Mvk_CmdBufSubmit", uint64_t( -1 ), Optick::ThreadMask::Main );
		mvkEncodeEventStorage = Optick::RegisterStorage( "Mvk_EncodeThread", uint64_t( -1 ), Optick::ThreadMask::GPU );
		mvkAcquireEventDesc = Optick::EventDescription::CreateShared( "Acquire_Wait" );
		mvkSubmitEventDesc = Optick::EventDescription::CreateShared( "Submit_Wait" );
		mvkEncodeEventDesc = Optick::EventDescription::CreateShared( "Metal_Encode" );
		Optick::SetStateChangedCallback( ( Optick::StateCallback )optickStateChangedCallback );
	#endif
#endif

	CHECK( createDevice() );

	auto vecInstanceExt = stringSetToVector( enabledExtensions.instance );
	auto vecLayers = stringSetToVector( enabledExtensions.layers );
	auto vecDeviceExt = stringSetToVector( enabledExtensions.device );

	nvrhi::vulkan::DeviceDesc deviceDesc;
	deviceDesc.errorCB = &DefaultMessageCallback::GetInstance();
	deviceDesc.instance = m_VulkanInstance;
	deviceDesc.physicalDevice = m_VulkanPhysicalDevice;
	deviceDesc.device = m_VulkanDevice;
	deviceDesc.graphicsQueue = m_GraphicsQueue;
	deviceDesc.graphicsQueueIndex = m_GraphicsQueueFamily;
	if( m_DeviceParams.enableComputeQueue )
	{
		deviceDesc.computeQueue = m_ComputeQueue;
		deviceDesc.computeQueueIndex = m_ComputeQueueFamily;
	}
	if( m_DeviceParams.enableCopyQueue )
	{
		deviceDesc.transferQueue = m_TransferQueue;
		deviceDesc.transferQueueIndex = m_TransferQueueFamily;
	}
	deviceDesc.instanceExtensions = vecInstanceExt.data();
	deviceDesc.numInstanceExtensions = vecInstanceExt.size();
	deviceDesc.deviceExtensions = vecDeviceExt.data();
	deviceDesc.numDeviceExtensions = vecDeviceExt.size();

	m_NvrhiDevice = nvrhi::vulkan::createDevice( deviceDesc );

	if( m_DeviceParams.enableNvrhiValidationLayer )
	{
		m_ValidationLayer = nvrhi::validation::createValidationLayer( m_NvrhiDevice );
	}

	CHECK( createSwapChain() );

	//m_BarrierCommandList = m_NvrhiDevice->createCommandList();		// SRS - no longer needed

	// SRS - Give each swapchain image its own semaphore in case of overlap (e.g. MoltenVK async queue submit)
	for( int i = 0; i < m_SwapChainImages.size(); i++ )
	{
		m_PresentSemaphoreQueue.push( m_VulkanDevice.createSemaphore( vk::SemaphoreCreateInfo() ) );
	}
	m_PresentSemaphore = m_PresentSemaphoreQueue.front();

	m_FrameWaitQuery = m_NvrhiDevice->createEventQuery();
	m_NvrhiDevice->setEventQuery( m_FrameWaitQuery, nvrhi::CommandQueue::Graphics );

#undef CHECK

#if USE_OPTICK
	const Optick::VulkanFunctions optickVulkanFunctions = { ( PFN_vkGetInstanceProcAddr_ )vkGetInstanceProcAddr };
#endif

	OPTICK_GPU_INIT_VULKAN( ( VkInstance )m_VulkanInstance, ( VkDevice* )&m_VulkanDevice, ( VkPhysicalDevice* )&m_VulkanPhysicalDevice, ( VkQueue* )&m_GraphicsQueue, ( uint32_t* )&m_GraphicsQueueFamily, 1, &optickVulkanFunctions );

	return true;
}

void DeviceManager_VK::DestroyDeviceAndSwapChain()
{
	OPTICK_SHUTDOWN();

	if( m_VulkanDevice )
	{
		m_VulkanDevice.waitIdle();
	}

	m_FrameWaitQuery = nullptr;

	for( int i = 0; i < m_SwapChainImages.size(); i++ )
	{
		m_VulkanDevice.destroySemaphore( m_PresentSemaphoreQueue.front() );
		m_PresentSemaphoreQueue.pop();
	}
	m_PresentSemaphore = vk::Semaphore();

	//m_BarrierCommandList = nullptr;		// SRS - no longer needed

	destroySwapChain();

	m_NvrhiDevice = nullptr;
	m_ValidationLayer = nullptr;
	m_RendererString.clear();

	if( m_DebugReportCallback )
	{
		m_VulkanInstance.destroyDebugReportCallbackEXT( m_DebugReportCallback );
	}

#if defined( USE_AMD_ALLOCATOR )
	if( m_VmaAllocator )
	{
		// SRS - make sure image allocation garbage is emptied for all frames
		for( int i = 0; i < NUM_FRAME_DATA; i++ )
		{
			idImage::EmptyGarbage();
		}
		vmaDestroyAllocator( m_VmaAllocator );
		m_VmaAllocator = nullptr;
	}
#endif

	if( m_VulkanDevice )
	{
		m_VulkanDevice.destroy();
		m_VulkanDevice = nullptr;
	}

	if( m_WindowSurface )
	{
		assert( m_VulkanInstance );
		m_VulkanInstance.destroySurfaceKHR( m_WindowSurface );
		m_WindowSurface = nullptr;
	}

	if( m_VulkanInstance )
	{
		m_VulkanInstance.destroy();
		m_VulkanInstance = nullptr;
	}
}

void DeviceManager_VK::BeginFrame()
{
	OPTICK_CATEGORY( "Vulkan_BeginFrame", Optick::Category::Wait );

	// SRS - get Vulkan GPU memory usage for display in statistics overlay HUD
	vk::PhysicalDeviceMemoryProperties2 memoryProperties2;
	vk::PhysicalDeviceMemoryBudgetPropertiesEXT memoryBudget;
	memoryProperties2.pNext = &memoryBudget;
	m_VulkanPhysicalDevice.getMemoryProperties2( &memoryProperties2 );

	VkDeviceSize gpuMemoryAllocated = 0;
	for( uint32_t i = 0; i < memoryProperties2.memoryProperties.memoryHeapCount; i++ )
	{
		gpuMemoryAllocated += memoryBudget.heapUsage[i];
	}
	commonLocal.SetRendererGpuMemoryMB( gpuMemoryAllocated / 1024 / 1024 );

	const vk::Result res = m_VulkanDevice.acquireNextImageKHR( m_SwapChain,
						   std::numeric_limits<uint64_t>::max(), // timeout
						   m_PresentSemaphore,
						   vk::Fence(),
						   &m_SwapChainIndex );

	assert( res == vk::Result::eSuccess || res == vk::Result::eSuboptimalKHR );

	m_NvrhiDevice->queueWaitForSemaphore( nvrhi::CommandQueue::Graphics, m_PresentSemaphore, 0 );
}

void DeviceManager_VK::EndFrame()
{
	OPTICK_CATEGORY( "Vulkan_EndFrame", Optick::Category::Wait );

	m_NvrhiDevice->queueSignalSemaphore( nvrhi::CommandQueue::Graphics, m_PresentSemaphore, 0 );

	// SRS - Don't need barrier commandlist if EndFrame() is called before executeCommandList() in idRenderBackend::GL_EndFrame()
	//m_BarrierCommandList->open(); // umm...
	//m_BarrierCommandList->close();
	//m_NvrhiDevice->executeCommandList( m_BarrierCommandList );

#if defined(__APPLE__) && defined( USE_MoltenVK )
#if MVK_VERSION >= MVK_MAKE_VERSION( 1, 2, 9 ) && USE_OPTICK
	// SRS - Capture MoltenVK command buffer submit time just before executeCommandList() in idRenderBackend::GL_EndFrame()
	mvkPreviousSubmitTime = mvkLatestSubmitTime;
	mvkLatestSubmitTime = Optick::GetHighPrecisionTime();
#endif
#endif
}

void DeviceManager_VK::Present()
{
	OPTICK_GPU_FLIP( m_SwapChain );
	OPTICK_CATEGORY( "Vulkan_Present", Optick::Category::Wait );
	OPTICK_TAG( "Frame", idLib::frameNumber - 1 );

	void* pNext = nullptr;
#if USE_OPTICK
	// SRS - if display timing enabled, define the presentID for labeling the Optick GPU VSync / Present queue
	vk::PresentTimeGOOGLE presentTime = vk::PresentTimeGOOGLE()
										.setPresentID( idLib::frameNumber - 1 );
	vk::PresentTimesInfoGOOGLE presentTimesInfo = vk::PresentTimesInfoGOOGLE()
			.setSwapchainCount( 1 )
			.setPTimes( &presentTime );
	if( displayTimingEnabled )
	{
		pNext = &presentTimesInfo;
	}
#endif

	vk::PresentInfoKHR info = vk::PresentInfoKHR()
							  .setWaitSemaphoreCount( 1 )
							  .setPWaitSemaphores( &m_PresentSemaphore )
							  .setSwapchainCount( 1 )
							  .setPSwapchains( &m_SwapChain )
							  .setPImageIndices( &m_SwapChainIndex )
							  .setPNext( pNext );

	const vk::Result res = m_PresentQueue.presentKHR( &info );
	assert( res == vk::Result::eSuccess || res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR );

	// SRS - Cycle the semaphore queue and setup m_PresentSemaphore for the next swapchain image
	m_PresentSemaphoreQueue.pop();
	m_PresentSemaphoreQueue.push( m_PresentSemaphore );
	m_PresentSemaphore = m_PresentSemaphoreQueue.front();

	// SRS - The following event queries provide explicit CPU/GPU synchronization (supports validation layer if enabled)
	if constexpr( NUM_FRAME_DATA > 2 )
	{
		OPTICK_CATEGORY( "Vulkan_Sync3", Optick::Category::Wait );

		// SRS - For triple buffering, sync on previous frame's command queue completion
		m_NvrhiDevice->waitEventQuery( m_FrameWaitQuery );
	}

	m_NvrhiDevice->resetEventQuery( m_FrameWaitQuery );
	m_NvrhiDevice->setEventQuery( m_FrameWaitQuery, nvrhi::CommandQueue::Graphics );

	if constexpr( NUM_FRAME_DATA < 3 )
	{
		OPTICK_CATEGORY( "Vulkan_Sync2", Optick::Category::Wait );

		// SRS - For double buffering, sync on current frame's command queue completion
		m_NvrhiDevice->waitEventQuery( m_FrameWaitQuery );
	}

#if defined(__APPLE__) && defined( USE_MoltenVK )
	if( vkGetPerformanceStatisticsMVK )
	{
		// SRS - get MoltenVK's Metal encoding time for display in statistics overlay HUD
		MVKPerformanceStatistics mvkPerfStats;
		size_t mvkPerfStatsSize = sizeof( mvkPerfStats );
		if( vkGetPerformanceStatisticsMVK( m_VulkanDevice, &mvkPerfStats, &mvkPerfStatsSize ) == VK_SUCCESS )
		{
			uint64 mvkEncodeTime = Max( 0.0, mvkPerfStats.queue.commandBufferEncoding.latest - mvkPerfStats.queue.retrieveCAMetalDrawable.latest ) * 1000000.0;

#if MVK_VERSION >= MVK_MAKE_VERSION( 1, 2, 9 ) && USE_OPTICK
			if( optickCapturing )
			{
				// SRS - create custom Optick event that displays MoltenVK's command buffer submit waiting time
				OPTICK_STORAGE_EVENT( mvkSubmitEventStorage, mvkSubmitEventDesc, mvkPreviousSubmitTime, mvkPreviousSubmitTime + mvkPreviousSubmitWaitTime );
				OPTICK_STORAGE_TAG( mvkSubmitEventStorage, mvkPreviousSubmitTime + mvkPreviousSubmitWaitTime / 2, "Frame", idLib::frameNumber - 2 );

				// SRS - select latest acquire time if hashes match and we didn't retrieve a new image, or vsync is on, or other high-load conditions
				double mvkLatestAcquireHash = mvkPerfStats.queue.retrieveCAMetalDrawable.latest + mvkPerfStats.queue.retrieveCAMetalDrawable.previous;
				bool useLatestAcquire = ( mvkLatestAcquireHash != mvkPreviousAcquireHash ) && ( mvkPerfStats.queue.waitSubmitCommandBuffers.latest > mvkPerfStats.queue.waitSubmitCommandBuffers.previous || mvkPerfStats.queue.commandBufferEncoding.latest > mvkPerfStats.queue.commandBufferEncoding.previous ) && ( mvkPerfStats.queue.retrieveCAMetalDrawable.latest > mvkPerfStats.queue.retrieveCAMetalDrawable.previous );
				int64_t mvkAcquireWaitTime = mvkLatestAcquireHash == mvkPreviousAcquireHash || r_swapInterval.GetInteger() > 0 || useLatestAcquire ? mvkPerfStats.queue.retrieveCAMetalDrawable.latest * 1000000.0 : mvkPerfStats.queue.retrieveCAMetalDrawable.previous * 1000000.0;

				// SRS - select latest presented frame if we are running synchronous, otherwise select previous presented frame as reference
				int64_t mvkAcquireStartTime = mvkPreviousSubmitTime + mvkPreviousSubmitWaitTime;
				int32_t frameNumberTag = idLib::frameNumber - 2;
				if( r_mvkSynchronousQueueSubmits.GetBool() )
				{
					mvkAcquireStartTime = mvkLatestSubmitTime + int64_t( mvkPerfStats.queue.waitSubmitCommandBuffers.latest * 1000000.0 );
					mvkAcquireWaitTime = mvkPerfStats.queue.retrieveCAMetalDrawable.latest * 1000000.0;
					frameNumberTag = idLib::frameNumber - 1;
				}

				// SRS - create custom Optick event that displays MoltenVK's image acquire waiting time
				OPTICK_STORAGE_EVENT( mvkAcquireEventStorage, mvkAcquireEventDesc, mvkAcquireStartTime, mvkAcquireStartTime + mvkAcquireWaitTime );
				OPTICK_STORAGE_TAG( mvkAcquireEventStorage, mvkAcquireStartTime + mvkAcquireWaitTime / 2, "Frame", frameNumberTag );

				// SRS - when Optick is active, use max of MoltenVK's latest/previous encoding time to select game command buffer vs. Optick's command buffer
				int64_t mvkEncodeStartTime = mvkAcquireStartTime + mvkAcquireWaitTime;
				mvkEncodeTime = Max( mvkPerfStats.queue.commandBufferEncoding.latest, mvkPerfStats.queue.commandBufferEncoding.previous ) * 1000000.0;
				mvkEncodeTime = ( mvkEncodeTime > mvkAcquireWaitTime ) && ( ( mvkPerfStats.queue.commandBufferEncoding.previous > mvkPerfStats.queue.commandBufferEncoding.latest && Max( mvkPreviousSubmitWaitTime, int64_t( mvkPerfStats.queue.waitSubmitCommandBuffers.previous * 1000000.0 ) ) > int64_t( mvkPerfStats.queue.waitSubmitCommandBuffers.latest * 1000000.0 ) ) || useLatestAcquire ) ? mvkEncodeTime - mvkAcquireWaitTime : mvkEncodeTime;

				// SRS - create custom Optick event that displays MoltenVK's Vulkan-to-Metal encoding time
				OPTICK_STORAGE_EVENT( mvkEncodeEventStorage, mvkEncodeEventDesc, mvkEncodeStartTime, mvkEncodeStartTime + mvkEncodeTime );
				OPTICK_STORAGE_TAG( mvkEncodeEventStorage, mvkEncodeStartTime + mvkEncodeTime / 2, "Frame", frameNumberTag );

				mvkPreviousSubmitWaitTime = Min( mvkPerfStats.queue.waitSubmitCommandBuffers.latest, mvkPerfStats.queue.waitSubmitCommandBuffers.previous ) * 1000000.0;
				mvkPreviousAcquireHash = mvkLatestAcquireHash;
			}
#endif
			commonLocal.SetRendererMvkEncodeMicroseconds( mvkEncodeTime / 1000 );
		}
	}
#endif
}

DeviceManager* DeviceManager::CreateVK()
{
	return new DeviceManager_VK();
}
