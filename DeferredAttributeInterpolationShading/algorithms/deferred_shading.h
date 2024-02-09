#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING

#include "algorithms/uniform_buffer.h"
#include <algorithm.h>

#include <glbinding/gl/gl.h>

namespace Algorithms {
class DeferredShading : public Algorithm<DeferredShading>
{
    // NOLINTNEXTLINE(cert-err58-cpp)
    inline static const std::string name{"DeferredShading"};

    OptionsMap options;
    std::vector<RenderPass> renderPasses;

    GLuint emptyVAO = 0;
    GLuint gBufferFBO = 0;

    uint8_t MSAASampleCount = 4;

    // 0 = color, 1 = normal, 2 = position
    constexpr static std::array<GLenum, 3> colorAttachments{
      GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    std::array<GLenum, colorAttachments.size()> colorTextureFormats{
      GL_RGBA8, GL_RGBA32F, GL_RGBA32F};
    std::array<GLuint, colorAttachments.size()> colorTextures{};
    std::array<GLuint, colorAttachments.size()> colorTexturesMS{};
    GLuint depthStencilTex{};

    struct UniformBufferData : public CommonUniformBufferData
    {
        GLuint numSamples;
    };
    UniformBufferObject<UniformBufferData> uniformBuffer;

    template<bool MS>
    GLuint getTextureForAttachment(GLenum colorAttachment);

    void createGBuffer(const glm::ivec2& resolution);
    void showGBufferTextures();

public:
    ~DeferredShading();

    uint8_t getMSAASampleCount() const { return MSAASampleCount; }
    void setMSAASampleCount(uint8_t numSamples);

    static size_t customGui() { return 0; }

    void windowResized(const glm::ivec2& resolution) {
        createGBuffer(resolution);
    }
    void initialize();
    void debug() { showGBufferTextures(); };

    friend DeferredShading::AlgorithmCRTPBaseT;
};
} // namespace Algorithms

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING */
