#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING

#include <algorithm.h>

namespace Algorithms {
class DefferedShading : public Algorithm<DefferedShading>
{
    // NOLINTNEXTLINE(cert-err58-cpp)
    inline static const std::string name{"DefferedShading"};

    OptionsMap options;
    std::vector<RenderPass> renderPasses;

    GLuint gBufferFBO = 0;
    std::array<GLuint, 4> colorTextures{};
    GLuint depthStencilTex{};

    void createGBuffer(const glm::ivec2& resolution);

public:
    void windowResized(const glm::ivec2& resolution) {
        createGBuffer(resolution);
    }
    void initialize();

    friend class Algorithm<DefferedShading>;
};
} // namespace Algorithms

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_SHADING */
