
#pragma once

#include <cassert>
#include <vector>

#include "common.h"
#include "ls_data.h"

namespace ls {  // laminate sketch

class Interface {
public:
    const static int DefaultOffset = 1;
    const static int DefaultSegLen = 5;

    Interface()
        :width_(0.)
        ,height_(0.)
        ,minDistanceBetweenPlies_(0.)
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
    LaminateData original_data_;
    LaminateData optimized_data_;
    double width_;
    double height_;
    double minDistanceBetweenPlies_;
};

}  // namespace ls
