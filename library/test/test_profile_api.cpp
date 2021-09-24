/*
 * Copyright (c) 2021-2022 Valve Corporation
 * Copyright (c) 2021-2022 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authors:
 * - Christophe Riccio <christophe@lunarg.com>
 */

#include "test.hpp"

#include "vulkan_profiles.hpp"

#include <cstdio>
#include <memory>
#include <vector>

static const float DEFAULT_QUEUE_PRIORITY(0.0f);

class TestScaffold {
   public:
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex;
    VkDeviceQueueCreateInfo queueCreateInfo;

    TestScaffold() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Testing scaffold";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        VkResult res = vkCreateInstance(&createInfo, nullptr, &instance);
        EXPECT_TRUE(res == VK_SUCCESS);

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        EXPECT_TRUE(deviceCount > 0);

        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
        physicalDevice = physicalDevices[0];
        queueCreateInfo = {};
        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());
        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); ++i) {
            if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queueFamilyIndex = i;
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = i;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &DEFAULT_QUEUE_PRIORITY;
                break;
            }
        }
    }
};

TEST(test_profile, enumerate) {
    TestScaffold scaffold;

    std::vector<VpProfileProperties> profiles;
    uint32_t profileCount = 0;
    
    VkResult result_count = vpEnumerateDeviceProfiles(scaffold.physicalDevice, nullptr, &profileCount, nullptr);
    ASSERT_TRUE(result_count == VK_SUCCESS);

    profiles.resize(profileCount);
    VkResult result_profile = vpEnumerateDeviceProfiles(scaffold.physicalDevice, nullptr, &profileCount, &profiles[0]);
    ASSERT_TRUE(result_profile == VK_SUCCESS);
    EXPECT_TRUE(profileCount > 0);

    for (VpProfileProperties profile : profiles) {
        std::printf("Profile supported: %s, version %d\n", profile.profileName, profile.specVersion);
    }
}

TEST(test_profile, create_profile) {
    TestScaffold scaffold;

    std::vector<VpProfileProperties> profiles;
    uint32_t profileCount = 0;

    VkResult result_count = vpEnumerateDeviceProfiles(scaffold.physicalDevice, nullptr, &profileCount, nullptr);
    ASSERT_TRUE(result_count == VK_SUCCESS);

    profiles.resize(profileCount);
    VkResult result_profile = vpEnumerateDeviceProfiles(scaffold.physicalDevice, nullptr, &profileCount, &profiles[0]);
    ASSERT_TRUE(result_profile == VK_SUCCESS);
    EXPECT_TRUE(profileCount > 0);

    for (VpProfileProperties profile : profiles) {
        std::printf("Profile supported: %s, version %d\n", profile.profileName, profile.specVersion);
    }

    int error = 0;

    for (const VpProfileProperties& profile : profiles) {
        std::printf("Creating a Vulkan device using profile %s, version %d: ", profile.profileName, profile.specVersion);

        VkDeviceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.pNext = nullptr;
        info.queueCreateInfoCount = 1;
        info.pQueueCreateInfos = &scaffold.queueCreateInfo;
        info.enabledExtensionCount = 0;
        info.ppEnabledExtensionNames = nullptr;
        info.pEnabledFeatures = nullptr;

        VkDevice device = VK_NULL_HANDLE;
        VkResult res = vpCreateDevice(scaffold.physicalDevice, &profile, &info, nullptr, &device);
        if (res != VK_SUCCESS) {
            ++error;
            ASSERT_TRUE(device == VK_NULL_HANDLE);
            std::printf("FAILURE: %d\n", res);
        } else {
            std::printf("SUCCESS!\n");
        }
    }

    EXPECT_EQ(0, error);
}


TEST(test_profile, create_extensions_supported) {
    TestScaffold scaffold;

    std::vector<VpProfileProperties> profiles;
    uint32_t profileCount = 0;

    VkResult result_count = vpEnumerateDeviceProfiles(scaffold.physicalDevice, nullptr, &profileCount, nullptr);
    ASSERT_TRUE(result_count == VK_SUCCESS);

    profiles.resize(profileCount);
    VkResult result_profile = vpEnumerateDeviceProfiles(scaffold.physicalDevice, nullptr, &profileCount, &profiles[0]);
    ASSERT_TRUE(result_profile == VK_SUCCESS);
    EXPECT_TRUE(profileCount > 0);

    for (VpProfileProperties profile : profiles) {
        std::printf("Profile supported: %s, version %d\n", profile.profileName, profile.specVersion);
    }

    int error = 0;

    static const char* extensions[] = {"VK_KHR_image_format_list", "VK_KHR_maintenance3", "VK_KHR_imageless_framebuffer"};

    for (const VpProfileProperties& profile : profiles) {
        std::printf("Creating a Vulkan device using profile %s, version %d: ", profile.profileName, profile.specVersion);

        VkPhysicalDeviceFeatures enabledFeatures = {};
        enabledFeatures.robustBufferAccess = VK_TRUE;

        VkDeviceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.pNext = nullptr;
        info.queueCreateInfoCount = 1;
        info.pQueueCreateInfos = &scaffold.queueCreateInfo;
        info.enabledExtensionCount = countof(extensions);
        info.ppEnabledExtensionNames = extensions;
        info.pEnabledFeatures = nullptr;

        VkDevice device = VK_NULL_HANDLE;
        VkResult res = vpCreateDevice(scaffold.physicalDevice, &profile, &info, nullptr, &device);
        if (res != VK_SUCCESS) {
            ++error;
            ASSERT_TRUE(device == VK_NULL_HANDLE);
            std::printf("FAILURE: %d\n", res);
        } else {
            vkDestroyDevice(device, nullptr);
            std::printf("SUCCESS!\n");
        }
    }

    EXPECT_EQ(0, error);
}

TEST(test_profile, create_extensions_unsupported) {
    TestScaffold scaffold;

    std::vector<VpProfileProperties> profiles;
    uint32_t profileCount = 0;

    VkResult result_count = vpEnumerateDeviceProfiles(scaffold.physicalDevice, nullptr, &profileCount, nullptr);
    ASSERT_TRUE(result_count == VK_SUCCESS);

    profiles.resize(profileCount);
    VkResult result_profile = vpEnumerateDeviceProfiles(scaffold.physicalDevice, nullptr, &profileCount, &profiles[0]);
    ASSERT_TRUE(result_profile == VK_SUCCESS);
    EXPECT_TRUE(profileCount > 0);

    for (VpProfileProperties profile : profiles) {
        std::printf("Profile supported: %s, version %d\n", profile.profileName, profile.specVersion);
    }

    int error = 0;

    static const char* extensions[] = {"VK_LUNARG_doesnot_exist", "VK_GTRUC_automagic_rendering",
                                       "VK_GTRUC_portability_everywhere"};

    for (const VpProfileProperties& profile : profiles) {
        std::printf("Creating a Vulkan device using profile %s, version %d: ", profile.profileName, profile.specVersion);

        VkDeviceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.pNext = nullptr;
        info.queueCreateInfoCount = 1;
        info.pQueueCreateInfos = &scaffold.queueCreateInfo;
        info.enabledExtensionCount = countof(extensions);
        info.ppEnabledExtensionNames = extensions;
        info.pEnabledFeatures = nullptr;

        VkDevice device = VK_NULL_HANDLE;
        VkResult res = vpCreateDevice(scaffold.physicalDevice, &profile, &info, nullptr, &device);
        if (res != VK_SUCCESS) {
            ASSERT_TRUE(device == VK_NULL_HANDLE);
            std::printf("EXPECTED FAILURE: %d\n", res);
        } else {
            ++error;
            vkDestroyDevice(device, nullptr);
            std::printf("UNEXPECTED SUCCESS\n");
        }
    }

    EXPECT_EQ(0, error);
}

TEST(test_profile, create_features) {
    TestScaffold scaffold;

    std::vector<VpProfileProperties> profiles;
    uint32_t profileCount = 0;

    VkResult result_count = vpEnumerateDeviceProfiles(scaffold.physicalDevice, nullptr, &profileCount, nullptr);
    ASSERT_TRUE(result_count == VK_SUCCESS);

    profiles.resize(profileCount);
    VkResult result_profile = vpEnumerateDeviceProfiles(scaffold.physicalDevice, nullptr, &profileCount, &profiles[0]);
    ASSERT_TRUE(result_profile == VK_SUCCESS);
    EXPECT_TRUE(profileCount > 0);

    for (VpProfileProperties profile : profiles) {
        std::printf("Profile supported: %s, version %d\n", profile.profileName, profile.specVersion);
    }

    int error = 0;

    for (const VpProfileProperties& profile : profiles) {
        std::printf("Creating a Vulkan device using profile %s, version %d: ", profile.profileName, profile.specVersion);

        VkPhysicalDeviceFeatures enabledFeatures = {};
        enabledFeatures.robustBufferAccess = VK_TRUE;

        VkDeviceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.pNext = nullptr;
        info.queueCreateInfoCount = 1;
        info.pQueueCreateInfos = &scaffold.queueCreateInfo;
        info.enabledExtensionCount = 0;
        info.ppEnabledExtensionNames = nullptr;
        info.pEnabledFeatures = &enabledFeatures;

        VkDevice device = VK_NULL_HANDLE;
        VkResult res = vpCreateDevice(scaffold.physicalDevice, &profile, &info, nullptr, &device);
        if (res != VK_SUCCESS) {
            ++error;
            ASSERT_TRUE(device == VK_NULL_HANDLE);
            std::printf("FAILURE: %d\n", res);
        } else {
            std::printf("SUCCESS!\n");
        }
    }

    EXPECT_EQ(0, error);
}