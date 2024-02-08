#include "deferred_shading.h"
#include "option_declaration_macros.h"

#include "models/cornell_box.h"
#include "scene.h"

#include <glm/vec2.hpp>
#include <common.h>

#include <layout_constants.h>

namespace Algorithms {

DeferredShading::~DeferredShading() {
    glDeleteVertexArrays(1, &emptyVAO);
    glDeleteFramebuffers(1, &gBufferFBO);
    glDeleteTextures(1, &depthStencilTex);
    glDeleteTextures(static_cast<int>(colorTextures.size()),
                     colorTextures.data());
}

void DeferredShading::initialize() {
    DECLARE_OPTION(restoreDepth, true);
    DECLARE_SHADER_ONLY_OPTION(discardPixelsWithoutGeometry, true);

    logDebug("Initializing");

    logDebug("Creating uniform buffer...");
    uniformBuffer.initialize(layout::UniformBuffers::DS_Uniforms, std::nullopt);

    WindowResolution = Variables::WindowSize;
    MSAAResolution
      = static_cast<int>(MSAASampleCount == 0 ? 1 : MSAASampleCount)
        * Variables::WindowSize;
    createFramebuffers(Variables::WindowSize);

    glLineWidth(2.0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glGenVertexArrays(1, &emptyVAO);

    renderPasses.emplace_back(
      "Reset uniform buffer.",
      [&]() -> void {
          glViewport(0, 0, MSAAResolution.x, MSAAResolution.y);

          Variables::WindowSize = MSAAResolution;
          Variables::Transform.update();

          auto uniforms = uniformBuffer.mapForWrite();
          uniforms->cameraPosition = Variables::Transform.ModelViewInverse
                                     * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
          uniforms->MVPMatrix = Variables::Transform.ModelViewProjection;
          // Warning: the buffer will be unmapped once `uniforms` goes out of
          // scope
      },
      nullptr);

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
      [this]() {
          // glBindFramebuffer(GL_FRAMEBUFFER, 0);
          // glClear(GL_COLOR_BUFFER_BIT);
          glDisable(GL_DEPTH_TEST);

          for (auto attachment : colorAttachments) {
              glBindTextureUnit(
                layout::location(
                  layout::texSamplerForFBOAttachment(attachment)),
                getTextureForAttachment(attachment));
          }

          glBindVertexArray(emptyVAO);
          glDrawArrays(GL_TRIANGLES, 0, 3);

          glEnable(GL_DEPTH_TEST);
      },
      "02_ds_deferred");
    renderPasses.emplace_back(
      "Restore Z-Buffer",
      [&]() -> void {
          Variables::WindowSize = WindowResolution;
          glViewport(0, 0, Variables::WindowSize.x, Variables::WindowSize.y);
          Variables::Transform.update();

          glBindFramebuffer(GL_READ_FRAMEBUFFER, gBufferFBO);
          glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
          glBlitFramebuffer(0, 0, MSAAResolution.x, MSAAResolution.y, 0, 0,
                            Variables::WindowSize.x, Variables::WindowSize.y,
                            GL_COLOR_BUFFER_BIT, GL_LINEAR);
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

void DeferredShading::setMSAASampleCount(uint8_t numSamples) {
    MSAASampleCount = numSamples;
    MSAAResolution
      = static_cast<int>(MSAASampleCount == 0 ? 1 : MSAASampleCount)
        * WindowResolution;
    createFramebuffers(Variables::WindowSize);
}

void DeferredShading::createFramebuffers(const glm::ivec2& resolution) {
    logDebug("Creating GBuffer ...");

    Tools::Texture::Create2D(depthStencilTex, gl::GLenum::GL_DEPTH24_STENCIL8,
                             MSAAResolution);

    for (uint8_t i = 0; i < static_cast<uint8_t>(colorAttachments.size());
         i++) {
        Tools::Texture::Create2D(colorTextures[i], colorTextureFormats[i],
                                 MSAAResolution, 0);
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
