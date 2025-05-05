#include "common.h"

namespace domain {

bool IsPerpendicularLines(const Point& p1, const Point& p2, const Point& p3, const Point& p4) {
    // Вычисляем знаменатель
    double denominator = (p4.x - p3.x) * (p2.y - p1.y) - (p2.x - p1.x) * (p4.y - p3.y);

    return IsZero(denominator);
}

std::optional<Point> FindSegmentsIntersection(const Point& p1, const Point& p2, const Point& p3, const Point& p4) {
    // Вычисляем знаменатель
    double denominator = (p4.x - p3.x) * (p2.y - p1.y) - (p2.x - p1.x) * (p4.y - p3.y);

    // Если знаменатель равен нулю, линии параллельны или совпадают
    if (IsZero(denominator)) {
        return std::nullopt;
    }

    // Вычисляем параметры t и u
    double t = ((p4.x - p3.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p4.y - p3.y)) / denominator;
    double u = ((p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x)) / denominator;

    // Проверяем, находятся ли параметры в пределах отрезков
    if (IsGreaterOrEqual(t, 0.f) && IsLessOrEqual(t, 1.0f)
        && IsGreaterOrEqual(u, 0.f) && IsLessOrEqual(u, 1.0f))
    {
        return Point(p1.x + t * (p2.x - p1.x), p1.y + t * (p2.y - p1.y));
    }

    return std::nullopt;
}

std::optional<Point> FindLinesIntersection(const Point& p1, const Point& p2,
    const Point& q1, const Point& q2) {
    double dx1 = p2.x - p1.x;
    double dy1 = p2.y - p1.y;
    double dx2 = q2.x - q1.x;
    double dy2 = q2.y - q1.y;

    double det = dx1 * dy2 - dy1 * dx2;
    if (IsZero(det)) {
        return std::nullopt; // Прямые параллельны
    }

    double t = ((q1.x - p1.x) * dy2 - (q1.y - p1.y) * dx2) / det;

    return Point(p1.x + dx1 * t, p1.y + dy1 * t);
}

bool IsPointInPolygon(const Point& test, const Polygon& polygon) {

    bool is_inside = false;

    for (size_t i = 0, j = polygon.GetNumOfPoints() - 1; i < polygon.GetNumOfPoints(); j = i++) {
        const Point& p1 = polygon.GetPoints()[i];
        const Point& p2 = polygon.GetPoints()[j];

        // Проверка на совпадение с вершиной
        if (ApproximatelyEqual(test.x, p1.x) && ApproximatelyEqual(test.y, p1.y)) {
            return true;
        }

        // Вертикальный луч пересекает ребро только если:
        // 1. Тестовая X-координата между X-координатами ребра
        // 2. Y-координата пересечения выше тестовой Y

        // Игнорируем вертикальные ребра (X не меняется)
        if (IsZero(p1.x - p2.x)) {
            if (IsZero(test.x - p1.x)) {
                // Проверка Y-диапазона
                if (IsGreaterOrEqual(test.y, std::min(p1.y, p2.y)) &&
                    IsLessOrEqual(test.y, std::max(p1.y, p2.y)))
                {
                    return true; // На вертикальном ребре
                }
            }
            continue;
        }

        // Проверяем, лежит ли test.x между p1.x и p2.x
        if (std::islessequal(test.x, std::min(p1.x, p2.x)) ||
            (test.x > std::max(p1.x, p2.x)))
        {
            continue;
        }

        // Вычисляем Y-координату пересечения вертикального луча с ребром
        double t = (test.x - p1.x) / (p2.x - p1.x);
        double y_intersect = p1.y + t * (p2.y - p1.y);

        // Точка лежит на ребре
        if (IsZero(y_intersect - test.y)) {
            return true;
        }

        // Если пересечение выше тестовой точки то ребро пересекает луч
        if (y_intersect > test.y) {
            is_inside = !is_inside;
        }
    }

    return is_inside;
}

Point GetPerpendicularPoint(const Point& start, const Point& end, double offset) {
    double dx = end.x - start.x;
    double dy = end.y - start.y;
    double len = std::hypot(dx, dy);
    if (IsZero(len)) {
        return start;
    }
    double perp_x = -dy / len * offset;  // Перпендикуляр направлен
    double perp_y = dx / len * offset;   // влево от линии

    return { start.x + perp_x, start.y + perp_y };
}


Polyline OffsetPolyline(const Polyline& polyline, double offset) {
    Polyline result;
    if (polyline.size() < 2) {
        return {};
    }

    for (size_t i = 0; i < polyline.size(); ++i) {
        if (i == 0 || i == polyline.size() - 1) {

            // Обработка первой и последней точки

            const Point& current = polyline[i];
            const Point& neighbor = (i == 0) ? polyline[i + 1] : polyline[i - 1];

            if (current == neighbor) {
                return {};
            }

            result.emplace_back(GetPerpendicularPoint(current, neighbor, (i == 0) ? offset : -offset));
        }
        else {

            // Обработка внутренних точек

            const Point& prev = polyline[i - 1];
            const Point& curr = polyline[i];
            const Point& next = polyline[i + 1];

            if (curr == next) {
                return {};
            }

            // Точки смещения предыдущего сегмента (prev-curr)
            Point p_prev1 = result.back();
            Point p_prev2 = GetPerpendicularPoint(curr, prev, -offset);

            // Точки смещения следующего сегмента (curr-next)
            Point p_next1 = GetPerpendicularPoint(curr, next, offset);
            Point p_next2 = GetPerpendicularPoint(next, curr, -offset);

            // Находим пересечение смещенных прямых
            const auto intersect = FindLinesIntersection(p_prev1, p_prev2, p_next1, p_next2);
            if (intersect.has_value()) {
                result.push_back(intersect.value());
            }
            else {
                // Прямые параллельны - добавляем среднюю точку
                Point avg = {
                    (p_prev1.x + p_next1.x) / 2,
                    (p_prev1.y + p_next1.y) / 2
                };
                result.push_back(avg);
            }
        }
    }
    return result;
}

Polyline RemoveSelfIntersections(const Polyline& input) {
    Polyline current = input;
    bool intersection_found = false;

    // Лямбда для удаления одного пересечения
    auto RemoveOneIntersection = [&intersection_found](const Polyline& input) -> Polyline {
        const size_t n = input.size();
        intersection_found = false;

        if (n < 4) {
            return input;
        }

        for (size_t i = 0; i < n - 1; ++i) {
            for (size_t j = i + 2; j < n - 1; ++j) {
                const auto intersection = FindSegmentsIntersection(input[i], input[i + 1],
                    input[j], input[j + 1]);
                if (intersection.has_value()) {
                    std::vector<Point> result;
                    result.reserve(i + 2 + (n - j - 1));

                    // Первая часть
                    for (size_t k = 0; k <= i; ++k) {
                        result.push_back(input[k]);
                    }

                    // Точка пересечения
                    result.push_back(intersection.value());

                    // Вторая часть
                    for (size_t k = j + 1; k < n; ++k) {
                        result.push_back(input[k]);
                    }

                    intersection_found = true;
                    return result;
                }
            }
        }
        return input;
        };

    do {
        current = RemoveOneIntersection(current);
    } while (intersection_found && current.size() > 3);

    return current;
}

bool IsLineIntersectsPolyline(const Point& begin, const Point& end, const Polyline& polyline) {

    for (size_t i = 1; i < polyline.size(); ++i) {
        if (FindSegmentsIntersection(begin, end, polyline[i - 1], polyline[i]).has_value()) {
            return true;
        }
    }
    return false;
}

bool IsPolylinePointInPolygon(const Polyline& polyline, const Polygon& poly) {
    for (const auto& point : polyline) {
        if (IsPointInPolygon(point, poly)) {
            return true;
        }
    }
    return false;
}

double DistanceBetweenPoints(const Point& p1, const Point& p2) {
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    return std::sqrt(dx * dx + dy * dy);
}

RawPolyline RemoveExtraDots(const RawPolyline& input, double abs_epsilon, double rel_epsilon) {
    RawPolyline result;

    if (input.polyline.size() < 3) {
        return {};
    }
    result.polyline.reserve(input.polyline.size());
    result.ori = input.ori;

    result.polyline.emplace_back(input.polyline[0]);

    for (size_t prev = 0, curr = 1, next = 2; next < input.polyline.size(); ++next, ++curr) {
        auto first_line_slope = SlopeComponents(input.polyline[prev], input.polyline[curr]);
        auto second_line_slope = SlopeComponents(input.polyline[curr], input.polyline[next]);

        if (ApproximatelyEqual(first_line_slope.first, second_line_slope.first, abs_epsilon, rel_epsilon)
            && ApproximatelyEqual(first_line_slope.second, second_line_slope.second, abs_epsilon, rel_epsilon)) {
            continue;
        }
        else {
            result.polyline.emplace_back(input.polyline[curr]);
            prev = curr;
        }
    }
        
    result.polyline.emplace_back(input.polyline.back());
    return result;
}

RawData RemoveExtraDots(const RawData& data, double abs_epsilon, double rel_epsilon) {
    RawData result;

    for (const auto& polyline : data) {
        result.emplace_back(RemoveExtraDots(polyline, abs_epsilon, rel_epsilon));
    }
    return result;
}

} // namespace domain
