#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <list>
#include <numbers>
#include <optional>
#include <vector>
#include <utility>
#include <iomanip>
#include <iostream>

namespace domain {

    struct Point {
        Point() = default;
        Point(float x, float y)
            : x(x)
            , y(y)
        {
        }
        float x = 0.0;
        float y = 0.0;
    };

    inline bool operator==(const Point lhs, const Point rhs) {
        return std::fabs(lhs.x - rhs.x) < 1e-4f    // Сравнение точек производим с точностью
            && std::fabs(lhs.y - rhs.y) < 1e-4f;   // до четвертого знака после запятой 
    }

    inline bool operator!=(const Point lhs, const Point rhs) {
        return !(lhs == rhs);
    }

    using Polyline = std::vector<Point>;

    enum class ORIENTATION{
        ZERO,                // 0
        PERPENDICULAR,       // 90
        OTHER                // +-45 и другие
    };

    struct Raw_layer{
        Polyline polyline_;
        ORIENTATION orientation_ = ORIENTATION::ZERO;
    };

    using Raw_sketch = std::list<Raw_layer>;

    struct Polygon {
        Polygon() = default;
        Polygon(std::initializer_list<Point> points)
            :points_(points)
        {
        }
        void AddPoint(Point p) {
            points_.push_back(p);
        }
        void AddPolyline(const Polyline& polyline) {
            points_.insert(points_.end(), polyline.begin(), polyline.end());
        }
        void AddPolyline(Polyline&& polyline) {
            points_.insert(points_.end(),
                std::make_move_iterator(polyline.begin()),
                std::make_move_iterator(polyline.end()));
        }
        const std::vector<Point>& GetPoints() const { return points_; }
        size_t GetNumOfPoints() const { return points_.size(); }
    private:
        std::vector<Point> points_;
    };

    inline bool IsZero(double x) {
        return std::fabs(x) < std::numeric_limits<float>::epsilon();
    }

    inline bool IsEqual(double x, double y) {
        return std::fabs(x - y) < std::numeric_limits<float>::epsilon();
    }

    inline bool IsLessOrEqual(double x, double y) {
        return (x < y) || IsEqual(x, y);
    }

    inline bool IsMoreOrEqual(double x, double y) {
        return (x > y) || IsEqual(x, y);
    }

    inline double GradToRad(double grad) {
        return std::numbers::pi * grad / 180.0;
    }

    inline double RadToGrad(double rad) {
        return rad * 180.0 / std::numbers::pi;
    }

    inline double LineSlope(Point p1, Point p2) {
        return atan2((p2.y - p1.y), (p2.x - p1.x));
    }

    /*
    inline double AngleBetweenSegments() {
        // Угол между сегментами через скалярное произведение
        double cos_angle = (dx_prev * dx_next + dy_prev * dy_next) / (len_prev * len_next);
        cos_angle = std::clamp(cos_angle, -1.0, 1.0);
        double angle = std::acos(cos_angle);
    }
    */

    // Находит точку пересечения двух отрезков
    std::optional<Point> FindSegmentsIntersection(Point p1, Point p2, Point p3, Point p4);

    // Находит пересечение двух бесконечных прямых, заданных двумя точками
    std::optional<Point> FindLinesIntersection(const Point& p1, const Point& p2,
        const Point& q1, const Point& q2);

    // Находит точку пересечения луча, направленного из точки 'start' под углом 'angle' (в радианах),
    // с границей прямоугольника с начальной точкой 'х = 0, y = 0'  и габаритами 'width x height'
    // внутри которого располагается точка 'start' 
    Point FindBoundaryIntersection(Point start, double angle, float width, float height);

    // Проверяет находится ли точка внутри многоугольника
    bool IsPointInPolygon(const Point test, const Polygon& polygon);

    Point GetPerpendicularPoint(const Point start, const Point end, double offset);

    // Смещает ломаную линию на расстояние d (влево относительно направления обхода)
    Polyline OffsetPolyline(const Polyline& polyline, double offset);

    // Функция для поиска и удаления одного пересечения
    Polyline RemoveOneIntersection(const Polyline& input, bool& found);

    // Основная функция для удаления всех пересечений
    Polyline RemoveSelfIntersections(const Polyline& input);

    bool IsLineIntersectsPolyline(const Point begin, const Point end, const Polyline& polyline);

    bool IsPolylinePointInPolygon(const Polyline& polyline, const Polygon& poly);

} // namespace domain {
