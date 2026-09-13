"""Apply the actual Conan source() patch to a pre-extracted, hash-checked SDL.
Usage: python tests/check_sdl_recipe.py /path/to/SDL3-3.4.16
The network download is replaced with a no-op; patch code is not duplicated.
"""
import importlib.util
import pathlib
import sys
from conan import ConanFile
from conan.internal.model.layout import Folders

recipe_dir = pathlib.Path(__file__).resolve().parents[1] / 'conan/sdl/3.x'
spec = importlib.util.spec_from_file_location('sdl_recipe', recipe_dir / 'conanfile.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
module.get = lambda *args, **kwargs: None
recipe = module.SDLConan()
recipe.folders = Folders()
recipe.folders.set_base_source(str(pathlib.Path(sys.argv[1]).resolve()))
recipe.folders.set_base_export_sources(str(recipe_dir))
recipe.version = '3.4.16'
recipe.conan_data = {'sources': {'3.4.16': {}}}
recipe.source()
print('Actual Conan SDL source patch applied successfully')
