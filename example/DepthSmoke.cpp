#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <projector/ProjectorVK.h>

namespace {

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

Vec3 sub(const Vec3& a, const Vec3& b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3 cross(const Vec3& a, const Vec3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

float dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 normalize(Vec3 v)
{
    const float len = std::sqrt(dot(v, v));
    if (len == 0.0f)
        return {};
    return { v.x / len, v.y / len, v.z / len };
}

struct PyramidMesh
{
    std::vector<float> vertices;
    std::vector<uint32_t> faces;
    std::vector<float> centers;
    std::vector<float> normals;
};

PyramidMesh makeTriangularPyramid()
{
    const std::array<Vec3, 4> points = {
        Vec3{ -0.9f, -0.7f, 2.0f },
        Vec3{  0.9f, -0.7f, 2.0f },
        Vec3{  0.0f,  0.9f, 2.0f },
        Vec3{  0.0f,  0.0f, 3.2f },
    };

    std::vector<std::array<uint32_t, 3>> faces = {
        { 0, 2, 1 },
        { 0, 1, 3 },
        { 1, 2, 3 },
        { 2, 0, 3 },
    };

    Vec3 meshCenter{};
    for (const auto& p : points) {
        meshCenter.x += p.x;
        meshCenter.y += p.y;
        meshCenter.z += p.z;
    }
    meshCenter.x /= static_cast<float>(points.size());
    meshCenter.y /= static_cast<float>(points.size());
    meshCenter.z /= static_cast<float>(points.size());

    PyramidMesh mesh;
    mesh.vertices.reserve(points.size() * 3);
    for (const auto& p : points) {
        mesh.vertices.push_back(p.x);
        mesh.vertices.push_back(p.y);
        mesh.vertices.push_back(p.z);
    }

    for (auto& face : faces) {
        const Vec3& a = points[face[0]];
        const Vec3& b = points[face[1]];
        const Vec3& c = points[face[2]];
        Vec3 center{
            (a.x + b.x + c.x) / 3.0f,
            (a.y + b.y + c.y) / 3.0f,
            (a.z + b.z + c.z) / 3.0f,
        };

        Vec3 normal = normalize(cross(sub(b, a), sub(c, a)));
        if (dot(normal, sub(center, meshCenter)) < 0.0f) {
            std::swap(face[1], face[2]);
            normal = normalize(cross(sub(points[face[1]], points[face[0]]), sub(points[face[2]], points[face[0]])));
        }

        mesh.faces.push_back(face[0]);
        mesh.faces.push_back(face[1]);
        mesh.faces.push_back(face[2]);
        mesh.centers.push_back(center.x);
        mesh.centers.push_back(center.y);
        mesh.centers.push_back(center.z);
        if (mesh.faces.size() == 3) {
            mesh.normals.push_back(0.0f);
            mesh.normals.push_back(0.0f);
            mesh.normals.push_back(1.0f);
        }
        else {
            mesh.normals.push_back(0.0f);
            mesh.normals.push_back(0.0f);
            mesh.normals.push_back(-1.0f);
        }
    }

    return mesh;
}

struct ProjectResult
{
    int validDepthPixels = 0;
    int visibleFaces = 0;
    float minDepth = FLT_MAX;
    float maxDepth = 0.0f;
};

ProjectResult runProject(LYJ_VK::ProjectorVK& projector,
    LYJ_VK::ProjectorCacheVK& cache,
    const PyramidMesh& mesh,
    int w,
    int h,
    float csTh,
    const std::string& imageName)
{
    std::vector<uint32_t> fIdsOut(w * h, UINT_MAX);
    std::vector<float> depthsOut(w * h, FLT_MAX);
    std::vector<char> pValidsOut(mesh.vertices.size() / 3, 0);
    std::vector<char> fValidsOut(mesh.faces.size() / 3, 0);
    std::vector<float> Tcw = {
        1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f,
    };

    projector.project(cache, Tcw.data(), depthsOut.data(), fIdsOut.data(),
        pValidsOut.data(), fValidsOut.data(), 0.1f, 10.0f, csTh, 0.05f);

    ProjectResult result;
    std::vector<unsigned char> depthVis(static_cast<size_t>(w) * h, 0);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float d = depthsOut[y * w + x];
            if (d == FLT_MAX || d <= 0.0f)
                continue;
            ++result.validDepthPixels;
            result.minDepth = std::min(result.minDepth, d);
            result.maxDepth = std::max(result.maxDepth, d);
            const float normalized = std::clamp((10.0f - d) / 10.0f, 0.0f, 1.0f);
            depthVis[static_cast<size_t>(y) * w + x] = static_cast<unsigned char>(normalized * 255.0f);
        }
    }

    for (char visible : fValidsOut) {
        if (visible)
            ++result.visibleFaces;
    }

    const std::filesystem::path outPath = std::filesystem::path(VULKAN_LYJ_HOME_PATH) / "Output" / imageName;
    std::filesystem::create_directories(outPath.parent_path());
    std::ofstream out(outPath, std::ios::binary);
    out << "P5\n" << w << " " << h << "\n255\n";
    out.write(reinterpret_cast<const char*>(depthVis.data()), static_cast<std::streamsize>(depthVis.size()));
    if (!out)
        throw std::runtime_error("failed to write depth image");

    return result;
}

} // namespace

int main()
{
    const int w = 512;
    const int h = 512;
    PyramidMesh mesh = makeTriangularPyramid();

    std::vector<float> camParams = { 260.0f, 260.0f, w / 2.0f, h / 2.0f };

    LYJ_VK::ProjectorVK projector;
    if (!projector.create(mesh.vertices.data(), static_cast<unsigned int>(mesh.vertices.size() / 3),
        mesh.centers.data(), mesh.normals.data(), mesh.faces.data(), static_cast<unsigned int>(mesh.faces.size() / 3),
        camParams.data(), w, h)) {
        throw std::runtime_error("failed to create ProjectorVK");
    }

    LYJ_VK::ProjectorCacheVK cache(static_cast<unsigned int>(mesh.vertices.size() / 3),
        static_cast<unsigned int>(mesh.faces.size() / 3), w, h);

    const ProjectResult noCull = runProject(projector, cache, mesh, w, h, 1.1f, "pyramid_no_normal_cull.pgm");
    const ProjectResult normalCull = runProject(projector, cache, mesh, w, h, 0.0f, "pyramid_normal_cull.pgm");

    std::cout << "pyramid no normal cull: pixels=" << noCull.validDepthPixels
              << ", visible faces=" << noCull.visibleFaces
              << ", depth=[" << noCull.minDepth << ", " << noCull.maxDepth << "]" << std::endl;
    std::cout << "pyramid normal cull: pixels=" << normalCull.validDepthPixels
              << ", visible faces=" << normalCull.visibleFaces
              << ", depth=[" << normalCull.minDepth << ", " << normalCull.maxDepth << "]" << std::endl;
    std::cout << "outputs: " << (std::filesystem::path(VULKAN_LYJ_HOME_PATH) / "Output" / "pyramid_no_normal_cull.pgm").string()
              << ", " << (std::filesystem::path(VULKAN_LYJ_HOME_PATH) / "Output" / "pyramid_normal_cull.pgm").string()
              << std::endl;

    cache.release();
    projector.release();
    LYJ_VK::GetLYJVKInstance()->clean();

    if (noCull.validDepthPixels <= 0 || normalCull.validDepthPixels <= 0)
        return 2;
    return normalCull.maxDepth > noCull.maxDepth + 0.5f ? 0 : 3;
}
