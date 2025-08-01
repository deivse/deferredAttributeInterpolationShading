#include <scene.h>
#include <layout_constants.h>

#include <spdlog/spdlog.h>

using UniformBuffers = layout::UniformBuffers;

void Scene::Lights::create(float maxDistanceFromWorldOrigin) {
    // Create random light
    lights.resize(numLights);
    for (int i = 0; i < numLights; i++) {
        const float radius = glm::linearRand(1.0f, maxDistanceFromWorldOrigin);
        const glm::vec3 position
          = glm::normalize(glm::linearRand(glm::vec3(-1.0f), glm::vec3(1.0f)))
            * radius;
        const float range = glm::linearRand(rangeLimits.x, rangeLimits.y);

        lights[i].position = glm::vec4(position, range);
        lights[i].color
          = glm::vec4(glm::linearRand(glm::vec3(0.1f), glm::vec3(0.8f)), 0.45f);
    }

    createBufferAndVertexArray();
}

void Scene::Lights::createBufferAndVertexArray() {
    // Create buffer with lights and bind it as GL_UNIFORM_BUFFER to 0
    // binding point
    glDeleteBuffers(1, &storageBuffer);
    glCreateBuffers(1, &storageBuffer);

    constexpr auto dataSize
      = Lights::arrayGPUMemoryOffset + sizeof(Light) * MAX_LIGHTS;

    glNamedBufferStorage(storageBuffer, dataSize, nullptr,
                         GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,
                     layout::location(layout::ShaderStorageBuffers::Lights),
                     storageBuffer);

    // Create vertex array to display all lights
    glDeleteVertexArrays(1, &vertexArray);
    glCreateVertexArrays(1, &vertexArray);
    glVertexArrayVertexBuffer(vertexArray, 0, storageBuffer,
                              Lights::arrayGPUMemoryOffset,
                              2 * sizeof(glm::vec4));
    glVertexArrayAttribBinding(vertexArray, 0, 0);
    glVertexArrayAttribFormat(vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(vertexArray, 0);
}

void Scene::Lights::update() {
    if (rotate) {
        const float cosAngle = glm::cos(rotationSpeed);
        const float sinAngle = glm::sin(rotationSpeed);
        for (auto& light : lights) {
            light.position.x
              = light.position.x * cosAngle + light.position.z * sinAngle;
            light.position.z
              = -light.position.x * sinAngle + light.position.z * cosAngle;
        }
    }

    while (true) {
        auto* const lightsWritePtr
          = static_cast<std::byte*>(glMapNamedBufferRange(
            storageBuffer, 0, 16 + sizeof(Light) * (lights.size()),
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

        *reinterpret_cast<GLuint*>(lightsWritePtr) = numLights;
        std::copy(this->lights.begin(), this->lights.end(),
                  reinterpret_cast<Light*>(lightsWritePtr
                                           + Lights::arrayGPUMemoryOffset));

        if (glUnmapNamedBuffer(storageBuffer) == GL_TRUE) {
            break;
        }
        spdlog::warn("Lights storage buffer data store contents have become "
                     "corrupt during the time the data store was mapped, "
                     "reinitializing.");
        createBufferAndVertexArray();
    }
}

void Scene::Lights::genRandomRadiuses() {
    for (auto& light : lights) {
        light.position.w = glm::linearRand(rangeLimits.x, rangeLimits.y);
    }
}

Scene::Lights::LightRangesData::LightRangesData() {
    Tools::Shader::CreateShaderProgramFromFile(
      program, "light_range.vert", nullptr, nullptr, nullptr, "light.frag");
    // Create buffer with sphere geometry
    std::vector<glm::vec3> vertices = createSphereGeometry(0.5f, 20);
    numVertices = static_cast<GLsizei>(vertices.size());
    GLuint vertexBuffer = 0;
    glCreateBuffers(1, &vertexBuffer);
    glNamedBufferStorage(vertexBuffer, vertices.size() * sizeof(glm::vec3),
                         &vertices[0].x, gl::BufferStorageMask::GL_NONE_BIT);

    // Create vertex array for lights' spheres
    glCreateVertexArrays(1, &vertexArray);
    glVertexArrayVertexBuffer(vertexArray, 0, vertexBuffer, 0,
                              sizeof(glm::vec3));
    glVertexArrayAttribBinding(vertexArray, 0, 0);
    glVertexArrayAttribFormat(vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(vertexArray, 0);
}

void Scene::Lights::renderLightCenters() {
    static GLuint program = []() {
        GLuint p = 0;
        Tools::Shader::CreateShaderProgramFromFile(
          p, "light_center.vert", nullptr, nullptr, nullptr, "light.frag");
        return p;
    }();
    static GLuint vertexArray = []() {
        GLuint v = 0;
        glCreateVertexArrays(1, &v);
        return v;
    }();

    glUseProgram(program);
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_POINTS, 0, numLights);
}

void Scene::Spheres::updateGeometry() {
    // Calculate sphere positions
    const auto numSpheres = static_cast<size_t>(numSpheresPerRow)
                            * numSpheresPerRow * numSpheresPerRow;
    if (numSpheres != sphereOffsets.size()) {
        sphereOffsets.clear();
        for (size_t i = 0; i < numSpheres; i++) {
            const int x = i % numSpheresPerRow;
            const int y = i / numSpheresPerRow % numSpheresPerRow;
            const int z
              = i / (numSpheresPerRow * numSpheresPerRow) % numSpheresPerRow;
            sphereOffsets.emplace_back(
              glm::vec3(x, y, z) - glm::vec3((numSpheresPerRow - 1) * 0.5f), 1);
        }

        glDeleteBuffers(1, &sphereOffsetsBuffer);
        glCreateBuffers(1, &sphereOffsetsBuffer);
        glNamedBufferStorage(
          sphereOffsetsBuffer,
          sphereOffsets.size() * sizeof(decltype(sphereOffsets)::value_type),
          &sphereOffsets[0].x, BufferStorageMask::GL_NONE_BIT);
        glBindBufferBase(gl::GLenum::GL_UNIFORM_BUFFER,
                         layout::location(UniformBuffers::SphereOffsets),
                         sphereOffsetsBuffer);
    }

    if ((vertexArray == 0) || (numSlices != numSphereSlices)) {
        numSlices = numSphereSlices;
        glDeleteVertexArrays(1, &vertexArray);
        glDeleteBuffers(1, &vertexBuffer);

        // Create vertex buffer
        sphereVertices = createSphereGeometry(0.5f, numSphereSlices);
        glCreateBuffers(1, &vertexBuffer);
        glNamedBufferStorage(
          vertexBuffer, sphereVertices.size() * sizeof(glm::vec3),
          &sphereVertices[0].x, gl::BufferStorageMask::GL_NONE_BIT);
        // Create vertex array
        glCreateVertexArrays(1, &vertexArray);
        glVertexArrayVertexBuffer(vertexArray, 0, vertexBuffer, 0,
                                  sizeof(glm::vec3));
        glVertexArrayAttribBinding(vertexArray, 0, 0);
        glVertexArrayAttribFormat(vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(vertexArray, 0);
    }
    trianglesPerSphere = static_cast<GLsizei>(sphereVertices.size()) / 3;
}

void Scene::Spheres::render() {
    glBindTextureUnit(layout::location(layout::TextureUnits::Albedo),
                      sphereTexture);
    glBindVertexArray(vertexArray);

    glDrawArraysInstanced(GL_TRIANGLES, 0,
                          static_cast<GLsizei>(sphereVertices.size()),
                          static_cast<GLsizei>(std::pow(numSpheresPerRow, 3)));

    glBindVertexArray(0);
}
