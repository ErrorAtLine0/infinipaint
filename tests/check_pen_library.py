"""Dependency and pressure/filter wiring; C++ tests exercise the algorithms."""
from pathlib import Path
import re
import subprocess
import unittest
ROOT=Path(__file__).resolve().parents[1]
PIN="adbdce4e902433fcd14fba16e08863f2ec909f79"
def source(p):
    return (ROOT/p).read_text(encoding="utf-8")
class SharedLibraryWiring(unittest.TestCase):
    def test_dependency(self):
        actual=subprocess.check_output(["git","rev-parse","HEAD:deps/pen-stabilizer"],cwd=ROOT,text=True).strip()
        self.assertEqual(actual,PIN)
        self.assertEqual(subprocess.check_output(["git","rev-parse","HEAD"],cwd=ROOT/"deps/pen-stabilizer",text=True).strip(),PIN)
        self.assertIn("../deps/pen-stabilizer/include/pen_stabilizer/stabilizer.hpp",source("src/PenStabilizer.hpp"))
        self.assertNotIn("localNormal",source("src/PenStabilizer.hpp"))
        self.assertIn("target_link_libraries(main pen_stabilizer::pen_stabilizer)",source("CMakeLists.txt"))
    def test_main_link_signature_matches_upstream(self):
        cmake=source("CMakeLists.txt")
        calls=re.findall(r"target_link_libraries\s*\(\s*main\s+([^)]*)\)",cmake,re.DOTALL)
        self.assertTrue(calls)
        keyword_calls=[call for call in calls if re.match(r"\s*(PRIVATE|PUBLIC|INTERFACE)\b",call)]
        self.assertEqual(keyword_calls,[],"main must consistently use upstream's plain target_link_libraries signature")
    def test_independent_modes(self):
        brush=source("src/DrawingProgram/Tools/BrushTool.cpp")
        self.assertIn("toolConfig.brush.samplePath(drawP.world.main.conf.tabletOptions.penFilter.enabled)",brush)
        self.assertIn("toolConfig.brush.pressureResponse == BrushPressure::Response::Original);",brush)
        self.assertIn("return correction || pressureResponse != Response::Original;",source("src/BrushPressureConfig.hpp"))
        main=source("src/MainProgram.cpp")
        self.assertLess(main.index('j.at("toolConfig").get_to'),main.index("toolConfig.brush.migrateCorrection"))
        core=source("src/CanvasComponents/BrushComponentCode.cpp")
        self.assertIn("const size_t changed = widthsChanged ? 0 : genData.stabilizer.changedBegin();",core)
        self.assertIn("genData.sampleWidths.output(samples[i].width,i)",core)
if __name__=="__main__":
    unittest.main()

