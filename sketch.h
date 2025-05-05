#pragma once

#include <cassert>
#include <vector>

#include "common.h"
#include "ls_data.h"

namespace ls {  // laminate sketch


class Sketch {
    const static int DEFAULT_WIDTH = 200;
public:
    Sketch()
        :width_(0.)
        ,height_(0.)
        ,min_dist_between_plies_(0.)
    {
    }
        
    double Width() const noexcept { return width_; }
    double Height() const noexcept { return height_; }
    bool IsEmpty() const noexcept { return original_data_.IsEmpty(); }

    std::vector<Layer>& GetSketchLayers() { return optimized_data_.GetData(); }
        
    // Возвращает "сырой" эскиз для записи в dxf файл
    domain::RawData GetRawSketch() const;
        
    // Наполняет эскиз данными из "сырого" эскиза
    bool FillSketch(domain::RawData&& raw_sketch);

    void ScaleSketch(double scale);

    void OptimizeSketchForWidth(int width);

private:
    Data original_data_;
    Data optimized_data_;
    double width_;
    double height_;
    double min_dist_between_plies_;
};

}  // namespace ls
