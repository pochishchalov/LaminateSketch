#include "common.h"

namespace domain {

std::optional<Point> FindSegmentsIntersection(const Point& p1, const Point& p2, const Point& p3, const Point& p4,
    double abs_epsilon, double rel_epsilon)
{
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
    if (IsGreaterOrEqual(t, 0., abs_epsilon, rel_epsilon) && IsLessOrEqual(t, 1.0, abs_epsilon, rel_epsilon)
        && IsGreaterOrEqual(u, 0., abs_epsilon, rel_epsilon) && IsLessOrEqual(u, 1.0, abs_epsilon, rel_epsilon))
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

    for (size_t i = 0, j = polygon.pointsCount() - 1; i < polygon.pointsCount(); j = i++) {
        const Point& p1 = polygon.points()[i];
        const Point& p2 = polygon.points()[j];

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
        if (IsLessOrEqual(test.x, std::min(p1.x, p2.x)) ||
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
    if (input.polyline.empty()) return result;

    result.orientation = input.orientation;
    result.polyline.reserve(input.polyline.size());

    // Всегда добавляем первую точку
    result.polyline.push_back(input.polyline[0]);
    size_t prev = 0;

    for (size_t next = 1; next < input.polyline.size(); ++next) {
        const auto& p_prev = result.polyline.back();
        const auto& p_curr = input.polyline[next];

        // Проверяем коллинеарность текущего отрезка с предыдущим
        if (result.polyline.size() >= 2) {
            const auto& p_prev_prev = result.polyline[result.polyline.size() - 2];
            if (IsCollinear(p_prev_prev, p_prev, p_curr, abs_epsilon, rel_epsilon)) {
                // Удаляем предыдущую точку, так как она коллинеарна
                result.polyline.pop_back();
            }
        }
        result.polyline.push_back(p_curr);
    }

    return result;
}

RawData RemoveExtraDots(const RawData& data, double abs_epsilon, double rel_epsilon) {
    RawData result;

    for (const auto& polyline : data) {
        result.emplace_back(RemoveExtraDots(polyline, abs_epsilon, rel_epsilon));
    }
    return result;
}

Point СalculateBisector(const Point& a, const Point& b, const Point& c, double length) {
    Point bisector_end = b; // По умолчанию возвращаем саму точку b

    // Векторы BA и BC
    double ba_x = a.x - b.x;
    double ba_y = a.y - b.y;
    double bc_x = c.x - b.x;
    double bc_y = c.y - b.y;

    // Длины векторов
    double len_ba = sqrt(ba_x * ba_x + ba_y * ba_y);
    double len_bc = sqrt(bc_x * bc_x + bc_y * bc_y);

    // Проверка на нулевую длину векторов
    if (IsZero(len_ba) || IsZero(len_bc)) {
        return bisector_end;
    }

    // Нормализация векторов
    double norm_ba_x = ba_x / len_ba;
    double norm_ba_y = ba_y / len_ba;
    double norm_bc_x = bc_x / len_bc;
    double norm_bc_y = bc_y / len_bc;

    // Направление биссектрисы
    double dir_x = norm_ba_x + norm_bc_x;
    double dir_y = norm_ba_y + norm_bc_y;

    // Нормализация направления
    double dir_length = sqrt(dir_x * dir_x + dir_y * dir_y);
    if (dir_length == 0) {
        return bisector_end;
    }

    // Вычисление конечной точки
    bisector_end.x = b.x + (dir_x / dir_length) * length;
    bisector_end.y = b.y + (dir_y / dir_length) * length;

    return bisector_end;
}

Point ExtendLine(const Point& start, const Point& end, double distance) {
    // Вычисляем вектор направления
    double dx = end.x - start.x;
    double dy = end.y - start.y;

    // Вычисляем длину отрезка
    double length = std::sqrt(dx * dx + dy * dy);

    // Если точки совпадают, возвращаем конечную точку
    if (length == 0.0) {
        return end;
    } 

    // Нормализуем вектор и умножаем на расстояние
    double extend_x = (dx / length) * distance;
    double extend_y = (dy / length) * distance;

    // Создаем новую точку
    return {
        end.x + extend_x,
        end.y + extend_y
    };
}

Point GetPointOnRay(const Point& start, const Point& direction, double distance) {
    // Вычисляем вектор направления луча
    double dx = direction.x - start.x;
    double dy = direction.y - start.y;

    // Вычисляем длину вектора направления
    double length = std::sqrt(dx * dx + dy * dy);

    // Если точки совпадают, возвращаем стартовую точку
    if (length == 0.0) return start;

    // Нормализуем вектор направления и умножаем на расстояние
    double offsetX = (dx / length) * distance;
    double offsetY = (dy / length) * distance;

    // Вычисляем новую точку
    return {
        start.x + offsetX,
        start.y + offsetY
    };
}

} // namespace domain
