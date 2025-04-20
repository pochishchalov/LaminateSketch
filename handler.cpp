#include "handler.h"

domain::Raw_sketch ConvertDataToRawSketch(const dx_data& data) {
    domain::Raw_sketch result;

    for (const auto& entity : data.mBlock->ent) {
        switch (entity->eType)
        {
        case DRW::ETYPE::POLYLINE:
        {
            auto polyline = static_cast<DRW_Polyline*>(entity);
            auto& new_polyline = result.emplace_back(domain::Polyline{});
            for (const auto& vertex : polyline->vertlist) {
                new_polyline.emplace_back(domain::Point(
                    static_cast<float>(vertex->basePoint.x),
                    static_cast<float>(vertex->basePoint.y))
                );
            }
        }
        break;
        default:
            break;
        }
    }

    return result;
}

void ConvertRawSketchToData(const domain::Raw_sketch& sketch, dx_data& data) {
    DRW_Layer* layer = new DRW_Layer;
    layer->name = "SketchLayer";
    layer->color = 1; // Красный цвет

    for (const auto& polyline : sketch) {
        DRW_LWPolyline* new_polyline = new DRW_LWPolyline;
        new_polyline->layer = "SketchLayer";
        for (const auto& point : polyline) {
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

domain::Raw_sketch DxfHandler::GetRawSketchFromData() const {
    return ConvertDataToRawSketch(input_data_);
}

void DxfHandler::PutRawSketchToData(const domain::Raw_sketch raw_sketch) {
    ConvertRawSketchToData(raw_sketch, output_data_);
}
