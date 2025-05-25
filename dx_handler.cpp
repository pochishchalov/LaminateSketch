#include <vector>
#include <algorithm>
#include <iterator>

#include "dx_handler.h"

namespace dx {

domain::RawData ConvertDataToRawSketch(const Data& data) {
    domain::RawData result;

    // Общая лямбда для обработки полилиний
    auto process_polyline = [](auto* polyline) {
        domain::RawPolyline new_layer;

        // Оптимизированное преобразование цвета
        switch (polyline->color) {
        case 2:  new_layer.orientation = domain::Orientation::Perpendicular;  break;
        case 5:  new_layer.orientation = domain::Orientation::Zero;  break;
        default: new_layer.orientation = domain::Orientation::Other; break;
        }

        // Предварительное выделение памяти для точек
        const auto& vertices = polyline->vertlist;
        new_layer.polyline.reserve(vertices.size());

        // Быстрое преобразование точек
        for (const auto& vertex : vertices) {
            new_layer.polyline.emplace_back(
                vertex->basePoint.x,
                vertex->basePoint.y
                );
        }
        return new_layer;
    };

    // Общая лямбда для обработки сплайнов
    auto process_spline = [](auto* spline) {
        domain::RawPolyline new_layer;

        switch (spline->color) {
        case 2:  new_layer.orientation = domain::Orientation::Perpendicular;  break;
        case 5:  new_layer.orientation = domain::Orientation::Zero;  break;
        default: new_layer.orientation = domain::Orientation::Other; break;
        }

        const auto& points = spline->controllist;
        new_layer.polyline.reserve(points.size());

        for (const auto& point : points) {
            new_layer.polyline.emplace_back(point->x, point->y);
        }
        return new_layer;
    };

    for (const auto& entity : data.mBlock->ent) {
        switch (entity->eType) {
        case DRW::ETYPE::POLYLINE:
        case DRW::ETYPE::LWPOLYLINE: // Объединенная обработка
            result.emplace_back(process_polyline(
                static_cast<DRW_Polyline*>(entity.get())));
            break;

        case DRW::ETYPE::SPLINE:
            result.emplace_back(process_spline(
                static_cast<DRW_Spline*>(entity.get())));
            break;

        default:
            // Пропуск необрабатываемых типов
            break;
        }
    }

    return domain::RemoveExtraDots(result, 1e-3);
}

void ConvertRawSketchToData(const domain::RawData& sketch, Data& data) {

    for (const auto& sketch_layer : sketch) {
        auto new_polyline = std::make_unique<DRW_LWPolyline>();
        new_polyline->layer = "SketchLayer";
        switch (sketch_layer.orientation)
        {
        case domain::Orientation::Zero: new_polyline->color = 4;
            break;
        case domain::Orientation::Perpendicular: new_polyline->color = 2;
            break;
        case domain::Orientation::Other: new_polyline->color = 1;
            break;
        default:
            break;
        }

        for (const auto& point : sketch_layer.polyline) {
            new_polyline->addVertex(DRW_Vertex2D(point.x, point.y, 0.));
        }

        data.mBlock->ent.push_back(std::move(new_polyline));
    }
}

bool Handler::importFile(std::string file_name) {
    Iface input;
    inputData = {};
    return input.fileImport(file_name, &inputData);
}
bool Handler::exportFile(std::string file_name, DRW::Version version, bool is_binary) {
    Iface output;
    bool success = output.fileExport(file_name, version, is_binary, &outputData);
    outputData = {};
    return success;
}

domain::RawData Handler::getRawSketch() const {
    return ConvertDataToRawSketch(inputData);
}

void Handler::putRawSketch(const domain::RawData raw_sketch) {
    ConvertRawSketchToData(raw_sketch, outputData);
}

} // namespace dxf
