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
