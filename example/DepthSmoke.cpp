#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>

#include <projector/ProjectorVK.h>

int main()
{
    const int w = 512;
    const int h = 512;

    const std::vector<float> vertices = {
        -1.0f, -1.0f, 3.0f,
         1.0f, -1.0f, 3.0f,
         1.0f,  1.0f, 3.0f,
        -1.0f,  1.0f, 3.0f,
    };
    const std::vector<uint32_t> faces = {
        0, 1, 2,
        0, 2, 3,
    };
    const std::vector<float> centers = {
        1.0f / 3.0f, -1.0f / 3.0f, 3.0f,
       -1.0f / 3.0f,  1.0f / 3.0f, 3.0f,
    };
    const std::vector<float> normals = {
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
    };
    std::vector<float> camParams = {256.0f, 256.0f, w / 2.0f, h / 2.0f};
    std::vector<float> Tcw = {
        1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f,
    };

    std::vector<uint32_t> fIdsOut(w * h, UINT_MAX);
    std::vector<float> depthsOut(w * h, FLT_MAX);
    std::vector<char> pValidsOut(4, 0);
    std::vector<char> fValidsOut(2, 0);

    LYJ_VK::ProjectorVK projector;
    if (!projector.create(vertices.data(), 4, centers.data(), normals.data(), faces.data(), 2, camParams.data(), w, h))
        throw std::runtime_error("failed to create ProjectorVK");

    LYJ_VK::ProjectorCacheVK cache(4, 2, w, h);
    projector.project(cache, Tcw.data(), depthsOut.data(), fIdsOut.data(), pValidsOut.data(), fValidsOut.data(), 0.1f, 10.0f, 0.0f, 0.1f);

    int validCount = 0;
    float minDepth = FLT_MAX;
    float maxDepth = 0.0f;
    cv::Mat depthVis(h, w, CV_8UC1, cv::Scalar(0));
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const float d = depthsOut[y * w + x];
            if (d == FLT_MAX || d <= 0.0f)
                continue;

            ++validCount;
            minDepth = std::min(minDepth, d);
            maxDepth = std::max(maxDepth, d);
            const float normalized = std::clamp((10.0f - d) / 10.0f, 0.0f, 1.0f);
            depthVis.at<uchar>(y, x) = static_cast<uchar>(normalized * 255.0f);
        }
    }

    const std::string outPath = std::string(VULKAN_LYJ_HOME_PATH) + "/Output/depth_smoke.png";
    if (!cv::imwrite(outPath, depthVis))
        throw std::runtime_error("failed to write depth image");

    std::cout << "depth smoke valid pixels: " << validCount
              << ", min depth: " << minDepth
              << ", max depth: " << maxDepth
              << ", output: " << outPath << std::endl;

    cache.release();
    projector.release();
    LYJ_VK::GetLYJVKInstance()->clean();
    return validCount > 0 ? 0 : 2;
}
