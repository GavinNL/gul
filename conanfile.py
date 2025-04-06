from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy
import conan
import os
import sys


class ConanTutorialRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    #generators = "CMakeDeps", "CMakeToolchain"

    #preset_name = os.getenv('CONAN_PRESET_NAME')
    
    def requirements(self):
        self.requires("catch2/3.6.0");
        self.requires("glm/0.9.9.8");
        self.requires("stb/cci.20240213");
        self.requires("nlohmann_json/3.11.3");
        self.requires("meshoptimizer/0.20");

    def layout(self):
        '''
            By default, this will set up a "build" folder in the src folder and
            create the build files in a folder that is dependent on the compiler/version/buildtype
            
            For the CI environment, you can custom define the preset name so that
            the folder that is created has a single name to avoid having to 
            add create conditoinals in your CI logic because of the preset name

            conan install . -s build_type=Debug --build missing -c:h "user:preset_name=ci" 
        '''
        # This makes sure that the generated files are
        # generated in the current working directory
        #self.preset_name = self.conf.get("user:preset_name", default=None)

        #print(self.build_folder)
        #if self.preset_name is None:
        #    self.folders.build_folder_vars = ["settings.compiler", "settings.compiler.version", "settings.build_type"]
        #else:
        #    self.folders.build_folder_vars = ["self.preset_name"]

        #cmake_layout(self)
        

    def generate(self):
        pass
        #deps = CMakeDeps(self)
        #deps.generate()
        #tc = CMakeToolchain(self)
        #tc.generate()

        # Used to copy additional files from a recipe into the build folder
        #copy(self, "res/*", self.dependencies["imgui"].package_folder, os.path.join(self.build_folder, "imgui_src") )


