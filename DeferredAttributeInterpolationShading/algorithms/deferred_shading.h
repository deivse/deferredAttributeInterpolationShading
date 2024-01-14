#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING

#include <algorithm.h>

namespace Algorithms {
class DeferredShading : public Algorithm<DeferredShading>
{
    // NOLINTNEXTLINE(cert-err58-cpp)
    inline static const std::string name{"DeferredShading"};

    OptionsMap options;
    std::vector<RenderPass> renderPasses;

    GLuint gBufferFBO = 0;
    constexpr static std::array<GLenum, 3> colorAttachments{
      GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
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

    friend class Algorithm<DeferredShading>;
};
} // namespace Algorithms

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING */
