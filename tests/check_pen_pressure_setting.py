"""Source wiring checks, not full renderer/device validation."""
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
def source(p):
    return (ROOT / p).read_text(encoding="utf-8")
class PressureWiring(unittest.TestCase):
    def test_config(self):
        config = source("src/BrushPressureConfig.hpp")
        self.assertIn("pressureResponse = Response::Original", config)
        self.assertIn('j.contains("preservePenPressure")', config)
        self.assertIn("using BrushToolConfig = BrushPressure::Config", source("src/DrawingProgram/ToolConfiguration.hpp"))
    def test_panels_and_capture(self):
        brush = source("src/DrawingProgram/Tools/BrushTool.cpp")
        for panel in ("gui_toolbox", "gui_phone_toolbox"):
            self.assertIn("gui_pressure_options();", brush.split("void BrushTool::"+panel,1)[1].split("\nvoid ",1)[0])
        for label in ("Smoothed pressure (default)", "Preserve samples", "Uniform peak width", "Width propagation"):
            self.assertIn('"'+label+'"', brush)
        self.assertNotIn('"Preserve per-point pen pressure"', brush)
        self.assertIn("toolConfig.brush.samplePath(", brush)
        self.assertIn("toolConfig.brush.pressureResponse == BrushPressure::Response::Peak", brush)
        motion=brush.split("void BrushTool::input_mouse_motion_callback",1)[1].split("\nvoid ",1)[0]
        self.assertNotIn("pressureResponse", motion)
        self.assertIn("motion.penContact && motion.penId == genData.penId", motion)
        self.assertIn("if (!genData.penPath) BrushComponentCode::fix_tip", brush)
    def test_eraser_and_original(self):
        self.assertIn("bool useDirectPenPath = false", source("src/CanvasComponents/BrushComponentCode.hpp"))
        self.assertRegex(source("src/DrawingProgram/Tools/EraserTool.cpp"), r"mouse_button\([^;]*, false\);")
        core=source("src/CanvasComponents/BrushComponentCode.cpp")
        self.assertIn("genData.penPath = useDirectPenPath && button.deviceType == InputManager::MouseDeviceType::PEN;", core)
        self.assertIn("smooth_out_points(genData.brushPoints, drawP.world.main.conf.tabletOptions.brushPressureSmoothingFactor);", core)
if __name__ == "__main__":
    unittest.main()
