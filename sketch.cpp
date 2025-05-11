
#include <limits>
#include <numeric>

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
std::pair<double, double> MoveRawSketchToZeroAndGetDimensions(RawData& raw_sketch) {
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
        const double first_point_x = layer.polyline.begin()->x;
        const double last_point_x = layer.polyline.back().x;

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

// Общая функция для проверки и установки соединения
// Возвращает результат соединения с левой 'first' и правой 'second' точкой
std::pair<bool, bool> TryConnectIntersection(const std::optional<Point>& intersect,
                                             ls::Node& first, ls::Node& second,
                                             ls::Node& connectable_node) {
    if (!intersect) {
        return { false, false };
    }

    bool is_first = ApproximatelyEqual(*intersect, first.point, 1e-4);
    bool is_second = ApproximatelyEqual(*intersect, second.point, 1e-4);
    if (!is_first && !is_second) {
        return { false, false };
    }

    ls::Node& target = is_first ? first : second;
    if (!target.top_pos && !connectable_node.bottom_pos) {
        target.top_pos = connectable_node.pos;
        connectable_node.bottom_pos = target.pos;
        return { is_first, is_second };
    }
    return { false, false };
}

// Соединяет неиспользованные точки с сегментом граниченным узлами first и last
void ConnectLineWithNodes(ls::Node& first, ls::Node& second, ls::Data& data,
                          std::vector<std::pair<ls::NodePos, bool>>& unused_nodes)
{
    for (auto& [node_pos, is_tied] : unused_nodes) {

        ls::Node& connectable = data.GetNode(node_pos);

        if (connectable.bottom_pos.has_value()) {
            continue;
        }

        const bool is_first_node = data.IsFirstNodeInPly(node_pos);
        const bool is_last_node = data.IsLastNodeInPly(node_pos);

        std::vector<ls::Node*> neighbors;
        // Соседний узел слева
        if (!is_first_node) {
            neighbors.push_back(&data.GetNode(ls::NodePos{
                                                          node_pos.layer_pos, node_pos.ply_pos,
                                                          static_cast<unsigned short>(node_pos.node_pos - 1) }));
        }
        // Соседний узел справа
        if (!is_last_node) {
            neighbors.push_back(&data.GetNode(ls::NodePos{
                                                          node_pos.layer_pos, node_pos.ply_pos,
                                                          static_cast<unsigned short>(node_pos.node_pos + 1) }));
        }

        // Обработка биссектрисы
        if (!is_first_node && !is_last_node) {

            std::pair<Point, Point> bisect_line = {
                СalculateBisector(neighbors[0]->point, connectable.point, neighbors[1]->point, 3.0),
                СalculateBisector(neighbors[0]->point, connectable.point, neighbors[1]->point, -3.0)
            };

            auto [is_first, is_second] = TryConnectIntersection(
                FindSegmentsIntersection(first.point, second.point, bisect_line.first, bisect_line.second, 1e-4),
                first, second, connectable);

            if (is_first || is_second) {
                is_tied = true;
                if (is_second) {
                    return;
                }
                continue;
            }
        }

        std::vector<Point> intersections;
        bool is_complete = false;

        // Обработка перпендикуляров к соседям
        for (ls::Node* neighbor : neighbors) {

            std::pair<Point, Point> perp_line = {
                GetPerpendicularPoint(connectable.point, neighbor->point, 3.0),
                GetPerpendicularPoint(connectable.point, neighbor->point, -3.0)
            };

            auto intersection = FindSegmentsIntersection(first.point, second.point,
                                                         perp_line.first, perp_line.second, 1e-4);

            auto [is_first, is_second] = TryConnectIntersection(
                intersection, first, second, connectable);

            if (is_first || is_second) {
                is_tied = true;
                if (is_second) {
                    return;
                }
                is_complete = true;
                break;
            }
            if (intersection.has_value()) {
                intersections.push_back(intersection.value());
            }
        }
        if (is_complete) {
            continue;
        }

        // Создание нового узла при необходимости
        if (intersections.size() > 0) {

            Point intersection_point = intersections[0];
            if (intersections.size() == 2) {
                if (IsParallelLines(first.point, second.point, connectable.point, neighbors[1]->point)) {
                    intersection_point = intersections[1];
                }
            }

            ls::Node& new_node = data.InsertNode(second.pos, ls::Node{ .point = intersection_point, .pos = second.pos });
            new_node.top_pos.emplace(connectable.pos);
            connectable.bottom_pos.emplace(new_node.pos);
            is_tied = true;
        }
    }
}

void ConnectNodes(ls::Ply& ply, ls::Data& data, std::vector<std::pair<ls::NodePos, bool>>& unused_nodes) {
    // Вызов GetLastNode() необходим каждый раз, т.к. в сегмент 'ply' могут добавляться новые узлы
    for (auto pos = ply.GetFirstNode().pos; pos <= ply.GetLastNode().pos; ++pos.node_pos) {
        if (pos.node_pos != 0) {
            ls::Node& first = data.GetNode(ls::NodePos{ .layer_pos = pos.layer_pos,
                                                       .ply_pos = pos.ply_pos,
                                                       .node_pos = static_cast<unsigned short>(pos.node_pos - 1) });
            ls::Node& second = data.GetNode(pos);

            ConnectLineWithNodes(first, second, data, unused_nodes);
        }
    }
}

void AddLayer(std::vector<RawData::iterator> upper_plies, ls::Data& data,
              std::vector<std::pair<ls::NodePos, bool>>& unused_nodes)
{
    // Сортировка сегментов слева направо
    std::sort(upper_plies.begin(), upper_plies.end(),
              [](const auto& lhs, const auto& rhs) {
                  return lhs->polyline.begin()->x < rhs->polyline.begin()->x;
              });

    const unsigned short layer_pos = data.LayersCount(); // Опережающее присвоение чтобы не вычитать единицу
    auto& new_layer = data.AddLayer();
    new_layer.reserve(upper_plies.size());

    const bool is_first_layer = (layer_pos == 0);

    for (auto& ply : upper_plies) {
        const unsigned short ply_pos = data.GetLayer(layer_pos).PliesCount(); // Опережающее присвоение чтобы не вычитать единицу
        auto& new_ply = new_layer.AddPly();

        const auto points_count = ply->PointsCount();

        new_ply.ori = ply->ori;
        new_ply.reserve(is_first_layer ? points_count : points_count + unused_nodes.size());

        // Добавляем узлы в сегмент
        for (unsigned short i = 0; i < points_count; ++i) {
            new_ply.AddNode(ls::Node{
                .point = ply->GetPoint(i),
                .pos = {.layer_pos = layer_pos,
                        .ply_pos = ply_pos,
                        .node_pos = i}
            });
        }
        // Соединяем узлы нового сегмента с неиспользованными узлами
        if (!is_first_layer) {
            ConnectNodes(new_ply, data, unused_nodes);
        }
    }

    // Очистка от использованных узлов
    unused_nodes.erase(
        std::remove_if(unused_nodes.begin(), unused_nodes.end(),
                       [](const auto& p) { return p.second; }),
        unused_nodes.end()
        );

    // Узлы нового сегмента добавляются к неиспользованным
    size_t total_nodes = 0;
    for (const auto& ply : new_layer) {
        total_nodes += ply.PointsCount();
    }
    unused_nodes.reserve(unused_nodes.size() + total_nodes);

    for (unsigned short ply_pos = 0; ply_pos < new_layer.size(); ++ply_pos) {
        const auto& ply = new_layer[ply_pos];
        for (unsigned short node_pos = 0; node_pos < ply.size(); ++node_pos) {
            unused_nodes.emplace_back(
                ls::NodePos{ layer_pos, ply_pos, node_pos },
                false
                );
        }
    }
}

void ReverseLayers(ls::Data& data) {

    unsigned short correction_pos = data.LayersCount() - 1;

    for (auto& layer : data) {
        for (auto& ply : layer) {
            for (auto& node : ply) {
                node.pos.layer_pos = correction_pos - node.pos.layer_pos;
                if (auto top = node.top_pos.has_value()) {
                    node.top_pos.value().layer_pos = correction_pos - node.top_pos.value().layer_pos;
                }
                if (auto bottom = node.bottom_pos.has_value()) {
                    node.bottom_pos.value().layer_pos = correction_pos - node.bottom_pos.value().layer_pos;
                }
            }
        }
    }
    std::reverse(data.begin(), data.end());
}

ls::Data ConvertRawSketch(RawData&& raw_sketch) {
    ls::Data result;
    result.ReserveLayers(raw_sketch.size()); // Слоев не может быть больше чем ломаных в сыром эскизе

    StartPointOptimization(raw_sketch);    // Переворачиваем линии эскиза если они идут справа налево

    std::vector<std::pair<ls::NodePos, bool>> unused_nodes;  // Для хранения позиций узлов не связанных с другими

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
    ReverseLayers(result);
    return result;
}

void ScaleLayers(ls::Data& layers, double scale)
{
    for (auto& layer : layers) {
        for (auto& ply : layer) {
            for (auto& node : ply) {
                node.point.x *= scale;
                node.point.y *= scale;
            }
        }
    }
}

std::optional<ls::NodePos> TryGetNextPos(const ls::NodePos pos, ls::Data& layers) {
    if (!layers.IsLastNodeInPly(pos)) {
        return layers.GetLayer(pos.layer_pos).GetPly(pos.ply_pos).GetNode(pos.node_pos + 1).pos;
    }
    ls::Node& node = layers.GetNode(pos);
    if (node.top_pos.has_value()) {
        return TryGetNextPos(node.top_pos.value(), layers);
    }
    return std::nullopt;
}

double GetMinDistanceGroupNodes(const ls::NodePos pos, ls::Data& layers) {
    ls::Node node = layers.GetNode(pos);

    double result = std::numeric_limits<double>::max();

    while (node.top_pos.has_value()) {
        ls::Node top_node = layers.GetNode(node.top_pos.value());
        result = std::min(result, DistanceBetweenPoints(node.point, top_node.point));
        node = layers.GetNode(top_node.pos);
    }

    return result;
}

double GetMinDistanceBetweenPlies(ls::Data& layers) {

    double result = std::numeric_limits<double>::max();

    auto pos = layers.GetStartNodePos();

    while (true) {
        result = std::min(result, GetMinDistanceGroupNodes(pos, layers));
        auto next_pos = TryGetNextPos(pos, layers);
        if (next_pos.has_value()) {
            pos = layers.GetLowestNodePos(next_pos.value());
        }
        else {
            break;
        }
    }
    return result;
}

double GetMinDistanceBetweenGroupNodes(const ls::NodePos first, const ls::NodePos second, ls::Data& layers) {
    ls::Node first_node = layers.GetNode(first);
    ls::Node second_node = layers.GetNode(second);

    double result = DistanceBetweenPoints(first_node.point, second_node.point);

    while (true) {
        if (first_node.top_pos.has_value() && second_node.top_pos.has_value()) {
            first_node = layers.GetNode(first_node.top_pos.value());
            second_node = layers.GetNode(second_node.top_pos.value());
            result = std::min(result, DistanceBetweenPoints(first_node.point, second_node.point));
        }
        else {
            break;
        }
    }

    return result;
}

void CompressPairGroupNodes(const ls::NodePos first, const ls::NodePos second, ls::Data& layers, double max_distance) {
    ls::Node first_node = layers.GetNode(first);
    ls::Node second_node = layers.GetNode(second);

    auto mid_point = GetPointOnRay(first_node.point, second_node.point, max_distance);
    double dx = second_node.point.x - mid_point.x;
    double dy = second_node.point.y - mid_point.y;

    auto pos = second;

    while (true) {
        auto i_pos = pos;
        while (true) {
            ls::Node& changed_node = layers.GetNode(i_pos);
            changed_node.point.x -= dx;
            changed_node.point.y -= dy;
            if (changed_node.top_pos.has_value()) {
                i_pos = changed_node.top_pos.value();
            }
            else {
                break;
            }
        }
        auto next_pos = TryGetNextPos(pos, layers);
        if (next_pos.has_value()) {
            pos = layers.GetLowestNodePos(next_pos.value());
        }
        else {
            break;
        }
    }
}

void CompressSketch(ls::Data& layers, double max_distance) {
    auto first = layers.GetStartNodePos();
    auto second = TryGetNextPos(first, layers).value();

    while (true) {
        double distance = GetMinDistanceBetweenGroupNodes(first, second, layers);
        if (max_distance < distance) {
            CompressPairGroupNodes(first, second, layers, max_distance);
        }
        auto next_pos = TryGetNextPos(second, layers);
        if (next_pos.has_value()) {
            first = second;
            second = layers.GetLowestNodePos(next_pos.value());
        }
        else {
            break;
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
                new_layer.polyline.emplace_back(node.point);
            }
        }
    }

    return result;
}

bool Sketch::FillSketch(domain::RawData&& raw_sketch) {

    auto [width, height] = MoveRawSketchToZeroAndGetDimensions(raw_sketch);

    width_ = width, height_ = height;

    auto data = ls::ConvertRawSketch(std::move(raw_sketch));

    if (data.IsEmpty()) {
        return false;
    }
    else {

        original_data_ = (std::move(data));

        OptimizeSketch(Sketch::DEFAULT_OFFSET, Sketch::DEFAULT_SEG_LEN);

        return true;
    }
}

void Sketch::ScaleSketch(double scale) {
    ScaleLayers(optimized_data_, scale);
}

void Sketch::OptimizeSketch(double offset, double segment_len) {
    auto temp_layers_ = original_data_;

    double min_distance = GetMinDistanceBetweenPlies(temp_layers_);

    ScaleLayers(temp_layers_, offset / min_distance);

    CompressSketch(temp_layers_, segment_len);

    optimized_data_ = temp_layers_;

}

} // namespace ls
