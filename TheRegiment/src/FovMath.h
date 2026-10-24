#pragma once
#include <algorithm>
#include <cmath>
namespace regiment {
constexpr double Pi = 3.14159265358979323846;
constexpr double ReferenceAspect = 4.0 / 3.0;
inline double ClampBase(double fov) { return std::clamp(fov, 30.0, 110.0); }
// Preserve the original zoom multiplier in tangent space. Base is the user's
// horizontal FOV at 4:3; the incoming camera can still zoom or animate its FOV.
inline float ProjectFov(float incoming, double aspect, double base, bool horPlus) {
    if (!std::isfinite(incoming) || incoming <= 1.0f || incoming >= 179.0f ||
        !std::isfinite(aspect) || aspect <= 0.0) return incoming;
    double scale = std::tan(base * Pi / 360.0) / std::tan(70.0 * Pi / 360.0);
    if (horPlus) scale *= aspect / ReferenceAspect;
    return static_cast<float>(std::clamp(360.0 / Pi * std::atan(std::tan(incoming * Pi / 360.0) * scale), 1.0, 170.0));
}
inline float CorrectFov(float incoming,double aspect,double base,bool horPlus){return ProjectFov(incoming,aspect,ClampBase(base),horPlus);}
inline double ClampViewmodel(double fov){return std::clamp(fov,20.0,140.0);}
inline float CorrectViewmodelFov(float incoming,double aspect,double base,bool horPlus){return ProjectFov(incoming,aspect,ClampViewmodel(base),horPlus);}
}
