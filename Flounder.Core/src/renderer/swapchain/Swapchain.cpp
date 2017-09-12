﻿#include "Swapchain.hpp"
#include "../../devices/Display.hpp"

namespace Flounder
{
	Swapchain::Swapchain() :
		m_logicalDevice(VK_NULL_HANDLE),
		m_swapChain(VK_NULL_HANDLE),
		m_swapChainImages(std::vector<VkImage>()),
		m_swapChainImageViews(std::vector<VkImageView>()),
		m_swapChainFramebuffers(std::vector<VkFramebuffer>()),
		m_swapChainImageFormat(VkFormat()),
		m_swapChainExtent(VkExtent2D())
	{
	}

	Swapchain::~Swapchain()
	{
		CleanupFrameBuffers();
		Cleanup();
	}

	void Swapchain::Create(const VkDevice *logicalDevice, VkPhysicalDevice*physicalDevice, const VkSurfaceKHR *surface, GLFWwindow *window)
	{
		m_logicalDevice = logicalDevice;

		// Get support details for the swap chain to pass to helper functions
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(*physicalDevice, *surface);

		// Uses the helper functions to get optimal settings.
		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, window);

		// Fills in the data from the create info.
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = *surface;

		// Gets the proper image count. 
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		// Attempts to create swap chain
		GlfwVulkan::ErrorCheck(vkCreateSwapchainKHR(*logicalDevice, &createInfo, nullptr, &m_swapChain));

		// Populates the swap chain image vector.
		vkGetSwapchainImagesKHR(*logicalDevice, m_swapChain, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(*logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());

		// Stores the data for the chosen surface format and extent.
		m_swapChainImageFormat = surfaceFormat.format;
		m_swapChainExtent = extent;

		CreateImageViews(logicalDevice);

		printf("Swapchain created successfully.\n");
	}

	void Swapchain::CreateFramebuffers(const VkDevice *logicalDevice, const VkRenderPass *renderPass)
	{
		m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

		for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
		{
			VkImageView attachments[] = { m_swapChainImageViews[i] };

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = *renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = m_swapChainExtent.width;
			framebufferInfo.height = m_swapChainExtent.height;
			framebufferInfo.layers = 1;

			GlfwVulkan::ErrorCheck(vkCreateFramebuffer(*logicalDevice, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]));
		}

		printf("Framebuffers created successfully!\n");
	}

	void Swapchain::Cleanup()
	{
		// Waits for the device to finish before destroying.
		vkDeviceWaitIdle(Display::Get()->GetVkDevice());

		for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
		{
			vkDestroyImageView(Display::Get()->GetVkDevice(), m_swapChainImageViews[i], VK_NULL_HANDLE);
		}

		vkDestroySwapchainKHR(Display::Get()->GetVkDevice(), m_swapChain, VK_NULL_HANDLE);
	}

	void Swapchain::CleanupFrameBuffers()
	{
		// Waits for the device to finish before destroying.
		vkDeviceWaitIdle(Display::Get()->GetVkDevice());

		for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++)
		{
			vkDestroyFramebuffer(Display::Get()->GetVkDevice(), m_swapChainFramebuffers[i], VK_NULL_HANDLE);
		}
	}

	void Swapchain::CreateImageViews(const VkDevice* logicalDevice)
	{
		m_swapChainImageViews.resize(m_swapChainImages.size());

		for (uint32_t i = 0; i < m_swapChainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			GlfwVulkan::ErrorCheck(vkCreateImageView(*logicalDevice, &createInfo, nullptr, &m_swapChainImageViews[i]));
		}

		printf("Image views created successfully.\n");
	}

	SwapChainSupportDetails Swapchain::QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		SwapChainSupportDetails details;

		// Sets capabilities variable on struct.
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
		
		// Sets formats vector on struct.
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		// Sets presentModes vector on struct.
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		
		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR Swapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) const
	{
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
		}

		for (auto availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR Swapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) const
	{
		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

		for (auto availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}

			if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				bestMode = availablePresentMode;
			}
		}

		return bestMode;
	}

	VkExtent2D Swapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window) const
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}

		int width, height;
		glfwGetWindowSize(window, &width, &height);

		VkExtent2D actualExtent = {};
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, static_cast<uint32_t>(width)));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, static_cast<uint32_t>(height)));

		return actualExtent;
	}
}