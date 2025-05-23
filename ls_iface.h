
#pragma once

#include <cassert>
#include <vector>

#include "common.h"
#include "ls_data.h"

namespace ls {  // laminate sketch

class Iface {
public:
    const static int DEFAULT_OFFSET = 1;
    const static int DEFAULT_SEG_LEN = 20;

    Iface()
        :width_(0.)
        ,height_(0.)
        ,min_distance_between_plies_(0.)
    {
    }

    double width() const noexcept { return width_; }
    double height() const noexcept { return height_; }
    bool isEmpty() const noexcept { return original_data_.isEmpty(); }

    std::vector<Layer>& sketchLayers() { return optimized_data_.getData(); }
    std::vector<Layer>& origSketchLayers() { return original_data_.getData(); }

    // Возвращает "сырой" эскиз для записи в dxf файл
    domain::RawData rawSketch() const;

    // Наполняет эскиз данными из "сырого" эскиза
    bool fillSketch(domain::RawData&& raw_sketch);

    void scaleSketch(double scale);

    void optimizeSketch(double offset, double segment_len);

private:
    Data original_data_;
    Data optimized_data_;
    double width_;
    double height_;
    double min_distance_between_plies_;
};

}  // namespace ls

namespace domain {

bool IsUpperPolyline(const Polyline& input, const RawData& raw_sketch);

} //namespace domain
