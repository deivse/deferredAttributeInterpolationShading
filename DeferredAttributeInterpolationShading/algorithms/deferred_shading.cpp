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
    // DECLARE_SHADER_ONLY_OPTION(discardPixelsWithoutGeometry, true);
    DECLARE_OPTION(StoreCoverage, false);

    logDebug("Initializing");

    logDebug("Creating uniform buffer...");
    uniformBuffer.initialize(layout::UniformBuffers::DS_Uniforms, std::nullopt);

    createGBuffer(Variables::WindowSize);

    glLineWidth(2.0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glGenVertexArrays(1, &emptyVAO);

    renderPasses.emplace_back(
      "Reset uniform buffer.",
      [&]() -> void {
          auto uniforms = uniformBuffer.mapForWrite();
          uniforms->cameraPosition = Variables::Transform.ModelViewInverse
                                     * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
          uniforms->MVPMatrix = Variables::Transform.ModelViewProjection;
          uniforms->numSamples = MSAASampleCount;
          // Warning: the buffer will be unmapped once `uniforms` goes out of
          // scope
      },
      nullptr);

    renderPasses.emplace_back(
      "Depth Pre-pass",
      [&]() -> void {
          glEnable(GL_DEPTH_TEST);
          glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
          glClear(GL_DEPTH_BUFFER_BIT);
          Scene::get().update();
          Scene::get().spheres.render();
      },
      "00_ds_depth_pre", &StoreCoverage);

    // Add rendering passes
    renderPasses.emplace_back(
      "Fill G-Buffer",
      [&]() -> void {
          if (StoreCoverage) {
              glDepthFunc(GL_EQUAL);
          } else {
              glEnable(GL_DEPTH_TEST);
              glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
              glClear(GL_DEPTH_BUFFER_BIT);
              Scene::get().update();
          }

          glClear(GL_COLOR_BUFFER_BIT);

          Scene::get().spheres.render();
          glDepthFunc(GL_LEQUAL);
      },
      "01_ds_gbuffer");

    renderPasses.emplace_back(
      "Deferred Shading",
      [this]() {
          glBindFramebuffer(GL_FRAMEBUFFER, 0);
          glClear(GL_COLOR_BUFFER_BIT);
          glDisable(GL_DEPTH_TEST);

          glBindVertexArray(emptyVAO);
          glDrawArrays(GL_TRIANGLES, 0, 3);

          glEnable(GL_DEPTH_TEST);
      },
      "02_ds_deferred");

    renderPasses.emplace_back(
      "Restore Depth",
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
    if (MSAASampleCount > 0) return;

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
    createGBuffer(Variables::WindowSize);
}

template<bool MS>
GLuint DeferredShading::getTextureForAttachment(GLenum colorAttachment) {
    if constexpr (MS) {
        return colorTexturesMS[static_cast<GLuint>(colorAttachment)
                               - static_cast<GLuint>(GL_COLOR_ATTACHMENT0)];
    } else {
        return colorTextures[static_cast<GLuint>(colorAttachment)
                             - static_cast<GLuint>(GL_COLOR_ATTACHMENT0)];
    }
}

void DeferredShading::createGBuffer(const glm::ivec2& resolution) {
    logDebug("Creating GBuffer ...");

    Tools::Texture::Create2D(depthStencilTex, gl::GLenum::GL_DEPTH24_STENCIL8,
                             resolution, MSAASampleCount);

    for (uint8_t i = 0; i < static_cast<uint8_t>(colorAttachments.size());
         i++) {
        Tools::Texture::Create2D(colorTextures[i], colorTextureFormats[i],
                                 MSAASampleCount ? glm::ivec2(1, 1)
                                                 : resolution);
        Tools::Texture::Create2D(colorTexturesMS[i], colorTextureFormats[i],
                                 MSAASampleCount ? resolution
                                                 : glm::ivec2{1, 1},
                                 MSAASampleCount > 0 ? MSAASampleCount : 1);
    }

    // Create a framebuffer object ...
    glDeleteFramebuffers(1, &gBufferFBO);
    glCreateFramebuffers(1, &gBufferFBO);
    glNamedFramebufferDrawBuffers(gBufferFBO, colorAttachments.size(),
                                  colorAttachments.data());

    // and attach color and depth textures
    for (size_t i = 0; i < colorAttachments.size(); i++) {
        glNamedFramebufferTexture(
          gBufferFBO, colorAttachments[i],
          MSAASampleCount > 0 ? colorTexturesMS[i] : colorTextures[i], 0);
    }
    glNamedFramebufferTexture(
      gBufferFBO, gl::GLenum::GL_DEPTH_STENCIL_ATTACHMENT, depthStencilTex, 0);

    assert(glGetError() == GL_NO_ERROR);
    assert(glCheckNamedFramebufferStatus(gBufferFBO, GL_FRAMEBUFFER)
           == GL_FRAMEBUFFER_COMPLETE);

    for (auto attachment : colorAttachments) {
        glBindTextureUnit(
          layout::location(
            layout::texSamplerForFBOAttachment<false>(attachment)),
          getTextureForAttachment<false>(attachment));
        glBindTextureUnit(
          layout::location(
            layout::texSamplerForFBOAttachment<true>(attachment)),
          getTextureForAttachment<true>(attachment));
    }
}

} // namespace Algorithms
