#include <VulkanInclude.h>

#include <array>
#include <iostream>

int main()
{
	std::array<float, 3 * 128> descriptors1{};
	std::array<float, 3 * 128> descriptors2{};
	descriptors1[0] = 512.0f;
	descriptors1[128 + 1] = 512.0f;
	descriptors1[256 + 2] = 512.0f;
	descriptors2[0] = 1.0f;
	descriptors2[128 + 2] = 1.0f;
	descriptors2[256 + 1] = 1.0f;

	LYJ_VK::SIFTMatcherCacheVK cache;
	cache.upload1(3, descriptors1.data());
	cache.upload2(3, descriptors2.data(), false);
	LYJ_VK::SIFTMatchVKHandle matcher = LYJ_VK::initSIFTMatcherVK();
	if (!matcher) {
		std::cerr << "failed to initialize Vulkan SIFT matcher" << std::endl;
		return 1;
	}

	short forward[3] = { -1, -1, -1 };
	short reverse[3] = { -1, -1, -1 };
	LYJ_VK::matchBFVK(matcher, cache, forward, reverse);

	const short expectedForward[3] = { 0, 2, 1 };
	const short expectedReverse[3] = { 0, 2, 1 };
	for (int i = 0; i < 3; ++i) {
		if (forward[i] != expectedForward[i] || reverse[i] != expectedReverse[i]) {
			std::cerr << "unexpected SIFT BF match at " << i << std::endl;
			LYJ_VK::releaseSIFTMatcherVK(matcher);
			return 1;
		}
	}

	const float identity[12] = {
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 0.0f
	};
	const float points1[9] = {
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		2.0f, 0.0f, 1.0f
	};
	const float points2[9] = {
		0.0f, 0.0f, 1.0f,
		2.0f, 0.0f, 1.0f,
		10.0f, 0.0f, 1.0f
	};
	cache.upload1(3, identity, descriptors1.data(), points1);
	cache.upload2(3, identity, descriptors2.data(), points2, false);
	LYJ_VK::matchBFVK(matcher, cache, forward, reverse, 0.7f, 0.8f, 1, 1, 0.01f);
	if (forward[0] != 0 || forward[1] != -1 || forward[2] != 1 ||
		reverse[0] != 0 || reverse[1] != 2 || reverse[2] != -1) {
		std::cerr << "unexpected 3D-filtered SIFT BF result" << std::endl;
		LYJ_VK::releaseSIFTMatcherVK(matcher);
		return 1;
	}

	std::array<float, 2 * 128> duplicateQueries{};
	std::array<float, 2 * 128> duplicateCandidates{};
	duplicateQueries[0] = 1.0f;
	duplicateQueries[128] = 0.99f;
	duplicateQueries[129] = 0.1f;
	duplicateCandidates[0] = 1.0f;
	duplicateCandidates[128 + 2] = 1.0f;
	cache.upload1(2, duplicateQueries.data());
	cache.upload2(2, duplicateCandidates.data());
	short duplicateForward[2] = { -1, -1 };
	short duplicateReverse[2] = { -1, -1 };
	LYJ_VK::matchBFVK(matcher, cache, duplicateForward, duplicateReverse, 0.7f, 0.8f, 1);
	if (duplicateForward[0] != 0 || duplicateForward[1] != -1) {
		std::cerr << "unexpected mutual-best SIFT BF result" << std::endl;
		LYJ_VK::releaseSIFTMatcherVK(matcher);
		return 1;
	}
	LYJ_VK::matchBFVK(matcher, cache, duplicateForward, duplicateReverse, 0.7f, 0.8f, 0);
	if (duplicateForward[0] != 0 || duplicateForward[1] != 0 || duplicateReverse[0] != 0) {
		std::cerr << "unexpected non-mutual SIFT BF result" << std::endl;
		LYJ_VK::releaseSIFTMatcherVK(matcher);
		return 1;
	}

	LYJ_VK::releaseSIFTMatcherVK(matcher);
	std::cout << "descriptor-only Vulkan SIFT matcher test passed" << std::endl;
	return 0;
}
