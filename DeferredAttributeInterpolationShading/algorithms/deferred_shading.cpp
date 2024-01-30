#include "deferred_shading.h"
#include "option_declaration_macros.h"

#include "models/cornell_box.h"
#include "scene.h"

#include <glm/vec2.hpp>
#include <common.h>

#include <layout_constants.h>

namespace Algorithms {

void DeferredShading::initialize() {
    DECLARE_OPTION(restoreDepth, true);
    DECLARE_SHADER_ONLY_OPTION(discardPixelsWithoutGeometry, true);

    logDebug("Initializing");
    createGBuffer(Variables::WindowSize);

    glLineWidth(2.0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    GLuint emptyVAO = 0;
    glGenVertexArrays(1, &emptyVAO);

    // Add rendering passes
    renderPasses.emplace_back(
      "Fill G-Buffer",
      [&]() -> void {
          glEnable(GL_DEPTH_TEST);
          glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

          Scene::get().update();
          Scene::get().spheres.render();
      },
      "01_ds_gbuffer");

    renderPasses.emplace_back(
      "Deferred Shading",
      [this, emptyVAO]() {
          glBindFramebuffer(GL_FRAMEBUFFER, 0);
          glClear(GL_COLOR_BUFFER_BIT);
          glDisable(GL_DEPTH_TEST);

          const auto cameraPosition = Variables::Transform.ModelViewInverse
                                      * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
          glUniform3fv(layout::location(layout::Uniforms::CameraPosition), 1,
                       &cameraPosition.x);

          for (auto attachment : colorAttachments) {
              glBindTextureUnit(
                layout::location(
                  layout::texSamplerForFBOAttachment(attachment)),
                getTextureForAttachment(attachment));
          }

          Scene::get().lights.setUniforms();

          glBindVertexArray(emptyVAO);
          glDrawArrays(GL_TRIANGLES, 0, 3);

          glEnable(GL_DEPTH_TEST);
      },
      "02_ds_deferred");
    renderPasses.emplace_back(
      "Restore Z-Buffer",
      [&]() -> void {
          glBindFramebuffer(GL_READ_FRAMEBUFFER, gBufferFBO);
          glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
          glBlitFramebuffer(0, 0, Variables::WindowSize.x,
                            Variables::WindowSize.y, 0, 0,
                            Variables::WindowSize.x, Variables::WindowSize.y,
                            GL_DEPTH_BUFFER_BIT, GL_NEAREST);
          glBindFramebuffer(GL_FRAMEBUFFER, 0);
      },
      &restoreDepth);
}

void DeferredShading::showGBufferTextures() {
    using Variables::WindowSize;
    const auto numColorTextures = static_cast<int>(colorTextures.size());
    const auto numTextures = numColorTextures + 1;

    const auto aspectRatio
      = static_cast<float>(WindowSize.x) / static_cast<float>(WindowSize.y);

    const auto texWidth = WindowSize.x / numTextures;
    const auto texHeight
      = static_cast<int>(static_cast<float>(texWidth) / aspectRatio);
    const auto xOffset = WindowSize.x - texWidth;

    for (int i = 0; i < numColorTextures; i++) {
        Tools::Texture::Show2D(colorTextures[i], xOffset, texHeight * i,
                               texWidth, texHeight);
    }

    Tools::Texture::ShowDepth(depthStencilTex, xOffset,
                              texHeight * numColorTextures, texWidth, texHeight,
                              0, 1);
}

void DeferredShading::createGBuffer(const glm::ivec2& resolution) {
    logDebug("Creating GBuffer ...");

    glDeleteTextures(1, &depthStencilTex);
    Tools::Texture::Create2D(depthStencilTex, gl::GLenum::GL_DEPTH24_STENCIL8,
                             resolution);

    glDeleteTextures(static_cast<int>(colorTextures.size()),
                     colorTextures.data());

    for (uint8_t i = 0; i < static_cast<uint8_t>(colorAttachments.size());
         i++) {
        Tools::Texture::Create2D(colorTextures[i], colorTextureFormats[i],
                                 resolution);
    }

    // Create a framebuffer object ...
    glDeleteFramebuffers(1, &gBufferFBO);
    glCreateFramebuffers(1, &gBufferFBO);
    glNamedFramebufferDrawBuffers(gBufferFBO, colorAttachments.size(),
                                  colorAttachments.data());

    // and attach color and depth textures
    for (size_t i = 0; i < colorAttachments.size(); i++) {
        glNamedFramebufferTexture(gBufferFBO, colorAttachments[i],
                                  colorTextures[i], 0);
    }
    glNamedFramebufferTexture(
      gBufferFBO, gl::GLenum::GL_DEPTH_STENCIL_ATTACHMENT, depthStencilTex, 0);

    assert(glGetError() == GL_NO_ERROR);
    assert(glCheckNamedFramebufferStatus(gBufferFBO, GL_FRAMEBUFFER)
           == GL_FRAMEBUFFER_COMPLETE);
}

} // namespace Algorithms
