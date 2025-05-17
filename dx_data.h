
#ifndef DX_DATA_H
#define DX_DATA_H
#include "libdxfrw.h"

#include <memory>

namespace dx {

    //class to store image data and path from DRW_ImageDef
    class IfaceImg : public DRW_Image {
    public:
        IfaceImg() = default;
        IfaceImg(const DRW_Image& p) :DRW_Image(p) {}
        std::string path; //stores the image path
    };

    //container class to store entites.
    class IfaceBlock : public DRW_Block {
    public:
        IfaceBlock() = default;
        IfaceBlock(const DRW_Block& p) :DRW_Block(p) {}
        std::list<std::unique_ptr<DRW_Entity>>ent; //stores the entities list
    };


    //container class to store full dwg/dxf data.
    class Data {
    public:
        Data() : mBlock(std::make_unique<IfaceBlock>()) {}

        DRW_Header headerC;                 //stores a copy of the header vars
        std::list<DRW_LType>lineTypes;      //stores a copy of all line types
        std::list<DRW_Layer>layers;         //stores a copy of all layers
        std::list<DRW_Dimstyle>dimStyles;   //stores a copy of all dimension styles
        std::list<DRW_Vport>VPorts;         //stores a copy of all vports
        std::list<DRW_Textstyle>textStyles; //stores a copy of all text styles
        std::list<DRW_AppId>appIds;         //stores a copy of all line types
        std::list<std::unique_ptr<IfaceBlock>>blocks;    //stores a copy of all blocks and the entities in it
        std::list<IfaceImg*>images;      //temporary list to find images for link with DRW_ImageDef. Do not delete it!!

        std::unique_ptr<IfaceBlock> mBlock;              //container to store model entities

        size_t GetBlocksSize() { return blocks.size(); }
        size_t GetMBlocksSize() { return mBlock->ent.size(); }

    };

} // namespace dx

#endif // DX_DATA_H
