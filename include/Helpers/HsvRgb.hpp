#pragma once
#include <cmath>

// https://stackoverflow.com/a/6930407

template <typename S, typename T> S rgb_to_hsv(const T& in)
{
    S out;
    float min, max, delta;

    min = in[0] < in[1] ? in[0] : in[1];
    min = min < in[2] ? min : in[2];

    max = in[0] > in[1] ? in[0] : in[1];
    max = max > in[2] ? max : in[2];

    out[2] = max; // v
    delta = max - min;
    if(delta < 0.000001f)
    {
        out[1] = 0;
        out[0] = 0; // undefined, maybe nan?
        return out;
    }
    if(max > 0.0f) { // NOTE: if Max is == 0, this divide would cause a crash
        out[1] = (delta / max); // s
    } else {
        // if max is 0, then r = g = b = 0
        // s = 0, h is undefined
        out[1] = 0.0f;
        out[0] = NAN; // its now undefined
        return out;
    }
    if(in[0] >= max) // > is bogus, just keeps compiler happy
        out[0] = (in[1] - in[2]) / delta; // between yellow & magenta
    else {
        if(in[1] >= max)
            out[0] = 2.0 + ( in[2] - in[0] ) / delta; // between cyan & yellow
        else
            out[0] = 4.0 + ( in[0] - in[1] ) / delta; // between magenta & cyan
    }

    out[0] *= 60.0f; // degrees

    if(out[0] < 0.0f)
        out[0] += 360.0f;

    return out;
}


template <typename S, typename T> S hsv_to_rgb(const T& in)
{
    float hh, p, q, t, ff;
    long i;
    S out;

    if(in[1] <= 0.0f) { // < is bogus, just shuts up warning
        out[0] = in[2];
        out[1] = in[2];
        out[2] = in[2];
        return out;
    }
    hh = in[0];
    if(hh >= 360.0f)
        hh = 0.0f;

    hh /= 60.0f;
    i = (long)hh;
    ff = hh - i;
    p = in[2] * (1.0f - in[1]);
    q = in[2] * (1.0f - (in[1] * ff));
    t = in[2] * (1.0f - (in[1] * (1.0f - ff)));

    switch(i) {
        case 0:
            out[0] = in[2];
            out[1] = t;
            out[2] = p;
            break;
        case 1:
            out[0] = q;
            out[1] = in[2];
            out[2] = p;
            break;
        case 2:
            out[0] = p;
            out[1] = in[2];
            out[2] = t;
            break;
        case 3:
            out[0] = p;
            out[1] = q;
            out[2] = in[2];
            break;
        case 4:
            out[0] = t;
            out[1] = p;
            out[2] = in[2];
            break;
        case 5:
        default:
            out[0] = in[2];
            out[1] = p;
            out[2] = q;
            break;
    }
    return out;     
}
