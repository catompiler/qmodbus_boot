#ifndef MODBUSFILE_H
#define MODBUSFILE_H

#include "modbusobj.h"
#include "modbuserr.h"
#include <stdint.h>
#include <QByteArray>
#include <QVector>
#include <QQueue>
#include <QModbusRequest>


class ModbusFileRegion;
class QModbusResponse;


class ModbusFile : public ModbusObj
{
    Q_OBJECT
public:
    explicit ModbusFile(QObject *parent = 0);
    ModbusFile(ModbusDev* dev, uint16_t fileNum, QObject *parent = 0);
    ~ModbusFile();

    uint16_t fileNumber() const;
    void setFileNumber(uint16_t fileNum);

    bool readRegion(ModbusFileRegion* fileRgn);
    bool writeRegion(ModbusFileRegion* fileRgn);

signals:
    void regionReaded(ModbusFileRegion* fileRgn);
    void regionWrited(ModbusFileRegion* fileRgn);
    void errorOccured(ModbusFileRegion* fileRgn, ModbusErr error);

private slots:
    void regionOpMsgSended();
    void regionOpMsgError(ModbusErr error);

private:

    class RegionOp {
    public:

        enum Type {
            Read = 0,
            Write = 1
        };

        RegionOp(ModbusDev* d, ModbusFileRegion* r, RegionOp::Type t);
        ~RegionOp();

        ModbusFileRegion* region();
        RegionOp::Type type() const;

        bool done() const;

        bool iterStoreData(const QModbusResponse& resp);
        void iterNext();

        QModbusRequest modbusRequest() const;

    private:
        ModbusDev* modbus_dev;
        ModbusFileRegion* file_region;
        RegionOp::Type op_type;
        uint16_t cur_index;
        uint16_t cur_count;

        QModbusRequest readModbusRequset() const;
        QModbusRequest writeModbusRequset() const;

        uint16_t remainCount() const;
        uint16_t maxCount() const;
    };

    uint16_t file_number;

    typedef QQueue<RegionOp> RgnsQueue;
    RgnsQueue* rgns_queue;

    bool doNextRegionOp();
    bool processRegionOp();

    void regionOpSuccess(RegionOp& op);
    void regionOpFail(RegionOp& op, ModbusErr error);
};

class ModbusFileRegion : public QObject
{
    Q_OBJECT
public:

    friend class ModbusFile;

    ModbusFileRegion(QObject* parent = 0);
    ModbusFileRegion(ModbusFile* mbFile, QObject *parent = 0);
    ModbusFileRegion(ModbusFile* mbFile, uint16_t recNum, uint16_t recsCount, QObject *parent = 0);
    ~ModbusFileRegion();

    ModbusFile* modbusFile();
    void setModbusFile(ModbusFile* mbFile);

    uint16_t recordNumber() const;
    void setRecordNumber(uint16_t recNum);

    uint16_t recordsCount() const;
    void setRecordsCount(uint16_t recsCount);

    QByteArray data() const;
    void setData(const QByteArray& data);

    const QVector<uint16_t>& records() const;
    QVector<uint16_t>& records();
    void setRecords(const QVector<uint16_t>& recs);

    bool read();
    bool write();

signals:
    void dataReaded();
    void dataWrited();
    void errorOccured(ModbusErr error);

protected:
    void file_read_region();
    void file_write_region();
    void file_error_occured(ModbusErr error);

private:
    ModbusFile* modbus_file;
    uint16_t record_number;

    QVector<uint16_t> record_data;
};


#endif // MODBUSFILE_H
