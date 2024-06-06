#pragma once
#include <cmath>

// Second-order IIR filter (biquad), (coefficients use Audio EQ Cookbook convention)
// State kept as double to prevent precision loss in feedback paths.
struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0;
    double a1 = 0, a2 = 0;
    double x1 = 0, x2 = 0;
    double y1 = 0, y2 = 0;

    inline float process(float in) noexcept {
        double y = b0*in + b1*x1 + b2*x2 - a1*y1 - a2*y2;
        x2 = x1; x1 = in;
        y2 = y1; y1 = y;
        return static_cast<float>(y);
    }

    void preserveStateFrom(const Biquad& src) {
        x1 = src.x1; x2 = src.x2;
        y1 = src.y1; y2 = src.y2;
    }

    static Biquad highPass(double fc, double Q, double sr) {
        double w0 = 2*M_PI*fc/sr, alpha = std::sin(w0)/(2*Q), cw = std::cos(w0);
        double a0 = 1 + alpha;
        Biquad f;
        f.b0 =  (1 + cw) / (2*a0);
        f.b1 = -(1 + cw) / a0;
        f.b2 =  (1 + cw) / (2*a0);
        f.a1 = -2*cw / a0;
        f.a2 = (1 - alpha) / a0;
        return f;
    }

    static Biquad lowPass(double fc, double Q, double sr) {
        double w0 = 2*M_PI*fc/sr, alpha = std::sin(w0)/(2*Q), cw = std::cos(w0);
        double a0 = 1 + alpha;
        Biquad f;
        f.b0 = (1 - cw) / (2*a0);
        f.b1 = (1 - cw) / a0;
        f.b2 = (1 - cw) / (2*a0);
        f.a1 = -2*cw / a0;
        f.a2 = (1 - alpha) / a0;
        return f;
    }

    static Biquad peaking(double fc, double dBgain, double Q, double sr) {
        double A = std::pow(10.0, dBgain/40.0);
        double w0 = 2*M_PI*fc/sr, alpha = std::sin(w0)/(2*Q), cw = std::cos(w0);
        double a0 = 1 + alpha/A;
        Biquad f;
        f.b0 = (1 + alpha*A) / a0;
        f.b1 = -2*cw / a0;
        f.b2 = (1 - alpha*A) / a0;
        f.a1 = -2*cw / a0;
        f.a2 = (1 - alpha/A) / a0;
        return f;
    }

    static Biquad lowShelf(double fc, double dBgain, double sr) {
        double A  = std::pow(10.0, dBgain/40.0), sqA = std::sqrt(A);
        double w0 = 2*M_PI*fc/sr, cw = std::cos(w0);
        double alpha = std::sin(w0)/2 * std::sqrt((A + 1/A)*(1/0.9 - 1) + 2);
        double a0 = (A+1) + (A-1)*cw + 2*sqA*alpha;
        Biquad f;
        f.b0 =  A*((A+1) - (A-1)*cw + 2*sqA*alpha) / a0;
        f.b1 = 2*A*((A-1) - (A+1)*cw) / a0;
        f.b2 =  A*((A+1) - (A-1)*cw - 2*sqA*alpha) / a0;
        f.a1 = -2*((A-1) + (A+1)*cw) / a0;
        f.a2 = ((A+1) + (A-1)*cw - 2*sqA*alpha) / a0;
        return f;
    }

    static Biquad highShelf(double fc, double dBgain, double sr) {
        double A  = std::pow(10.0, dBgain/40.0), sqA = std::sqrt(A);
        double w0 = 2*M_PI*fc/sr, cw = std::cos(w0);
        double alpha = std::sin(w0)/2 * std::sqrt((A + 1/A)*(1/0.9 - 1) + 2);
        double a0 = (A+1) - (A-1)*cw + 2*sqA*alpha;
        Biquad f;
        f.b0 =  A*((A+1) + (A-1)*cw + 2*sqA*alpha) / a0;
        f.b1 = -2*A*((A-1) + (A+1)*cw) / a0;
        f.b2 =  A*((A+1) + (A-1)*cw - 2*sqA*alpha) / a0;
        f.a1 =  2*((A-1) - (A+1)*cw) / a0;
        f.a2 = ((A+1) - (A-1)*cw - 2*sqA*alpha) / a0;
        return f;
    }
};
