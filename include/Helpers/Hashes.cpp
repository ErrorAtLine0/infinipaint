#include "Hashes.hpp"

bool operator <(const Vector2d& a, const Vector2d& b) {
    return ((a.x() < b.x()) || ((a.x() == b.x()) && (a.y() < b.y())));
}
