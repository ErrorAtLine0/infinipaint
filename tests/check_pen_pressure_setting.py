"""Source-level configuration/UI wiring checks; not a GUI or renderer test."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


def source(path):
    return (ROOT / path).read_text(encoding="utf-8")


class PressureSettingWiring(unittest.TestCase):
    def test_default_and_config_roundtrip_registration(self):
        config = source("src/DrawingProgram/ToolConfiguration.hpp")
        brush = config.split("struct BrushToolConfig {", 1)[1].split("} brush;", 1)[0]
        self.assertIn("bool preservePenPressure = false;", brush)
        self.assertRegex(brush, r"NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT\(BrushToolConfig,\s*hasRoundCaps,\s*relativeWidth,\s*preservePenPressure\)")

    def test_both_brush_panels_and_contact_down(self):
        brush = source("src/DrawingProgram/Tools/BrushTool.cpp")
        for panel in ("gui_toolbox", "gui_phone_toolbox"):
            body = brush.split(f"void BrushTool::{panel}(", 1)[1].split("\nvoid ", 1)[0]
            self.assertIn('"Preserve per-point pen pressure"', body)
            self.assertIn("&drawP.world.main.toolConfig.brush.preservePenPressure", body)
        self.assertRegex(brush, r"mouse_button\([^;]*toolConfig\.brush\.preservePenPressure\);")
        motion = brush.split("void BrushTool::input_mouse_motion_callback", 1)[1].split("\nvoid ", 1)[0]
        self.assertNotIn("preservePenPressure", motion)
        self.assertIn("motion.penContact && motion.penId == genData.penId", motion)
        self.assertIn("motion.deviceType != InputManager::MouseDeviceType::PEN", motion)
        self.assertIn("if (!genData.penPath) BrushComponentCode::fix_tip", brush)

    def test_eraser_and_shared_default_remain_original(self):
        header = source("src/CanvasComponents/BrushComponentCode.hpp")
        self.assertIn("bool useDirectPenPath = false", header)
        eraser = source("src/DrawingProgram/Tools/EraserTool.cpp")
        self.assertRegex(eraser, r"mouse_button\([^;]*, false\);")
        core = source("src/CanvasComponents/BrushComponentCode.cpp")
        self.assertIn("genData.penPath = useDirectPenPath && button.deviceType == InputManager::MouseDeviceType::PEN;", core)
        self.assertIn("genData.penWidth = minimum + genData.penWidth * (1.0f - minimum);", core)


if __name__ == "__main__":
    unittest.main()
