#include <scene.h>

void Scene::Lights::create(float maxDistanceFromWorldOrigin) {
    // Create random light
    lights.resize(MAX_LIGHTS);
    for (int i = 0; i < MAX_LIGHTS; i++) {
        const float positionRange
          = glm::linearRand(1.0f, maxDistanceFromWorldOrigin);
        const glm::vec3 position
          = glm::normalize(glm::linearRand(glm::vec3(-1.0f), glm::vec3(1.0f)))
            * positionRange;
        const float range = glm::linearRand(rangeLimits.x, rangeLimits.y);

        lights[i].position = glm::vec4(position, range);
        lights[i].color
          = glm::vec4(glm::linearRand(glm::vec3(0.0f), glm::vec3(0.5f)), 0.10f);
    }

    // Create buffer with lights and bind it as GL_UNIFORM_BUFFER to 0
    // binding point
    glDeleteBuffers(1, &uniformBuffer);
    glCreateBuffers(1, &uniformBuffer);
    glNamedBufferStorage(uniformBuffer, sizeof(Light) * lights.size(),
                         lights.data(), GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniformBuffer);

    // Create vertex array to display all lights
    glDeleteVertexArrays(1, &vertexArray);
    glCreateVertexArrays(1, &vertexArray);
    glVertexArrayVertexBuffer(vertexArray, 0, uniformBuffer, 0,
                              2 * sizeof(glm::vec4));
    glVertexArrayAttribBinding(vertexArray, 0, 0);
    glVertexArrayAttribFormat(vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(vertexArray, 0);
}

Scene::Lights::LightRanges::LightRanges() {
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
    static GLuint program = 0;
    static GLuint vertexArray = 0;
    if (program == 0) {
        Tools::Shader::CreateShaderProgramFromFile(program, "light_center.vert",
                                                   nullptr, nullptr, nullptr,
                                                   "light.frag");
        glCreateVertexArrays(1, &vertexArray);
    }
    glUseProgram(program);
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_POINTS, 0, numLights);
}

void Scene::Lights::updatePositions() {
    if (!rotate) return;

    const float cosAngle = glm::cos(rotationSpeed);
    const float sinAngle = glm::sin(rotationSpeed);
    for (auto& light : lights) {
        light.position.x
          = light.position.x * cosAngle + light.position.z * sinAngle;
        light.position.z
          = -light.position.x * sinAngle + light.position.z * cosAngle;
    }
    // Update light buffer
    glNamedBufferSubData(uniformBuffer, 0, sizeof(Light) * lights.size(),
                         lights.data());
}

void Scene::Lights::updateRadiuses() {
    for (auto& light : lights) {
        light.position.w = glm::linearRand(rangeLimits.x, rangeLimits.y);
    }
    // Update light buffer
    glNamedBufferSubData(uniformBuffer, 0, sizeof(Light) * lights.size(),
                         lights.data());
}

void Scene::Spheres::render() {
    // Create texture for scene
    static GLuint texture
      = Tools::Texture::CreateFromFile("../common/textures/metal01.jpg");

    // Calculate sphere positions
    const auto numSpheres = static_cast<size_t>(numSpheresPerRow)
                            * numSpheresPerRow * numSpheresPerRow;
    if (numSpheres != spherePositions.size()) {
        spherePositions.clear();
        for (size_t i = 0; i < numSpheres; i++) {
            const int x = i % numSpheresPerRow;
            const int y = i / numSpheresPerRow % numSpheresPerRow;
            const int z
              = i / (numSpheresPerRow * numSpheresPerRow) % numSpheresPerRow;
            spherePositions.push_back(
              glm::vec3(x, y, z) - glm::vec3((numSpheresPerRow - 1) * 0.5f));
        }

        // glDeleteBuffers(1, &attribBuffer);
        // glCreateBuffers(1, &attribBuffer);
        // glNamedBufferStorage(attribBuffer, spherePositions.size() *
        // sizeof(glm::vec3), &spherePositions[0].x, GL_NONE);
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

    glBindTextureUnit(0, texture);
    glBindVertexArray(vertexArray);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(sphereVertices.size()));

    glBindVertexArray(0);
}
