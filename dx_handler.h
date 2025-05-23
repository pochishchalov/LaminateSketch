#pragma once

#include "common.h"
#include "dx_iface.h"

namespace dx {

class Handler {
public:
    Handler() = default;
    bool importFile(std::string file_name);
    bool exportFile(std::string file_name, DRW::Version version, bool is_binary);

    domain::RawData getRawSketch() const;
    void putRawSketch(const domain::RawData raw_sketch);

private:
    Data inputData;
    Data outputData;
};

} // namespace dxf
