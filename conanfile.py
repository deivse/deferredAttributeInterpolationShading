import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.files import copy


class PGR2Conan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    tool_requires = ["ninja/1.11.1", "cmake/3.25.3"]

    requires = [
        "glm/0.9.9.8",
        "imgui/1.90",
        "implot/0.16",
        "glfw/3.3.8",
        "glew/2.2.0",
        "stb/cci.20230920",
    ]

    cmake_generator = "Ninja Multi-Config"

    def layout(self):
        cmake_layout(self, build_folder="build", generator=self.cmake_generator)

    def generate(self):
        tc = CMakeToolchain(self, self.cmake_generator)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
