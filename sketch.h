#pragma once

#include <vector>

#include "common.h"

namespace sketch {

    struct Node {
        Node() = default;
        Node(domain::Point point)
            :point_(point)
        {
        }
        domain::Point point_;
        Node* top_node_ = nullptr;
        Node* bottom_node_ = nullptr;
    };

    using Ply = std::vector<Node>;
    using Layer = std::vector<Ply>;

    /*
    struct Substrate_Node {
        Substrate_Node(Node* node)
            :node_(node)
        {
        }
        Node* node_;
        bool left_border = true;
        bool right_border = true;
    };

    using Substrate = std::list<Substrate_Node>;
    */

    class Sketch {

    public:
        Sketch()
            :width_(0.f)
            ,height_(0.f)
        {
        }

        float Width() const noexcept { return width_; }
        float Height() const noexcept { return height_; }
        bool IsEmpty() const noexcept { return layers_.empty();}

        const std::vector<Layer>& GetSketchLayers() const { return layers_; }

        // Возвращает "сырой" эскиз для записи в dxf файл
        domain::Raw_sketch GetRawSketch() const;

        // Наполняет эскиз данными из "сырого" эскиза
        void FillSketch(domain::Raw_sketch&& raw_sketch);

    private:
        std::vector<Layer> layers_;
        //Substrate substrate_;   // Подложка необходимая для управления и корректировки эскиза (пока не используется)
        float width_;
        float height_;
    };
    
}  // namespace sketch

namespace domain {

    bool IsUpperPolyline(const Polyline& input, Raw_sketch raw_sketch);

    // Перемещает "сырой" эскиз в начало координат (0,0), возвращает его габариты widht, height
    std::pair<float, float> MoveRawSketchToZeroAndGetDimensions(domain::Raw_sketch& raw_sketch);

    std::vector<sketch::Layer> ConvertRawSketch(Raw_sketch raw_sketch);


} // namespace domain
