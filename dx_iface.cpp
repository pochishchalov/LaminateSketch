#include <iostream>
#include <algorithm>
#include <memory>
#include "dx_iface.h"
#include "libdwgr.h"
#include "libdxfrw.h"

namespace dx {

bool Iface::fileImport(const std::string& fileI, Data* fData) {
    unsigned int found = fileI.find_last_of(".");
    std::string fileExt = fileI.substr(found + 1);
    std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::toupper);
    cData = fData;
    currentBlock = cData->mBlock.get();

    if (fileExt == "DXF") {
        auto dxf = std::make_unique<dxfRW>(fileI.c_str());
        bool success = dxf->read(this, false);
        return success;
    }
    else if (fileExt == "DWG") {
        auto dwg = std::make_unique<dwgR>(fileI.c_str());
        bool success = dwg->read(this, false);
        return success;
    }

    std::cout << "File extension must be dxf or dwg" << std::endl;
    return false;
}

bool Iface::fileExport(const std::string& file, DRW::Version v, bool binary, Data* fData) {
    cData = fData;
    dxfW = std::make_unique<dxfRW>((file.c_str()));
    bool success = dxfW->write(this, v, binary);
    dxfW.release();

    return success;
}

void Iface::writeEntity(DRW_Entity* e) {

    switch (e->eType) {
    case DRW::POINT:
        dxfW->writePoint(static_cast<DRW_Point*>(e));
        break;
    case DRW::LINE:
        dxfW->writeLine(static_cast<DRW_Line*>(e));
        break;
    case DRW::CIRCLE:
        dxfW->writeCircle(static_cast<DRW_Circle*>(e));
        break;
    case DRW::ARC:
        dxfW->writeArc(static_cast<DRW_Arc*>(e));
        break;
    case DRW::SOLID:
        dxfW->writeSolid(static_cast<DRW_Solid*>(e));
        break;
    case DRW::ELLIPSE:
        dxfW->writeEllipse(static_cast<DRW_Ellipse*>(e));
        break;
    case DRW::LWPOLYLINE:
        dxfW->writeLWPolyline(static_cast<DRW_LWPolyline*>(e));
        break;
    case DRW::POLYLINE:
        dxfW->writePolyline(static_cast<DRW_Polyline*>(e));
        break;
    case DRW::SPLINE:
        dxfW->writeSpline(static_cast<DRW_Spline*>(e));
        break;
    case DRW::INSERT:
        dxfW->writeInsert(static_cast<DRW_Insert*>(e));
        break;
    case DRW::MTEXT:
        dxfW->writeMText(static_cast<DRW_MText*>(e));
        break;
    case DRW::TEXT:
        dxfW->writeText(static_cast<DRW_Text*>(e));
        break;
    case DRW::DIMLINEAR:
    case DRW::DIMALIGNED:
    case DRW::DIMANGULAR:
    case DRW::DIMANGULAR3P:
    case DRW::DIMRADIAL:
    case DRW::DIMDIAMETRIC:
    case DRW::DIMORDINATE:
        dxfW->writeDimension(static_cast<DRW_Dimension*>(e));
        break;
    case DRW::LEADER:
        dxfW->writeLeader(static_cast<DRW_Leader*>(e));
        break;
    case DRW::HATCH:
        dxfW->writeHatch(static_cast<DRW_Hatch*>(e));
        break;
    case DRW::IMAGE:
        dxfW->writeImage(static_cast<DRW_Image*>(e), static_cast<IfaceImg*>(e)->path);
        break;
    default:
        break;
    }
}
} // namespace dx
