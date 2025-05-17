#pragma once

#include "common.h"
#include "dx_iface.h"

namespace dx {

class Handler {
public:
    Handler() = default;
    bool ImportFile(std::string file_name);
    bool ExportFile(std::string file_name, DRW::Version version, bool is_binary);

    domain::RawData GetRawSketchFromData() const;
    void PutRawSketchToData(const domain::RawData raw_sketch);

private:
    Data input_data_;
    Data output_data_;
};

} // namespace dxf
