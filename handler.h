#pragma once

#include "common.h"
#include "dx_iface.h"

namespace dxf {

class DxfHandler {
public:
    DxfHandler() = default;
    bool ImportFile(std::string file_name);
    bool ExportFile(std::string file_name, DRW::Version version, bool is_binary);

    domain::RawData GetRawSketchFromData() const;
    void PutRawSketchToData(const domain::RawData raw_sketch);

private:
    dx_data input_data_;
    dx_data output_data_;
};

} // namespace dxf
