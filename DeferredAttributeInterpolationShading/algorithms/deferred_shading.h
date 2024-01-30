#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING

#include <algorithm.h>

#include <glbinding/gl/gl.h>

namespace Algorithms {
class DeferredShading : public Algorithm<DeferredShading>
{
    // NOLINTNEXTLINE(cert-err58-cpp)
    inline static const std::string name{"DeferredShading"};

    OptionsMap options;
    std::vector<RenderPass> renderPasses;

    GLuint gBufferFBO = 0;

    // 0 = color, 1 = normal, 2 = position
    constexpr static std::array<GLenum, 3> colorAttachments{
      GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    std::array<GLenum, colorAttachments.size()> colorTextureFormats{
      GL_RGBA8, GL_RGBA32F, GL_RGBA32F};
    std::array<GLuint, colorAttachments.size()> colorTextures{};
    GLuint depthStencilTex{};

    GLuint getTextureForAttachment(GLenum colorAttachment) {
        return colorTextures[static_cast<GLuint>(colorAttachment)
                             - static_cast<GLuint>(GL_COLOR_ATTACHMENT0)];
    }
    void createGBuffer(const glm::ivec2& resolution);
    void showGBufferTextures();

public:
    void windowResized(const glm::ivec2& resolution) {
        createGBuffer(resolution);
    }
    void initialize();
    void debug() { showGBufferTextures(); };

    friend DeferredShading::AlgorithmCRTPBaseT;
};
} // namespace Algorithms

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING */
