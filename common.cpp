#include "common.h"

namespace domain {

    void PrintPoint(const Point point) {
        std::cout << std::setprecision(10) << point.x << ' ' << point.y;
    }

    // Находит точку пересечения двух отрезков
    std::optional<Point> FindSegmentsIntersection(Point p1, Point p2, Point p3, Point p4) {
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
        if (IsMoreOrEqual(t, 0.f) && IsLessOrEqual(t, 1.0f)
            && IsMoreOrEqual(u, 0.f) && IsLessOrEqual(u, 1.0f))
        {
            return Point(static_cast<float>(p1.x + t * (p2.x - p1.x)),
                static_cast<float>(p1.y + t * (p2.y - p1.y)));
        }

        return std::nullopt;
    }

    // Находит пересечение двух бесконечных прямых, заданных двумя точками
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

        return Point(static_cast<float>(p1.x + dx1 * t),
            static_cast<float>(p1.y + dy1 * t));
    }


    // Находит точку пересечения луча, направленного из точки 'start' под углом 'angle' (в радианах),
    // с границей прямоугольника с начальной точкой 'х = 0, y = 0'  и габаритами 'width x height'
    // внутри которого располагается точка 'start' 
    Point FindBoundaryIntersection(Point start, double angle, float width, float height) {

        // Если гарантируется что точка внутри прямоугольника, то проверку можно убрать
        if (start.x < 0.f || start.x > width
            || start.y < 0.f || start.y > height)
        {
            return start;
        }

        // Направляющий вектор луча
        double dx = cos(angle);
        double dy = sin(angle);

        // Обработка вертикальных/горизонтальных лучей
        if (IsZero(dx)) { // Вертикальный луч
            return { start.x, (dy < 0.) ? 0.f : height }; // Координата 'y' в зависимости от направления
        }
        if (IsZero(dy)) { // Горизонтальный луч
            return { (dx < 0.) ? 0.f : width, start.y }; // Координата 'x' в зависимости от направления
        }

        // Проверка пересечений с вертикальными границами (x = 0 и x = width)
        float v_border = (dx < 0.) ? 0.f : width; // Выбираем вертикальную границу исходя из направления
        float y = static_cast<float>(start.y + ((v_border - start.x) / (dx / dy)));
        if (IsEqual(y, 0.) || IsEqual(y, height)
            || (y > 0.f && y < height))
        {
            return { v_border, y };
        }

        // Проверка пересечений с горизонтальными границами (y = 0 и y = height)
        float h_border = (dy < 0.) ? 0.f : height; // Выбираем горизонтальную границу исходя из направления
        float x = static_cast<float>(start.x + ((h_border - start.y) / (dy / dx)));
        if (IsEqual(x, 0.) || IsEqual(x, width)
            || (x > 0.f && x < width))
        {
            return { x, h_border };
        }

        return start; // Если точка находится вне границы и пересечение отсутствует
    }

    // Проверяет находится ли точка внутри многоугольника
    bool IsPointInPolygon(const Point test, const Polygon& polygon) {

        bool is_inside = false;

        for (size_t i = 0, j = polygon.GetNumOfPoints() - 1; i < polygon.GetNumOfPoints(); j = i++) {
            const Point& p1 = polygon.GetPoints()[i];
            const Point& p2 = polygon.GetPoints()[j];

            // Проверка на совпадение с вершиной
            if (IsEqual(test.x, p1.x) && IsEqual(test.y, p1.y)) {
                return true;
            }

            // Вертикальный луч пересекает ребро только если:
            // 1. Тестовая X-координата между X-координатами ребра
            // 2. Y-координата пересечения выше тестовой Y

            // Игнорируем вертикальные ребра (X не меняется)
            if (IsZero(p1.x - p2.x)) {
                if (IsZero(test.x - p1.x)) {
                    // Проверка Y-диапазона
                    if (IsMoreOrEqual(test.y, std::min(p1.y, p2.y)) &&
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

    Point GetPerpendicularPoint(const Point start, const Point end, double offset) {
        double dx = end.x - start.x;
        double dy = end.y - start.y;
        double len = std::hypot(dx, dy);
        if (IsZero(len)) {
            return start;
        }
        double perp_x = -dy / len * offset;  // Перпендикуляр направлен 
        double perp_y = dx / len * offset;   // влево от линии

        return { static_cast<float>(start.x + perp_x),
            static_cast<float>(start.y + perp_y) };
    }


    // Смещает ломаную линию на расстояние d (влево относительно направления обхода)
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
                        static_cast<float>((p_prev1.x + p_next1.x) / 2),
                        static_cast<float>((p_prev1.y + p_next1.y) / 2)
                    };
                    result.push_back(avg);
                }
            }
        }
        return result;
    }

    // Функция для поиска и удаления одного пересечения
    Polyline RemoveOneIntersection(const Polyline& input, bool& found) {
        const size_t n = input.size();
        found = false;

        if (n < 4) {
            return input;
        }

        // Ищем пересечение с наименьшим индексом j
        for (size_t i = 0; i < n - 1; ++i) {
            for (size_t j = i + 2; j < n - 1; ++j) {
                const auto intersection = FindSegmentsIntersection(input[i], input[i + 1],
                    input[j], input[j + 1]);
                if (intersection.has_value()) {
                    std::vector<Point> result;
                    result.reserve(i + 2 + (n - j - 1));

                    // Первая часть
                    for (size_t k = 0; k <= i; ++k)
                        result.push_back(input[k]);

                    // Точка пересечения
                    result.push_back(intersection.value());

                    // Вторая часть
                    for (size_t k = j + 1; k < n; ++k)
                        result.push_back(input[k]);

                    found = true;
                    return result;
                }
            }
        }
        return input;
    }

    // Основная функция для удаления всех пересечений
    Polyline RemoveSelfIntersections(const Polyline& input) {
        Polyline current = input;
        bool intersectionFound;

        do {
            intersectionFound = false;
            current = RemoveOneIntersection(current, intersectionFound);
        } while (intersectionFound && current.size() >= 4);

        return current;
    }

    bool IsLineIntersectsPolyline(const Point begin, const Point end, const Polyline& polyline) {

        for (size_t i = 1; i < polyline.size(); ++i) {
            if (FindSegmentsIntersection(begin, end, polyline[i - 1], polyline[i]).has_value()) {
                return true;
            }
        }
        return false;
    }

    bool IsPolylinePointInPolygon(const Polyline& polyline, const Polygon& poly) {
        for (const auto point : polyline) {
            if (IsPointInPolygon(point, poly)) {
                return true;
            }
        }
        return false;
    }

} // namespace domain
