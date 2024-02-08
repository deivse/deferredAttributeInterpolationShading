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
    glm::ivec2 WindowResolution = {0, 0};
    glm::ivec2 MSAAResolution = {0, 0};

    // 0 = color, 1 = normal, 2 = position
    constexpr static std::array<GLenum, 3> colorAttachments{
      GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    std::array<GLenum, colorAttachments.size()> colorTextureFormats{
      GL_RGBA8, GL_RGBA32F, GL_RGBA32F};
    std::array<GLuint, colorAttachments.size()> colorTextures{};
    GLuint depthStencilTex{};

    UniformBufferObject<CommonUniformBufferData> uniformBuffer;

    GLuint getTextureForAttachment(GLenum colorAttachment) {
        return colorTextures[static_cast<GLuint>(colorAttachment)
                             - static_cast<GLuint>(GL_COLOR_ATTACHMENT0)];
    }
    void createFramebuffers(const glm::ivec2& resolution);
    void showGBufferTextures();

public:
    ~DeferredShading();

    uint8_t getMSAASampleCount() const { return MSAASampleCount; }
    void setMSAASampleCount(uint8_t numSamples);

    void windowResized(const glm::ivec2& resolution) {
        WindowResolution = resolution;
        MSAAResolution
          = static_cast<int>(MSAASampleCount == 0 ? 1 : MSAASampleCount)
            * WindowResolution;
        createFramebuffers(resolution);
    }
    void initialize();
    void debug() { showGBufferTextures(); };

    friend DeferredShading::AlgorithmCRTPBaseT;
};
} // namespace Algorithms

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING */
