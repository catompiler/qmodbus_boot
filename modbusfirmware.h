#ifndef MODBUSFIRMWARE_H
#define MODBUSFIRMWARE_H

#include "modbusobj.h"
#include "modbuserr.h"
#include <QByteArray>

class ModbusReg;
class ModbusFile;
class ModbusFileRegion;
class ModbusChain;


class ModbusFirmware : public ModbusObj
{
    Q_OBJECT
public:

    explicit ModbusFirmware(QObject *parent = 0);
    ModbusFirmware(ModbusDev* dev, QObject *parent = 0);
    ~ModbusFirmware();

    bool isConfReaded() const;
    bool isExecuting() const;

    quint32 flashSize() const;
    quint32 pageSize() const;
    quint32 pagesCount() const;

    quint32 pageNumber(quint32 addr) const;
    quint32 pageAddress(quint32 pg_num) const;
    quint32 pageAlignedAddress(quint32 addr) const;

    quint32 dataSize() const;
    quint32 dataAddress() const;

    const QByteArray& data() const;
    void setData(const QByteArray& ba);

    bool readData(quint32 address, quint32 size);
    bool writeData(quint32 address, const QByteArray& ba);

    bool cancel();

    bool runApp();

    static constexpr quint32 flashBase()
        { return 0x08000000; }

signals:
    void confReaded();
    void confReadErrorOccured(ModbusErr error);

    void dataReaded();
    void dataReadErrorOccured(ModbusErr error);
    void dataReadCanceled();

    void dataWrited();
    void dataWriteErrorOccured(ModbusErr error);
    void dataWriteCanceled();

    void progressSetMin(int val);
    void progressSetMax(int val);
    void progressChanged(int val);

public slots:
    void confRead();

private slots:
    void confChainSuccess();
    void confChainFail(ModbusErr error);

    void iterChainSuccess();
    void iterChainFail(ModbusErr error);
    void iterChainCanceled();

private:
    void iterChainNext();

    void createOpObjects();
    void createReadOpObjects();
    void createWriteOpObjects();

    ModbusReg* reg_flash_size;
    ModbusReg* reg_page_size;

    ModbusReg* reg_run_app;

    ModbusReg* reg_page_num;
    ModbusReg* reg_page_erase;
    ModbusFile* file_page;
    ModbusFileRegion* file_rgn_page;



    ModbusChain* conf_chain;
    ModbusChain* iter_chain;

// DEBUG.
public:

    enum OpType {
        Read = 0,
        Write
    };

    struct IterOp{
        IterOp(){
            firmware = nullptr;

            reset();
        }

        IterOp(ModbusFirmware* mf, quint32 op_addr, quint32 op_size){

            firmware = mf;

            reset();

            begin(op_addr, op_size);
        }

        IterOp(const IterOp& op){

            buffer = op.buffer;
            running = op.running;

            firmware = op.firmware;

            address = op.address;
            size = op.size;
            cur_size = op.cur_size;

            skip_before = op.skip_before;
            skip_after = op.skip_after;

            page = op.page;
            rec_num = op.rec_num;
            rec_count = op.rec_count;
        }

        ~IterOp(){}

        void update(){
            quint32 op_addr = address + cur_size;

            page = firmware->pageNumber(op_addr);

            quint32 page_addr = firmware->pageAddress(page);
            quint32 page_addr_next = firmware->pageAddress(page + 1);

            rec_num = (op_addr - page_addr) / 2;
            skip_before = op_addr % 2;

            quint32 op_size = qMin(size - cur_size, page_addr_next - op_addr);
            quint32 op_addr_end = op_addr + op_size;
            skip_after = op_addr_end % 2;

            quint32 rec_end = (op_addr_end - page_addr + skip_after) / 2;
            rec_count = rec_end - rec_num;
        }

        void begin(quint32 op_addr, quint32 op_size){

            reset();

            address = op_addr;
            size = op_size;

            update();

            running = true;
        }

        void end(){
            running = false;
        }

        bool done(){
            return cur_size >= size;
        }

        quint32 iterSize(){
            return rec_count * 2 - skip_before - skip_after;
        }

        void next(){
            cur_size += iterSize();
            update();
        }

        void setModbusFirmware(ModbusFirmware* mf){
            firmware = mf;
        }

        void reset(){

            buffer.clear();
            running = false;

            address = 0;
            size = 0;
            cur_size = 0;

            skip_before = 0;
            skip_after = 0;
            rec_num = 0;
            rec_count = 0;

            page = 0;
            rec_num = 0;
            rec_count = 0;
            skip_before = 0;
            skip_after = 0;
        }

        void appendReaded(const QByteArray& ba){
            if(static_cast<quint32>(ba.size()) > skip_before + skip_after){
                buffer.append(ba.mid(skip_before, ba.size() - skip_before - skip_after));
            }
        }

        QByteArray dataToWrite() const{
            return buffer.mid(cur_size, rec_count * 2);
        }

        bool running;

        QByteArray buffer;

        ModbusFirmware* firmware;

        quint32 address;
        quint32 size;
        quint32 cur_size;

        quint32 skip_before;
        quint32 skip_after;

        quint32 page;
        quint32 rec_num;
        quint32 rec_count;
    };

    IterOp op_iter;
    OpType op_type;
};

#endif // MODBUSFIRMWARE_H
