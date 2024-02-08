#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_ATTRIBUTE_INTERPOLATION_SHADING
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_ATTRIBUTE_INTERPOLATION_SHADING

#include "algorithms/uniform_buffer.h"
#include <algorithm.h>

#include <glbinding/gl/gl.h>
#include <layout_constants.h>

namespace Algorithms {
class DeferredAttributeInterpolationShading
  : public Algorithm<DeferredAttributeInterpolationShading>
{
    // NOLINTNEXTLINE(cert-err58-cpp)
    inline static const std::string name{"D.A.I.S."};
    GLsizei hashTableSize = 8192; // TODO: make this a GUI parameter

    GLuint emptyVAO = 0;
    GLuint FBO = 0;

    constexpr static size_t TRIANGLE_SIZE = 72;
    constexpr static size_t MAX_TRIANGLE_COUNT = 1036800;

    struct UniformBufferData : public CommonUniformBufferData
    {
        glm::mat4x4 MVPInverse;
        glm::vec4 viewport;
        /// @brief triangleID & bitwiseModHashSize == triangleID % hashTableSize
        GLint bitwiseModHashSize;
        GLuint numTrianglesPerSphere;
        GLfloat projectionMatrix_32;
        GLfloat projectionMatrix_22;
        GLuint numSamples;
    };
    UniformBufferObject<UniformBufferData> uniformBuffer;
    GLuint triangleSSBO = 0, derivativeSSBO = 0;
    GLuint atomicCounterBuffer = 0;

    // Cache and Locks for geometry sampling stage.
    GLuint cacheTexture = 0, locksTexture = 0;

    GLuint triangleAddressFBOTexture = 0, triangleAddressFBOTextureMS = 0,
           FBOdepthTexture = 0;

    OptionsMap options;
    std::vector<RenderPass> renderPasses;

    uint8_t MSAASampleCount = 4;

    void createAtomicCounterBuffer();
    void createUniformBuffer() {
        logDebug("Creating settings uniform buffer...");
        uniformBuffer.initialize(layout::UniformBuffers::DAIS_Uniforms,
                                 std::nullopt);
    }
    void createHashTableResources();
    void createSSBO(GLuint& ssbo, layout::ShaderStorageBuffers binding);
    void createFBO(const glm::ivec2& resolution);

    void resetHashTable();

public:
    void setMSAASampleCount(uint8_t numSamples);
    uint8_t getMSAASampleCount() const { return MSAASampleCount; }

    ~DeferredAttributeInterpolationShading();

    void windowResized(const glm::ivec2& resolution) { createFBO(resolution); }
    void initialize();
    void debug(){};

    friend DeferredAttributeInterpolationShading::AlgorithmCRTPBaseT;
};
} // namespace Algorithms

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_ATTRIBUTE_INTERPOLATION_SHADING \
        */
