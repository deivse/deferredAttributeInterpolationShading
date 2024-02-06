#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_ATTRIBUTE_INTERPOLATION_SHADING
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_ATTRIBUTE_INTERPOLATION_SHADING

#include <algorithm.h>

#include <glbinding/gl/gl.h>

namespace Algorithms {
class DeferredAttributeInterpolationShading
  : public Algorithm<DeferredAttributeInterpolationShading>
{
    // NOLINTNEXTLINE(cert-err58-cpp)
    inline static const std::string name{"D.A.I.S."};
    GLsizei hashTableSize = 8192*2*2; // TODO: make this a GUI parameter

    GLuint emptyVAO = 0;
    GLuint FBO = 0;

    struct UniformSettingsBuffer
    {
        /// @brief triangleID & bitwiseModHashSize == triangleID % hashTableSize
        int32_t bitwiseModHashSize;
        uint32_t numTrianglesPerSphere;
    };
    GLuint settingsUniformBuffer = 0;
    GLuint triangleSSBO = 0;
    GLuint atomicCounterBuffer = 0;

    // Cache and Locks for geometry sampling stage.
    GLuint cacheTexture = 0, locksTexture = 0;

    GLuint triangleAddressFBOTexture = 0, FBOdepthTexture = 0;

    OptionsMap options;
    std::vector<RenderPass> renderPasses;

    void createAtomicCounterBuffer();
    void createSettingsUniformBuffer(
      std::optional<UniformSettingsBuffer> initialData = std::nullopt);
    void createHashTableResources();
    void createTriangleBuffer();
    void createFBO(const glm::ivec2& resolution);

    void resetHashTable();

public:
    ~DeferredAttributeInterpolationShading();

    void windowResized(const glm::ivec2& resolution) { createFBO(resolution); }
    void initialize();
    void debug(){};

    friend DeferredAttributeInterpolationShading::AlgorithmCRTPBaseT;
};
} // namespace Algorithms

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_ATTRIBUTE_INTERPOLATION_SHADING \
        */
