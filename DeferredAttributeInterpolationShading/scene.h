#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_SCENE
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_SCENE

#include <vector>

#include <glm/vec3.hpp>
#include <tools.h>

inline std::vector<glm::vec3> createSphereGeometry(float radius, int slices) {
    std::vector<glm::vec3> vertices;
    Tools::Mesh::CreateSphereVertexMesh(vertices, radius, slices, slices);
    for (int i = 1; i < vertices.size(); i += 3)
        std::swap(vertices[i], vertices[i + 1]); // TODO: fix CW order bug
    return vertices;
}

struct Light
{
    glm::vec4 position; // (x, y, z, radius)
    glm::vec4 color;    // (diffuse.rgb, specular)
};

struct Scene
{
    constexpr static auto MAX_LIGHTS = 8192;

    struct Lights
    {
        GLuint storageBuffer = 0; // storage buffer with lights and lightCount
        GLuint vertexArray
          = 0; // Vertex array with light sphere (attenuation) geometry
        int numLights = 256; // Number of lights currently used in the scene
        float rotationSpeed = 0.005;       // Speed of light moving (rotation)
        glm::vec2 rangeLimits{0.2f, 2.0f}; // Light range limits
        std::vector<Light> lights;
        bool rotate = true;           // Rotate lights
        bool showLightCenters = true; // Render light centers
        bool showLightRanges = false; // Render light ranges

        // 16 bytes for lightCount + padding since the Light array's
        // alignment in std430 layout is 16 == alignof(Light)
        constexpr static auto arrayGPUMemoryOffset = 16;

        struct LightRangesData
        {
            GLuint program = 0;
            GLuint vertexArray = 0;
            GLsizei numVertices = 0;

            LightRangesData();
        } ranges{};

        explicit Lights(float maxDistanceFromWorldOrigin) {
            create(maxDistanceFromWorldOrigin);
        }
        void create(float maxDistanceFromWorldOrigin);
        void update();
        void genRandomRadiuses();

        template<bool UseOwnShader = true>
        void renderLightRanges() {
            if constexpr (UseOwnShader) {
                glUseProgram(ranges.program);
                glBindVertexArray(ranges.vertexArray);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDrawArraysInstanced(GL_TRIANGLES, 0, ranges.numVertices,
                                      numLights);
                glDisable(GL_BLEND);
            } else {
                glBindVertexArray(ranges.vertexArray);
                glDrawArraysInstanced(GL_TRIANGLES, 0, ranges.numVertices,
                                      numLights);
            }
        }

        void renderLightCenters();

        void render() {
            if (showLightCenters) renderLightCenters();
            if (showLightRanges) renderLightRanges();
        }

    private:
        void createBufferAndVertexArray();
    } lights;

    struct Spheres
    {
        int numSpheresPerRow = 20;
        int numSphereSlices = 10;
        // is updated by prepareGeometry
        int trianglesPerSphere = 0;

        GLuint vertexArray = 0;
        GLuint vertexBuffer = 0;
        GLuint sphereOffsetsBuffer = 0;
        GLuint sphereTexture = 0;
        GLsizei numSlices = 0;
        std::vector<glm::vec4> sphereOffsets;
        std::vector<glm::vec3> sphereVertices;

        Spheres(int numSpheresPerRow, int numSphereSlices)
          : numSpheresPerRow(numSpheresPerRow),
            numSphereSlices(numSphereSlices),
            sphereTexture(Tools::Texture::CreateFromFile(
              "../../common/textures/metal01.jpg")) {
            updateGeometry();
        }
        void updateGeometry();
        void render();
    } spheres;

    ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    Scene(Scene&&) = default;
    Scene& operator=(Scene&&) = default;

    void update() { lights.update(); }

    static Scene& get() {
        static Scene scene{};
        return scene;
    }

private:
    explicit Scene(int numSpheresPerRow = 5, int numSphereSlices = 20)
      : lights(static_cast<float>(numSpheresPerRow)),
        spheres(numSpheresPerRow, numSphereSlices) {}
};

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_SCENE */
