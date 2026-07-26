#include "VulkanInclude.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

int main()
{
	const float camera[4] = { 500.0f, 500.0f, 320.0f, 240.0f };
	const float identity[12] = {
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 0.0f
	};

	std::array<unsigned int, 24> descriptors1{};
	std::array<unsigned int, 24> descriptors2{};
	std::fill(descriptors1.begin() + 8, descriptors1.begin() + 16, 0xffffffffu);
	std::fill(descriptors1.begin() + 16, descriptors1.end(), 0xaaaaaaaau);
	std::fill(descriptors2.begin(), descriptors2.begin() + 8, 0xffffffffu);
	std::fill(descriptors2.begin() + 16, descriptors2.end(), 0xaaaaaaaau);

	LYJ_VK::ORBMatcherCacheVK cache;
	cache.upload1(3, descriptors1.data());
	cache.upload2(3, descriptors2.data());
	LYJ_VK::MatchVKHandle matcher = LYJ_VK::initMatcherVK();
	if (!matcher) {
		std::cerr << "failed to initialize Vulkan ORB matcher" << std::endl;
		return 1;
	}

	short forward[3] = { -1, -1, -1 };
	short reverse[3] = { -1, -1, -1 };
	LYJ_VK::matchBFVK(matcher, cache, forward, reverse);
	const short expected[3] = { 1, 0, 2 };
	for (int i = 0; i < 3; ++i) {
		if (forward[i] != expected[i] || reverse[i] != expected[i]) {
			std::cerr << "unexpected BF match at " << i << ": " << forward[i]
				<< ", reverse " << reverse[i] << std::endl;
			LYJ_VK::releaseMatcherVK(matcher);
			return 2;
		}
	}

	LYJ_VK::matchBFVK(matcher, cache, forward, reverse, 64, 0.8f, 0, 1, 1.0f);
	for (int i = 0; i < 3; ++i) {
		if (forward[i] != -1 || reverse[i] != -1) {
			std::cerr << "missing 3D input was not rejected" << std::endl;
			LYJ_VK::releaseMatcherVK(matcher);
			return 3;
		}
	}
	const float cameraPoints1[9] = {
		0.0f, 0.0f, 1.0f,
		0.1f, 0.0f, 1.0f,
		0.2f, 0.0f, 1.0f
	};
	const float cameraPoints2[9] = {
		0.1f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.2f, 0.0f, 1.0f
	};
	cache.upload1(3, identity, identity, nullptr, descriptors1.data(), cameraPoints1);
	cache.upload2(3, identity, identity, nullptr, nullptr, nullptr, descriptors2.data(), cameraPoints2);
	LYJ_VK::matchBFVK(matcher, cache, forward, reverse, 64, 0.8f, 0, 1, 0.05f);
	for (int i = 0; i < 3; ++i) {
		if (forward[i] != expected[i] || reverse[i] != expected[i]) {
			std::cerr << "unexpected 3D BF match at " << i << std::endl;
			LYJ_VK::releaseMatcherVK(matcher);
			return 4;
		}
	}

	LYJ_VK::releaseMatcherVK(matcher);
	matcher = LYJ_VK::initMatcherVK(640, 480, camera);
	if (!matcher) {
		std::cerr << "failed to initialize camera-aware Vulkan ORB matcher" << std::endl;
		return 5;
	}

	const float Tcw2[12] = {
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.1f, 0.0f, 0.0f
	};
	const float keypoints1[6] = { 100.0f, 240.0f, 320.0f, 240.0f, 500.0f, 240.0f };
	const size_t cellCount = static_cast<size_t>(cache.wGrid_) * cache.hGrid_;
	std::vector<short> featureGrid(cellCount * VKORBEVECELLSIZE, -1);
	std::vector<char> featureGridSizes(cellCount, 0);
	const int featureColumns[3] = { 10, 16, 20 };
	for (int i = 0; i < 3; ++i) {
		const size_t cell = 12 * cache.wGrid_ + featureColumns[i];
		featureGrid[cell * VKORBEVECELLSIZE] = static_cast<short>(i);
		featureGridSizes[cell] = 1;
	}
	cache.upload1(3, identity, identity, keypoints1, descriptors1.data());
	cache.upload2(3, Tcw2, identity, featureGrid.data(), featureGridSizes.data(), nullptr, descriptors2.data());
	LYJ_VK::matchFVK(matcher, cache, forward, reverse, 64, 0.8f, 0, 0, 1.0f);
	for (int i = 0; i < 3; ++i) {
		if (forward[i] != expected[i]) {
			std::cerr << "unexpected epipolar match at " << i << ": " << forward[i] << std::endl;
			LYJ_VK::releaseMatcherVK(matcher);
			return 6;
		}
	}

	const float worldPoints1[9] = {
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f
	};
	const char validWorldPoints[3] = { 1, 1, 1 };
	cache.upload1(3, identity, identity, keypoints1, descriptors1.data(), nullptr, worldPoints1, validWorldPoints);
	std::vector<short> cellData(VKORBKPSIZE, 0);
	std::vector<short> cellOffsets(cellCount + 1, 0);
	const size_t projectedCell = 12 * cache.wGrid_ + 18;
	for (size_t i = projectedCell + 1; i < cellOffsets.size(); ++i)
		cellOffsets[i] = 3;
	cellData[0] = 0;
	cellData[1] = 1;
	cellData[2] = 2;
	LYJ_VK::GridVK grid;
	grid.upload(cellData.data(), cellOffsets.data());
	LYJ_VK::matchProVK(matcher, cache, grid, forward, reverse, 64, 0.8f, 0, 0, 1.0f);
	for (int i = 0; i < 3; ++i) {
		if (forward[i] != expected[i]) {
			std::cerr << "unexpected projection match at " << i << ": " << forward[i] << std::endl;
			LYJ_VK::releaseMatcherVK(matcher);
			return 7;
		}
	}

	LYJ_VK::releaseMatcherVK(matcher);
	std::cout << "ORB matcher smoke test passed" << std::endl;
	return 0;
}
