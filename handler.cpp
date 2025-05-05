#include "handler.h"

namespace dxf {

domain::RawData ConvertDataToRawSketch(const dx_data& data) {
    domain::RawData result;

    // Общая лямбда для обработки полилиний
    auto process_polyline = [](auto* polyline) {
        domain::RawPolyline new_layer;

        // Оптимизированное преобразование цвета
        switch(polyline->color) {
        case 2:  new_layer.ori = domain::ORI::PERP;  break;
        case 5:  new_layer.ori = domain::ORI::ZERO;  break;
        default: new_layer.ori = domain::ORI::OTHER; break;
        }

        // Предварительное выделение памяти для точек
        const auto& vertices = polyline->vertlist;
        new_layer.polyline.reserve(vertices.size());

        // Быстрое преобразование точек
        for(const auto& vertex : vertices) {
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

        switch(spline->color) {
        case 2:  new_layer.ori = domain::ORI::PERP;  break;
        case 5:  new_layer.ori = domain::ORI::ZERO;  break;
        default: new_layer.ori = domain::ORI::OTHER; break;
        }

        const auto& points = spline->controllist;
        new_layer.polyline.reserve(points.size());

        for(const auto& point : points) {
            new_layer.polyline.emplace_back(point->x, point->y);
        }
        return new_layer;
    };

    for(const auto& entity : data.mBlock->ent) {
        switch(entity->eType) {
        case DRW::ETYPE::POLYLINE:
        case DRW::ETYPE::LWPOLYLINE: // Объединенная обработка
            result.emplace_back(process_polyline(
                static_cast<DRW_Polyline*>(entity)));
            break;

        case DRW::ETYPE::SPLINE:
            result.emplace_back(process_spline(
                static_cast<DRW_Spline*>(entity)));
            break;

        default:
            // Пропуск необрабатываемых типов
            break;
        }
    }

    return domain::RemoveExtraDots(result, 1e-5, 1e-3);
}

void ConvertRawSketchToData(const domain::RawData& sketch, dx_data& data) {
    DRW_Layer* layer = new DRW_Layer;
    layer->name = "SketchLayer";
    layer->color = 1;  // Красный цвет

    for (const auto& layer : sketch) {
        DRW_LWPolyline* new_polyline = new DRW_LWPolyline;
        new_polyline->layer = "SketchLayer";
        for (const auto& point : layer.polyline) {
            new_polyline->addVertex(DRW_Vertex2D(point.x, point.y, 0.));
        }
        data.mBlock->ent.push_back(new_polyline);
    }
}

bool DxfHandler::ImportFile(std::string file_name) {
    dx_iface input;

    return input.fileImport(file_name, &input_data_);
}
bool DxfHandler::ExportFile(std::string file_name, DRW::Version version, bool is_binary) {
    dx_iface output;

    return output.fileExport(file_name, version, false, &output_data_);
}

domain::RawData DxfHandler::GetRawSketchFromData() const {
    return ConvertDataToRawSketch(input_data_);
}

void DxfHandler::PutRawSketchToData(const domain::RawData raw_sketch) {
    ConvertRawSketchToData(raw_sketch, output_data_);
}

} // namespace dxf
