/* Conquer Space
* Copyright (C) 2021 Conquer Space
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
#pragma once

#include <entt/entt.hpp>
#include <math.h>
#include "common/components/units.h"
#include "spdlog/spdlog.h"
#include "glm/glm.hpp"

namespace conquerspace {
namespace common {
namespace components {
namespace types {
//struct Kinematics {
//    glm::vec2 velocity;
//    float topspeed = 10;
//};

struct MoveTarget {
    entt::entity target;
    MoveTarget(entt::entity _targetent) : target(_targetent) {}
};


/**
 * Orbit of a body
 */
struct Orbit {
    double gravitationalparameter;
    degree theta;
    astronomical_unit semiMajorAxis;
    double eccentricity;
    double angularvelocity;
    degree argument;
    years period;


    // So we can prepare for moons and stuff
    entt::entity referenceBody = entt::null;

    Orbit() = default;
    Orbit(types::degree _trueAnomaly, types::astronomical_unit _semiMajorAxis,
          double _eccentricity, types::degree _argument, double _gravparam): theta(_trueAnomaly),
          semiMajorAxis(_semiMajorAxis),
          eccentricity(_eccentricity),
          argument(_argument),
          gravitationalparameter(_gravparam){}//should be 40
};

struct Vec2 {
    astronomical_unit x;
    astronomical_unit y;

    Vec2() = default;
    constexpr Vec2(types::astronomical_unit _x, types::astronomical_unit _y)
        : x(_x), y(_y) {}

    constexpr Vec2(const Vec2& v) : x(v.x), y(v.y) {}

    template<class T>
    constexpr Vec2(const T& v) : x(v.x), y(v.y) {}

    constexpr Vec2& operator+=(const Vec2& v) {
        this->x = v.x;
        this->y = v.y;
        return *this;
    }

    constexpr Vec2& operator-=(const Vec2& v) {
        this->x -= v.x;
        this->y -= v.y;
        return *this;
    }

    template<typename T>
    constexpr Vec2& operator*=(T scalar) {
        this->x *= scalar;
        this->y *= scalar;
        return *this;
    }

    template<typename T>
    constexpr Vec2& operator/=(T scalar) {
        this->x /= scalar;
        this->y /= scalar;
        return *this;
    }

    constexpr Vec2 operator+(const Vec2& c) const {
        return Vec2(this->x + c.x, this->y + c.y);
    }

    constexpr Vec2 operator-(const Vec2& c) const {
        return Vec2(this->x - c.x, this->y - c.y);
    }

    template<typename T>
    constexpr Vec2 operator*(T scalar) const {
        return Vec2(this->x * scalar, this->y * scalar);
    }

    bool operator==(const Vec2& v) {
        return this->x == v.x && this->y == v.y;
    }

    template<class T>
    operator Vec2() {
        return T(this->x, this.y);
    }

    template<typename T>
    constexpr Vec2 operator/(T scalar) const {
        return Vec2(this->x / scalar, this->y / scalar);
    }

    double distance(const Vec2& v) { return distance(*this, v); }

    Vec2 normalize() {
        return normalize(*this);
    }

    static double distance(const Vec2& a, const Vec2& b) {
        return length((a - b));
    }

    static double length(const Vec2& v) {
        return sqrt((v.x * v.x) + (v.y * v.y));
    }

    static Vec2 normalize(const Vec2& v) {
        return v/length(v);
    }
};

struct Position : public Vec2 {
    using Vec2::Vec2;
};

struct PolarCoordinate {
    astronomical_unit r;
    degree theta;

    PolarCoordinate() = default;
    PolarCoordinate(types::astronomical_unit _r, types::degree _theta) : r(_r), theta(_theta) {}
};

inline Orbit& updateOrbit(Orbit& orb) {
    orb.theta += orb.angularvelocity;
    return orb;
}

inline void findPeriod(Orbit& orb) { 
    orb.period = TWOPI * std::sqrt(std::pow(orb.semiMajorAxis, 3) /
                                    orb.gravitationalparameter);
    orb.angularvelocity = 360.0 * (1.0 / (orb.period * 365));
}

inline types::radian toRadian(types::degree theta) {
    return theta * (conquerspace::common::components::types::PI / 180);
}

inline types::degree toDegree(types::radian theta) {
    return theta * (180 / conquerspace::common::components::types::PI);
}



inline Vec2 toVec2(const PolarCoordinate& coordinate) {
    return Vec2{static_cast<types::astronomical_unit>(
                    static_cast<double>(coordinate.r) *
                    cos(toRadian(coordinate.theta))),
                static_cast<types::astronomical_unit>(
                    static_cast<double>(coordinate.r) *
                    sin(toRadian(coordinate.theta)))};
}

inline PolarCoordinate toPolarCoordinate(const Orbit& orb) {
    double r = orb.semiMajorAxis * (1 - orb.eccentricity * orb.eccentricity) /
               (1 - orb.eccentricity *
                        cos(toRadian(fmod(orb.theta, 360) + orb.argument)));
    return PolarCoordinate{(types::astronomical_unit)r, fmod(orb.theta, 360)};
}

inline Vec2 toVec2(const Orbit& orb) { return toVec2(toPolarCoordinate(orb)); }
}  // namespace bodies
}  // namespace components
}  // namespace common
}  // namespace conquerspace
