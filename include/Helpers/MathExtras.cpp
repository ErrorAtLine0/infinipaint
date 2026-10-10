/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "MathExtras.hpp"

double clamp_angle_to_two_pi(double a) {
    float rotation = a;
    // using circular fmod caused inaccuracies, so these loops might be a better solution
    while(rotation >= 2.0 * std::numbers::pi)
        rotation -= 2.0 * std::numbers::pi;
    while (rotation < 0.0)
        rotation += 2.0 * std::numbers::pi;

    return rotation;
}

double snap_angle_loop(double rotationAngle, double distanceToSnap, double distanceBetweenSnapAngles) {
    rotationAngle = clamp_angle_to_two_pi(rotationAngle);
    for(double snapPos = 0.0; snapPos < std::numbers::pi * 2.0 + 0.001; snapPos += distanceBetweenSnapAngles) {
        if(std::fabs(rotationAngle - snapPos) < distanceToSnap) {
            rotationAngle = snapPos;
            if(rotationAngle >= std::numbers::pi * 2.0 - 0.001) // Set 2pi back to 0
                rotationAngle = 0;
            return rotationAngle;
        }
    }
    return rotationAngle;
}
