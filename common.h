#pragma once

#include <algorithm>
#include <cmath>
#include <compare>
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
    double x = 0.;
    double y = 0.;
};

inline bool operator==(const Point& lhs, const Point& rhs) {
    double rel_epsilon = 1e-9;
    auto max_val = std::max({ 1.0, std::fabs(lhs.x), std::fabs(rhs.x),
                             std::fabs(lhs.y), std::fabs(rhs.y) }) * rel_epsilon;
    return std::abs(lhs.x - rhs.x) < max_val
           && std::abs(lhs.y - rhs.y) < max_val;
}

inline bool operator!=(const Point& lhs, const Point& rhs) {
    return !(lhs == rhs);
}

using Polyline = std::vector<Point>;

// Направление укладки сегмента
enum class Orientation {
    NoOrientation,
    Zero,       // 0
    Perpendicular,       // 90
    Other       // +-45 и другие
};

struct RawPolyline {
    Polyline polyline;
    domain::Orientation orientation = domain::Orientation::Zero;

    // Метры доступа
    [[nodiscard]] size_t pointsCount() const noexcept { return polyline.size(); }
    [[nodiscard]] const Point& pointAt(size_t index) const { return polyline.at(index); }
    [[nodiscard]] bool isEmpty() const noexcept { return polyline.empty(); }

    // Модификаторы
    void append(const Point& point) { polyline.push_back(point); }
    void reserve(size_t capacity) { polyline.reserve(capacity); }

    // Операторы сравнения
    bool operator==(const RawPolyline& other) const {
        return orientation == other.orientation && polyline == other.polyline;
    }
    bool operator!=(const RawPolyline& other) const { return !(*this == other); }
};

using RawData = std::list<RawPolyline>;

struct Polygon {
    Polygon() = default;
    Polygon(std::initializer_list<Point> points)
        :points_(points)
    {
    }
    void addPoint(Point p) {
        points_.push_back(p);
    }
    void addPolyline(const Polyline& polyline) {
        points_.insert(points_.end(), polyline.begin(), polyline.end());
    }
    void addPolyline(Polyline&& polyline) {
        points_.insert(points_.end(),
                       std::make_move_iterator(polyline.begin()),
                       std::make_move_iterator(polyline.end()));
    }
    [[nodiscard]] const std::vector<Point>& points() const noexcept { return points_; }
    [[nodiscard]] size_t pointsCount() const noexcept { return points_.size(); }
    [[nodiscard]] bool isEmpty() const noexcept { return points_.empty(); }

private:
    std::vector<Point> points_;
};

// Универсальная проверка приблизительного равенства с разделением абсолютной и относительной погрешности
inline bool ApproximatelyEqual(double lhs, double rhs,
                               double abs_epsilon = 1e-12,
                               double rel_epsilon = 1e-9) {
    // Сначала проверяем абсолютную разницу (для чисел около нуля)
    if (std::fabs(lhs - rhs) <= abs_epsilon) {
        return true;
    }
    // Затем проверяем относительную разницу
    double max_val = std::max(std::fabs(lhs), std::fabs(rhs));
    return std::fabs(lhs - rhs) <= rel_epsilon * max_val;
}

// Проверка приблизительного равенства двух точек
inline bool ApproximatelyEqual(const Point& lhs, const Point& rhs,
                               double abs_epsilon = 1e-12,
                               double rel_epsilon = 1e-7) {
    return ApproximatelyEqual(lhs.x, rhs.x, abs_epsilon, rel_epsilon)
    && ApproximatelyEqual(lhs.y, rhs.y, abs_epsilon, rel_epsilon);
}

// Проверка приблизительного равенства двух ломаных
inline bool ApproximatelyEqual(const Polyline& lhs, const Polyline& rhs,
                               double abs_epsilon = 1e-12,
                               double rel_epsilon = 1e-7) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (!ApproximatelyEqual(lhs[i], rhs[i], abs_epsilon, rel_epsilon)) {
            return false;
        }
    }
    return true;
}

// Безопасная проверка на ноль с комбинированной погрешностью
inline bool IsZero(double value,
                   double abs_epsilon = 1e-12,
                   double rel_epsilon = 1e-9) {
    // Для нуля используем комбинированный подход
    return std::fabs(value) <= std::max(abs_epsilon, rel_epsilon * std::fabs(value));
}

// Безопасное сравнение с учетом погрешности
inline bool IsLessOrEqual(double lhs, double rhs,
                          double abs_epsilon = 1e-12,
                          double rel_epsilon = 1e-9) {
    // Сначала точное сравнение
    if (lhs <= rhs) {
        return true;
    }
    // Затем проверка на приблизительное равенство
    return ApproximatelyEqual(lhs, rhs, abs_epsilon, rel_epsilon);
}

inline bool IsGreaterOrEqual(double lhs, double rhs,
                             double abs_epsilon = 1e-12,
                             double rel_epsilon = 1e-9) {
    if (lhs >= rhs) {
        return true;
    }
    return ApproximatelyEqual(lhs, rhs, abs_epsilon, rel_epsilon);
}

// Специализированные функции для строгих сравнений
inline bool IsStrictlyLess(double lhs, double rhs,
                           double abs_epsilon = 1e-12,
                           double rel_epsilon = 1e-9) {
    return (lhs < rhs) && !ApproximatelyEqual(lhs, rhs, abs_epsilon, rel_epsilon);
}

inline bool IsStrictlyGreater(double lhs, double rhs,
                              double abs_epsilon = 1e-12,
                              double rel_epsilon = 1e-8) {
    return (lhs > rhs) && !ApproximatelyEqual(lhs, rhs, abs_epsilon, rel_epsilon);
}

// Градусы в радианы
inline double GradToRad(double grad) { return std::numbers::pi * grad / 180.0; }

// Радианы в градусы
inline double RadToGrad(double rad) { return rad * 180.0 / std::numbers::pi; }

// Наклон отрезка
inline double LineSlope(const Point& p1, const Point& p2) {
    return std::atan2((p2.y - p1.y), (p2.x - p1.x));
}
// Наклон в виде компонент
inline auto SlopeComponents(const Point& p1, const Point& p2) noexcept {
    return std::make_pair(p2.y - p1.y, p2.x - p1.x); // (dy, dx)
}

// Проверка коллинеарности трех точек через векторное произведение
inline bool IsCollinear(const Point& a, const Point& b, const Point& c,
                        double abs_epsilon = 1e-12,
                        double rel_epsilon = 1e-8) {
    const double area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    return ApproximatelyEqual(area, 0.0, abs_epsilon, rel_epsilon);
}

// Проверка параллельности линий
inline bool IsParallelLines(const Point& p1, const Point& p2, const Point& p3, const Point& p4,
                            double abs_epsilon = 1e-12,
                            double rel_epsilon = 1e-8) {
    // Вычисляем знаменатель
    double denominator = (p4.x - p3.x) * (p2.y - p1.y) - (p2.x - p1.x) * (p4.y - p3.y);

    return IsZero(denominator, abs_epsilon, rel_epsilon);
}

// Находит точку пересечения двух отрезков
std::optional<Point> FindSegmentsIntersection(const Point& p1, const Point& p2,
                                              const Point& p3, const Point& p4, double abs_epsilon = 1e-12, double rel_epsilon = 1e-8);

// Находит пересечение двух бесконечных прямых, заданных двумя точками
std::optional<Point> FindLinesIntersection(const Point& p1, const Point& p2,
                                           const Point& q1, const Point& q2);

// Проверяет находится ли точка внутри многоугольника
bool IsPointInPolygon(const Point& test, const Polygon& polygon);

// Находит точку, которая находится на перпендикуляре к этому отрезку,
// отходящую от начальной точки start на расстояние offset влево от направления отрезка
Point GetPerpendicularPoint(const Point& start, const Point& end, double offset);

// Смещает ломаную линию на расстояние d (влево относительно направления обхода)
Polyline OffsetPolyline(const Polyline& polyline, double offset);

// Функция удаления всех самопересечений ломаной линии
Polyline RemoveSelfIntersections(const Polyline& input);

// Проверка пересечения ломаной линии отрезком с точками begin - end
bool IsLineIntersectsPolyline(const Point& begin, const Point& end, const Polyline& polyline);

// Проверка находится ли ломаная линия внутри многоугольника
bool IsPolylinePointInPolygon(const Polyline& polyline, const Polygon& poly);

double DistanceBetweenPoints(const Point& p1, const Point& p2);

RawPolyline RemoveExtraDots(const RawPolyline& polyline, double abs_epsilon = 1e-12,
                            double rel_epsilon = 1e-7);

RawData RemoveExtraDots(const RawData& data, double abs_epsilon = 1e-12,
                        double rel_epsilon = 1e-7);

Point СalculateBisector(const Point& a, const Point& b, const Point& c, double length);

Point ExtendLine(const Point& start, const Point& end, double distance);

Point GetPointOnRay(const Point& start, const Point& direction, double distance);

} // namespace domain {
