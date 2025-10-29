from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class LioLivoxRecipe(ConanFile):
    name = "lio_livox"
    version = "0.1"
    settings = "os", "compiler", "build_type", "arch"
    generators = ("CMakeToolchain", "CMakeDeps")
    requires = (
        "eigen/3.4.0",
        "glog/0.7.0",
        "ceres-solver/2.2.0",
        "opencv/4.8.1",
        "pcl/1.13.1",
    )

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
