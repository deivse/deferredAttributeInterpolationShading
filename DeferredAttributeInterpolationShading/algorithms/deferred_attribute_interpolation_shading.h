#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_ATTRIBUTE_INTERPOLATION_SHADING
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_ATTRIBUTE_INTERPOLATION_SHADING

#include <algorithm.h>

#include <glbinding/gl/gl.h>

namespace Algorithms {
class DeferredAttributeInterpolationShading : public Algorithm<DeferredAttributeInterpolationShading>
{
    // NOLINTNEXTLINE(cert-err58-cpp)
    inline static const std::string name{"D.A.I.S."};

    OptionsMap options;
    std::vector<RenderPass> renderPasses;

    void createBuffers(const glm::ivec2& resolution);

public:
    void windowResized(const glm::ivec2& resolution) {
        createBuffers(resolution);
    }
    void initialize();
    void debug() { };

    friend DeferredAttributeInterpolationShading::AlgorithmCRTPBaseT;
};
} // namespace Algorithms

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHMS_DEFERRED_ATTRIBUTE_INTERPOLATION_SHADING \
        */
