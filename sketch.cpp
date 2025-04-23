#include <limits>

#include "sketch.h"

namespace sketch {

    using namespace domain;

    domain::Raw_sketch Sketch::GetRawSketch() const {
        Raw_sketch result;

        for (const auto& layer : layers_) {
            for (const auto& ply : layer) {
                auto& new_layer = result.emplace_back(Raw_layer{});

                new_layer.orientation_ = ply.orientation_;

                for (const auto& point : ply.nodes_) {
                    new_layer.polyline_.emplace_back(point.point_);
                }
            }
        }

        return result;
    }

    void Sketch::FillSketch(domain::Raw_sketch &&raw_sketch){

        auto [width, height] = MoveRawSketchToZeroAndGetDimensions(raw_sketch);

        width_ = width, height_ = height;

        auto layers = ConvertRawSketch(std::move(raw_sketch));

        layers_ = (std::move(layers));
    }

} // namespace sketch

namespace domain {

    // Определяет является ли ломаная линия верхним слоем (сегментом слоя)
    bool IsUpperPolyline(const Polyline& input, Raw_sketch raw_sketch) {
        // Смещаем проверяемую линию вверх и убираем самопересечения
        auto offset = RemoveSelfIntersections(OffsetPolyline(input, 3.));  // Смещение на 3 достаточно для всех случаев
                                                                           // не существует слоистых метериалов с толщиной монослоя более 3

        // Проверка пересечения остальных линий эскиза с линиями соединяющими
        // начальные и конечные точки 'input' и 'offset'
        for (const auto& layer : raw_sketch) {
            if (input == layer.polyline_) {
                continue;  // Пропускаем проверяемую линию
            }
            if (IsLineIntersectsPolyline(*input.begin(), *offset.begin(), layer.polyline_)
                || IsLineIntersectsPolyline(input.back(), offset.back(), layer.polyline_))
            {
                return false;
            }
        }

        // Создаем многоугольник разворачивая точки offset
        // и добавляя эти ломаные в многоугольник

        std::reverse(offset.begin(), offset.end());
        Polygon polygon;
        polygon.AddPolyline(input);
        polygon.AddPolyline(std::move(offset));

        for (const auto& layer : raw_sketch) {
            if (input == layer.polyline_) {
                continue;  // Пропускаем проверяемую линию
            }
            if (IsPolylinePointInPolygon(layer.polyline_, polygon))
            {
                return false;
            }
        }

        return true;
    }

    struct Borders{
        float left = 0.f;
        float bottom = 0.f;
        float right = 0.f;
        float top = 0.f;
    };

    // Возвращает границы "сырого" эскиза
    Borders GetBorders(const Raw_sketch& raw_sketch) {
        Borders result{
                .left = std::numeric_limits<float>::max(),
                .bottom = std::numeric_limits<float>::max(),
                .right = std::numeric_limits<float>::min(),
                .top = std::numeric_limits<float>::min()
        };


        for (const auto& layer : raw_sketch) {
            for (const auto point : layer.polyline_) {
                result.left = std::min(result.left, point.x);
                result.bottom = std::min(result.bottom, point.y);
                result.right = std::max(result.right, point.x);
                result.top = std::max(result.top, point.y);
            }
        }

        return result;
    }

    // Перемещает "сырой" эскиз в начало координат (0,0), возвращает его габариты widht, height
    std::pair<float, float> MoveRawSketchToZeroAndGetDimensions(Raw_sketch& raw_sketch) {
        const auto borders = GetBorders(raw_sketch);

        for (auto& layer : raw_sketch) {
            for (auto& point : layer.polyline_) {
                point.x -= borders.left;
                point.y -= borders.bottom;
            }
        }

        return std::make_pair((borders.right - borders.left), (borders.top - borders.bottom));
    }

    // Оптимизирует линии эскиза так, чтобы точки линии шли слева направо
    void StartPointOptimization(Raw_sketch& raw_sketch) {
        for (auto& layer : raw_sketch) {
            const float first_point_x = layer.polyline_.begin()->x;
            const float last_point_x = layer.polyline_.back().x;

            if (first_point_x > last_point_x) {
                std::reverse(layer.polyline_.begin(), layer.polyline_.end());
            }
        }
    }

    // Возвращает итераторы на верхние слои эскиза
    std::vector<Raw_sketch::const_iterator>GetUpperPlies(const Raw_sketch& raw_sketch) {
        std::vector<Raw_sketch::const_iterator> result;

        for (auto it = raw_sketch.begin(); it != raw_sketch.end(); ++it) {

            if (IsUpperPolyline(it->polyline_, raw_sketch)) {
                result.push_back(it);
            }
        }

        return result;
    }

    void AddLayer(std::vector<Raw_sketch::const_iterator> upper_plies,
        std::vector<sketch::Layer>&layers) {

        std::sort(upper_plies.begin(), upper_plies.end(),         // Сортируем полученные верхние сегменты
            [](const auto& lhs, const auto& rhs) {
                return lhs->polyline_.begin()->x < rhs->polyline_.begin()->x;
            }
        );

        auto& new_layer = layers.emplace_back(sketch::Layer{});   // Добавляем новый слой
        new_layer.reserve(upper_plies.size());                    // Резервируем место для сегметов слоя

        if (layers.empty()) {
            for (const auto& ply : upper_plies) {
                auto& new_ply = new_layer.emplace_back(sketch::Ply{});
                new_ply.nodes_.reserve(ply->polyline_.size());

                new_ply.orientation_ = ply->orientation_;

                for (size_t i = 0; i < ply->polyline_.size(); ++i)
                {
                    new_ply.nodes_.emplace_back(sketch::Node(*(ply->polyline_.begin() + i)));
                }

                }
            }
        else {
            for (const auto& ply : upper_plies) {                        // В дальнейшем узлы верхних сегментов
                auto& new_ply = new_layer.emplace_back(sketch::Ply{});   // должны проецироваться на нижние сегменты
                new_ply.nodes_.reserve(ply->polyline_.size() * 2);

                new_ply.orientation_ = ply->orientation_;

                for (size_t i = 0; i < ply->polyline_.size(); ++i)
                {
                    new_ply.nodes_.emplace_back(sketch::Node(*(ply->polyline_.begin() + i)));
                }
            }
        }
    }

    std::vector<sketch::Layer> ConvertRawSketch(Raw_sketch raw_sketch) {
        std::vector<sketch::Layer> result;
        result.reserve(raw_sketch.size());

        StartPointOptimization(raw_sketch);  // Переворачиваем линии эскиза если они идут справа налево

        while (!raw_sketch.empty()) {        // Создаем слои эскиза из линий "сырого" эскиза

            auto upper_plies = GetUpperPlies(raw_sketch);

            AddLayer(upper_plies, result);

            for (const auto pos : upper_plies) {
                raw_sketch.erase(pos);
            }
        }
        return result;
    }

} // namespace domain
