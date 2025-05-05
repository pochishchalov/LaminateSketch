#include <limits>

#include "sketch.h"

namespace domain {

// Определяет является ли ломаная линия верхним слоем (сегментом слоя)
bool IsUpperPolyline(const Polyline& input, const RawData& raw_sketch) {
    // Смещаем проверяемую линию вверх и убираем самопересечения
    auto offset = RemoveSelfIntersections(OffsetPolyline(input, 3.));  // Смещение на 3 достаточно для всех случаев
                                                                       // не существует слоистых материалов с толщиной монослоя более 3

// Проверка пересечения остальных линий эскиза с линиями соединяющими
// начальные и конечные точки 'input' и 'offset'
    for (const auto& layer : raw_sketch) {
        if (&input == &layer.polyline) {
            continue;  // Пропускаем проверяемую линию
        }
        if (IsLineIntersectsPolyline(*input.begin(), *offset.begin(), layer.polyline)
            || IsLineIntersectsPolyline(input.back(), offset.back(), layer.polyline))
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
        if (&input == &layer.polyline) {
            continue;  // Пропускаем проверяемую линию
        }
        if (IsPolylinePointInPolygon(layer.polyline, polygon))
        {
            return false;
        }
    }
    return true;
}

struct Borders {
    double left = 0.;
    double bottom = 0.;
    double right = 0.;
    double top = 0.;
};

// Возвращает границы "сырого" эскиза
Borders GetBorders(const RawData& raw_sketch) {
    Borders result{
        .left = std::numeric_limits<double>::max(),
        .bottom = std::numeric_limits<double>::max(),
        .right = std::numeric_limits<double>::min(),
        .top = std::numeric_limits<double>::min()
    };


    for (const auto& layer : raw_sketch) {
        for (const auto point : layer.polyline) {
            result.left = std::min(result.left, point.x);
            result.bottom = std::min(result.bottom, point.y);
            result.right = std::max(result.right, point.x);
            result.top = std::max(result.top, point.y);
        }
    }

    return result;
}

// Перемещает "сырой" эскиз в начало координат (0,0), возвращает его габариты widht, height
std::pair<float, float> MoveRawSketchToZeroAndGetDimensions(RawData& raw_sketch) {
    const auto borders = GetBorders(raw_sketch);

    for (auto& layer : raw_sketch) {
        for (auto& point : layer.polyline) {
            point.x -= borders.left;
            point.y -= borders.bottom;
        }
    }

    return std::make_pair((borders.right - borders.left), (borders.top - borders.bottom));
}

// Оптимизирует линии эскиза так, чтобы точки линии шли слева направо
void StartPointOptimization(RawData& raw_sketch) {
    for (auto& layer : raw_sketch) {
        const float first_point_x = layer.polyline.begin()->x;
        const float last_point_x = layer.polyline.back().x;

        if (first_point_x > last_point_x) {
            std::reverse(layer.polyline.begin(), layer.polyline.end());
        }
    }
}

// Возвращает итераторы на верхние слои эскиза
std::vector<RawData::iterator>GetUpperPlies(RawData& raw_sketch) {
    std::vector<RawData::iterator> result;

    for (RawData::iterator it = raw_sketch.begin(); it != raw_sketch.end(); ++it) {

        if (IsUpperPolyline(it->polyline, raw_sketch)) {
            result.push_back(it);
        }
    }

    return result;
}

void AddLayer(std::vector<RawData::iterator> upper_plies, ls::Data& data,
    std::list<std::pair<ls::NodePos, bool>>& unused_nodes) {

    std::sort(upper_plies.begin(), upper_plies.end(),         // Сортируем полученные верхние сегменты слева направо
        [](const auto& lhs, const auto& rhs) {
            return lhs->polyline.begin()->x < rhs->polyline.begin()->x; }
    );

    unsigned short layer_pos = data.LayersCount();              // Объявляем до создания слоя чтобы не вычитать единицу

    auto& new_layer = data.AddLayer();   // Добавляем новый слой
    new_layer.reserve(upper_plies.size());                 // Резервируем место для сегметов слоя


    for (auto& ply : upper_plies) {

        auto& new_ply = new_layer.AddPly();  // Добавляем новый сегмент

        new_ply.ori = ply->ori;

        if (layer_pos == 0) {                                 // Если слой первый, то просто преобразуем
            new_ply.reserve(ply->pointsCount());   // все точки в узлы и добавляем в сегмент

            for (const auto& point : ply->polyline) {
                new_ply.AddNode(ls::Node{ point });
            }
        }
        else {                                                      // Если слой не первый
            new_ply.reserve(ply->pointsCount() * 2);                // могут добавится дополнительные узлы

            for (size_t i = 0; i < ply->pointsCount(); ++i) {
                new_ply.AddNode(ls::Node{ ply->polyline[i] });
                if (i != 0) {
                    //TieNodes(new_ply, data, unused_nodes);          // Функция связывания неиспользованных узлов с новыми
                }
            }
        }
    }
        
    // Очищаем от использованных узлов
    for (auto iter = unused_nodes.begin(); iter != unused_nodes.end(); ++iter) {
        if (iter->second == true) {
            iter = unused_nodes.erase(iter);
        }
    }

    // Добавляем позиции всех точек нового слоя в категорию неиспользованных
    for (unsigned short ply_pos = 0; ply_pos < new_layer.size(); ++ply_pos) {
        for (unsigned short node_pos = 0; node_pos < new_layer[ply_pos].size(); ++node_pos) {
            unused_nodes.emplace_back(std::make_pair(ls::NodePos{ .layer_pos = layer_pos,
                                                       .ply_pos = ply_pos,
                                                       .node_pos = node_pos }, false));
        }
    }
        
}

ls::Data ConvertRawSketch(RawData&& raw_sketch) {
    ls::Data result;
    result.ReserveLayers(raw_sketch.size()); // Слоев не может быть больше чем ломаных в сыром эскизе

    StartPointOptimization(raw_sketch);    // Переворачиваем линии эскиза если они идут справа налево

    std::list<std::pair<ls::NodePos, bool>> unused_nodes;  // Для хранения позиций узлов не связанных с другими

    while (!raw_sketch.empty()) {          // Создаем слои эскиза из линий "сырого" эскиза

       auto upper_plies = GetUpperPlies(raw_sketch);

        if (upper_plies.empty()) {          // Ошибка обработки
            return {};
        }

        AddLayer(upper_plies, result, unused_nodes);  // Добавляем слои

        for (const auto pos : upper_plies) {          // Удаляем верхние слои из сырого эскиза
            raw_sketch.erase(pos);
        }
    }
    result.ReverseLayers();
    return result;
}

void ScaleLayers(ls::Data& layers, double scale)
{
    for (auto& layer : layers) {
        for (auto& ply : layer) {
            for (auto& node : ply) {
                node.point_.x *= scale;
                node.point_.y *= scale;
            }
        }
    }
}

} // namespace domain


namespace ls {

using namespace domain;
    
domain::RawData Sketch::GetRawSketch() const {
    RawData result;

    for (const auto& layer : optimized_data_) {
        for (const auto& ply : layer) {
            auto& new_layer = result.emplace_back(RawPolyline{});

            new_layer.ori = ply.ori;

            for (const auto& node : ply) {
                new_layer.polyline.emplace_back(node.point_);
            }
        }
    }

    return result;
}
    
bool Sketch::FillSketch(domain::RawData&& raw_sketch) {

   auto [width, height] = MoveRawSketchToZeroAndGetDimensions(raw_sketch);

   width_ = width, height_ = height;

   auto data = ConvertRawSketch(std::move(raw_sketch));

    if (data.IsEmpty()) {
        return false;
    }
    else {
            
        original_data_ = (std::move(data));

        OptimizeSketchForWidth(Sketch::DEFAULT_WIDTH);

        return true;
    }
}

void Sketch::ScaleSketch(double scale) {
    ScaleLayers(optimized_data_, scale);
}

void Sketch::OptimizeSketchForWidth(int width) {
   auto temp_layers_ = original_data_;

   //ScaleLayers(temp_layers_, width / width_);

   //double offset_factor = 3. / СalculateMinDistanceBetweenPlies(temp_layers_);

   //optimized_layers_ = OffsetLayers(temp_layers_, 2);

   optimized_data_ = temp_layers_;

}

} // namespace ls
