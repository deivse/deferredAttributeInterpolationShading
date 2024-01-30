#include "deferred_attribute_interpolation_shading.h"
#include "option_declaration_macros.h"
#include "scene.h"

#include <glm/vec2.hpp>
#include <common.h>

#include <layout_constants.h>

namespace Algorithms {

void DeferredAttributeInterpolationShading::initialize() {
    DECLARE_OPTION(restoreDepth, true);
    DECLARE_SHADER_ONLY_OPTION(discardPixelsWithoutGeometry, true);

    logDebug("Initializing");
    createBuffers(Variables::WindowSize);

    glLineWidth(2.0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    GLuint emptyVAO = 0;
    glGenVertexArrays(1, &emptyVAO);

    // Add rendering passes
    renderPasses.emplace_back(
      "Depth Prepass",
      [&]() -> void {
          glEnable(GL_DEPTH_TEST);
          glBindFramebuffer(GL_FRAMEBUFFER, 0);
          glClear(GL_DEPTH_BUFFER_BIT);

          Scene::get().update();
          Scene::get().spheres.render();
      },
      "01_dais_depth_prepass");

    
}

void DeferredAttributeInterpolationShading::createBuffers(const glm::ivec2& resolution) {
    logDebug("(Re)creating Buffers ...");

    // glDeleteTextures(1, &depthStencilTex);
    // Tools::Texture::Create2D(depthStencilTex, gl::GLenum::GL_DEPTH24_STENCIL8,
    //                          resolution);

    // glDeleteTextures(static_cast<int>(colorTextures.size()),
    //                  colorTextures.data());

    // for (uint8_t i = 0; i < static_cast<uint8_t>(colorAttachments.size());
    //      i++) {
    //     Tools::Texture::Create2D(colorTextures[i], colorTextureFormats[i],
    //                              resolution);
    // }

    // // Create a framebuffer object ...
    // glDeleteFramebuffers(1, &gBufferFBO);
    // glCreateFramebuffers(1, &gBufferFBO);
    // glNamedFramebufferDrawBuffers(gBufferFBO, colorAttachments.size(),
    //                               colorAttachments.data());

    // // and attach color and depth textures
    // for (size_t i = 0; i < colorAttachments.size(); i++) {
    //     glNamedFramebufferTexture(gBufferFBO, colorAttachments[i],
    //                               colorTextures[i], 0);
    // }
    // glNamedFramebufferTexture(
    //   gBufferFBO, gl::GLenum::GL_DEPTH_STENCIL_ATTACHMENT, depthStencilTex, 0);

    // assert(glGetError() == GL_NO_ERROR);
    // assert(glCheckNamedFramebufferStatus(gBufferFBO, GL_FRAMEBUFFER)
    //        == GL_FRAMEBUFFER_COMPLETE);
}

} // namespace Algorithms
