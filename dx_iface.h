
#ifndef DX_IFACE_H
#define DX_IFACE_H

#include "drw_interface.h"
#include "libdxfrw.h"
#include "dx_data.h"

namespace dx {

    class Iface : public DRW_Interface {
    public:
        Iface(): dxfW(nullptr), cData(nullptr), currentBlock(nullptr) {}
        ~Iface() = default;

        bool fileImport(const std::string& fileI, Data* fData);
        bool fileExport(const std::string& file, DRW::Version v, bool binary, Data* fData);
        void writeEntity(DRW_Entity* e);

        //reimplement virtual DRW_Interface functions

        //reader part, stores all in class dx_data
            //header
        void addHeader(const DRW_Header* data) {
            cData->headerC = *data;
        }

        // Таблицы
        virtual void addLType(const DRW_LType& data) {
            cData->lineTypes.push_back(data);
        }
        virtual void addLayer(const DRW_Layer& data) {
            cData->layers.push_back(data);
        }
        virtual void addDimStyle(const DRW_Dimstyle& data) {
            cData->dimStyles.push_back(data);
        }
        virtual void addVport(const DRW_Vport& data) {
            cData->VPorts.push_back(data);
        }
        virtual void addTextStyle(const DRW_Textstyle& data) {
            cData->textStyles.push_back(data);
        }
        virtual void addAppId(const DRW_AppId& data) {
            cData->appIds.push_back(data);
        }

        // Блоки
        virtual void addBlock(const DRW_Block& data) {
            auto bk = std::make_unique<IfaceBlock>(data);
            currentBlock = bk.get();
            cData->blocks.push_back(std::move(bk));
        }
        virtual void endBlock() {
            currentBlock = cData->mBlock.get();
        }

        virtual void setBlock(const int /*handle*/) {} // не используется

        // Сущности
        template<typename T>
        void addEntity(const T& data) {
            currentBlock->ent.push_back(std::make_unique<T>(data));
        }

        virtual void addPoint(const DRW_Point& data) { addEntity(data); }
        virtual void addLine(const DRW_Line& data) { addEntity(data); }
        virtual void addRay(const DRW_Ray& data) { addEntity(data); }
        virtual void addXline(const DRW_Xline& data) { addEntity(data); }
        virtual void addArc(const DRW_Arc& data) { addEntity(data); }
        virtual void addCircle(const DRW_Circle& data) { addEntity(data); }
        virtual void addEllipse(const DRW_Ellipse& data) { addEntity(data); }
        virtual void addLWPolyline(const DRW_LWPolyline& data) { addEntity(data); }
        virtual void addPolyline(const DRW_Polyline& data) { addEntity(data); }
        virtual void addSpline(const DRW_Spline* data) { addEntity(*data); }
        
        virtual void addKnot(const DRW_Entity& data) {}

        virtual void addInsert(const DRW_Insert& data) { addEntity(data); }
        virtual void addTrace(const DRW_Trace& data) { addEntity(data); }
        virtual void add3dFace(const DRW_3Dface& data) { addEntity(data); }
        virtual void addSolid(const DRW_Solid& data) { addEntity(data); }
        virtual void addMText(const DRW_MText& data) { addEntity(data); }
        virtual void addText(const DRW_Text& data) { addEntity(data); }
        virtual void addDimAlign(const DRW_DimAligned* data) { addEntity(*data); }
        virtual void addDimLinear(const DRW_DimLinear* data) { addEntity(*data); }
        virtual void addDimRadial(const DRW_DimRadial* data) { addEntity(*data); }
        virtual void addDimDiametric(const DRW_DimDiametric* data) { addEntity(*data); }
        virtual void addDimAngular(const DRW_DimAngular* data) { addEntity(*data); }
        virtual void addDimAngular3P(const DRW_DimAngular3p* data) { addEntity(*data); }
        virtual void addDimOrdinate(const DRW_DimOrdinate* data) { addEntity(*data); }
        virtual void addLeader(const DRW_Leader* data) { addEntity(*data); }
        virtual void addHatch(const DRW_Hatch* data) { addEntity(*data); }
        virtual void addViewport(const DRW_Viewport& data) { addEntity(data); }

        virtual void addImage(const DRW_Image* data) {
            auto img = std::make_unique<IfaceImg>(*data);
            cData->images.push_back(img.get());
            currentBlock->ent.push_back(std::move(img));
        }

        virtual void linkImage(const DRW_ImageDef* data) {
            const duint32 handle = data->handle;
            const std::string path(data->name);
            for (auto* img : cData->images) {
                if (img->ref == handle) {
                    img->path = path;
                }
            }
        }

        //writer part, send all in class dx_data to writer
        virtual void addComment(const char* /*comment*/) {}

        virtual void writeHeader(DRW_Header& data) {
            //complete copy of header vars:
            data = cData->headerC;
            //or copy one by one:
    //        for (std::map<std::string,DRW_Variant*>::iterator it=cData->headerC.vars.begin(); it != cData->headerC.vars.end(); ++it)
    //            data.vars[it->first] = new DRW_Variant( *(it->second) );
        }

        virtual void writeBlocks() {
            for (const auto& bk : cData->blocks) {
                dxfW->writeBlock(bk.get());
                for (const auto& e : bk->ent) {
                    writeEntity(e.get());
                }
            }
        }
        //only send the name, needed by the reader to prepare handles of blocks & blockRecords
        virtual void writeBlockRecords() {
            for (const auto& bk : cData->blocks) {
                dxfW->writeBlockRecord(bk->name);
            }
        }
        //write entities of model space and first paper_space
        virtual void writeEntities() {
            for (const auto& e : cData->mBlock->ent) {
                writeEntity(e.get());
            }
        }
        virtual void writeLTypes() {
            for (std::list<DRW_LType>::iterator it = cData->lineTypes.begin(); it != cData->lineTypes.end(); ++it)
                dxfW->writeLineType(&(*it));
        }
        virtual void writeLayers() {
            for (std::list<DRW_Layer>::iterator it = cData->layers.begin(); it != cData->layers.end(); ++it)
                dxfW->writeLayer(&(*it));
        }
        virtual void writeTextstyles() {
            for (std::list<DRW_Textstyle>::iterator it = cData->textStyles.begin(); it != cData->textStyles.end(); ++it)
                dxfW->writeTextstyle(&(*it));
        }
        virtual void writeVports() {
            for (std::list<DRW_Vport>::iterator it = cData->VPorts.begin(); it != cData->VPorts.end(); ++it)
                dxfW->writeVport(&(*it));
        }
        virtual void writeDimstyles() {
            for (std::list<DRW_Dimstyle>::iterator it = cData->dimStyles.begin(); it != cData->dimStyles.end(); ++it)
                dxfW->writeDimstyle(&(*it));
        }
        virtual void writeAppId() {
            for (std::list<DRW_AppId>::iterator it = cData->appIds.begin(); it != cData->appIds.end(); ++it)
                dxfW->writeAppId(&(*it));
        }

        std::unique_ptr<dxfRW> dxfW; //pointer to writer, needed to send data
        Data* cData; // class to store or read data
        IfaceBlock* currentBlock;
    };

} // namespace dx

#endif // DX_IFACE_H
