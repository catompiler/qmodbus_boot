#include "modbusfile.h"
#include "modbusmsg.h"
#include <QModbusReply>
#include <QModbusResponse>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>


#define REF_TYPE 0x6


ModbusFile::ModbusFile(QObject *parent) : ModbusObj(parent)
{
    file_number = 0;
    rgns_queue = new RgnsQueue();
}

ModbusFile::ModbusFile(ModbusDev* dev, uint16_t fileNum, QObject* parent) : ModbusObj(dev, parent)
{
    file_number = fileNum;
    rgns_queue = new RgnsQueue();
}

ModbusFile::~ModbusFile()
{
    delete rgns_queue;
}

uint16_t ModbusFile::fileNumber() const
{
    return file_number;
}

void ModbusFile::setFileNumber(uint16_t fileNum)
{
    file_number = fileNum;
}

bool ModbusFile::readRegion(ModbusFileRegion* fileRgn)
{
    if(!modbusDev() || !modbusDev()->isValid()) return false;

    bool need_do = rgns_queue->empty();

    rgns_queue->append(RegionOp(modbusDev(), fileRgn, RegionOp::Read));

    if(need_do) doNextRegionOp();

    return true;
}

bool ModbusFile::writeRegion(ModbusFileRegion* fileRgn)
{
    if(!modbusDev() || !modbusDev()->isValid()) return false;

    bool need_do = rgns_queue->empty();

    rgns_queue->append(RegionOp(modbusDev(), fileRgn, RegionOp::Write));

    if(need_do) doNextRegionOp();

    return true;
}

void ModbusFile::regionOpMsgSended()
{
    ModbusMsg* msg = qobject_cast<ModbusMsg*>(sender());
    if(!msg){
        qDebug() << "ModbusFile: regionOpMsgSended msg == NULL!";
        return;
    }

    msg->deleteLater();

    if(!msg->isSended()) return;

    if(rgns_queue->empty()){
        qDebug() << "ModbusFile: regionOpMsgSended empty queue!";
        return;
    }

    RegionOp& op = rgns_queue->first();

    QModbusReply* reply = msg->reply();
    if(!reply){
        qDebug() << "ModbusFile: regionOpMsgSended reply == NULL!";

        regionOpFail(op, ModbusErr(ModbusErr::State, tr("ModbusFile"), tr("Read reply == nullptr!")));

        rgns_queue->removeFirst();
        doNextRegionOp();

        return;
    }

    if(reply->error() != QModbusDevice::NoError){

        regionOpFail(op, ModbusErr(ModbusErr::State, tr("ModbusFile"), tr("Read reply has error!")));

        rgns_queue->removeFirst();
        doNextRegionOp();

        return;
    }

    if(op.type() == RegionOp::Read){
        if(!op.iterStoreData(reply->rawResult())){

            regionOpFail(op, ModbusErr(ModbusErr::General, tr("ModbusFile"), tr("Read result invalid!")));

            rgns_queue->removeFirst();
            doNextRegionOp();

            return;
        }
    }

    if(op.done()){
        regionOpSuccess(op);

        rgns_queue->removeFirst();
    }

    doNextRegionOp();
}

void ModbusFile::regionOpMsgError(ModbusErr error)
{
    ModbusMsg* msg = qobject_cast<ModbusMsg*>(sender());
    if(!msg){
        qDebug() << "ModbusFile: regionOpMsgError msg == NULL!";
        return;
    }

    msg->deleteLater();

    if(rgns_queue->empty()){
        qDebug() << "ModbusFile: regionOpMsgError empty queue!";
        return;
    }

    RegionOp& op = rgns_queue->first();

    regionOpFail(op, error);

    rgns_queue->removeFirst();
    doNextRegionOp();
}

bool ModbusFile::doNextRegionOp()
{
    if(!modbusDev() && !modbusDev()->isValid()) return false;

    for(;;){
        if(rgns_queue->empty()) return false;

        RegionOp& op = rgns_queue->first();
        op.iterNext();

        if(processRegionOp()) break;

        regionOpFail(op, ModbusErr(ModbusErr::General, tr("ModbusFile"), tr("Error processing file operation!")));
        rgns_queue->removeFirst();
    }

    return true;
}

bool ModbusFile::processRegionOp()
{
    if(!modbusDev() && !modbusDev()->isValid()) return false;
    if(rgns_queue->empty()) return false;

    RegionOp& op = rgns_queue->first();

    QModbusRequest request = op.modbusRequest();
    if(!request.isValid()) return false;

    ModbusMsg* msg = new ModbusMsg();
    msg->setRequest(request);

    connect(msg, &ModbusMsg::sendSuccess, this, &ModbusFile::regionOpMsgSended);
    connect(msg, &ModbusMsg::sendError, this, &ModbusFile::regionOpMsgError);

    if(!modbusDev()->sendMsg(msg)){
        delete msg;
        return false;
    }

    return true;
}

void ModbusFile::regionOpSuccess(RegionOp& op)
{
    ModbusFileRegion* fileRgn = op.region();

    if(op.type() == RegionOp::Read){

        fileRgn->file_read_region();
        emit regionReaded(fileRgn);

    }else{

        fileRgn->file_write_region();
        emit regionWrited(fileRgn);
    }
}

void ModbusFile::regionOpFail(RegionOp& op, ModbusErr error)
{
    ModbusFileRegion* fileRgn = op.region();
    fileRgn->file_error_occured(error);

    emit errorOccured(fileRgn, error);
}

ModbusFile::RegionOp::RegionOp(ModbusDev* d, ModbusFileRegion* r, ModbusFile::RegionOp::Type t)
{
    modbus_dev = d;
    file_region = r;
    op_type = t;
    cur_count = 0;
    cur_index = 0;
}

ModbusFile::RegionOp::~RegionOp()
{
}

ModbusFileRegion*ModbusFile::RegionOp::region()
{
    return file_region;
}

ModbusFile::RegionOp::Type ModbusFile::RegionOp::type() const
{
    return op_type;
}

bool ModbusFile::RegionOp::done() const
{
    uint16_t next_index = cur_index + cur_count;
    return next_index >= file_region->recordsCount();
}

bool ModbusFile::RegionOp::iterStoreData(const QModbusResponse& resp)
{
    if(op_type != Read) return false;

    QDataStream ds(resp.data());
    ds.setByteOrder(QDataStream::BigEndian);

    uint8_t data_len = 0,
            resp_len = 0,
            ref_type = 0;

    ds >> data_len;
    ds >> resp_len >> ref_type;

    if(ref_type != REF_TYPE) return false;
    if(resp_len != data_len - 1) return false; // only single request;

    int resp_records_data_len = resp_len - 1;
    int reading_records_data_len = cur_count * 2;
    if(resp_records_data_len != reading_records_data_len) return false;

    uint16_t last_index = cur_index + cur_count;
    if(last_index > file_region->recordsCount()) return false;

    QVector<uint16_t>& records = file_region->records();

    for(uint16_t index = cur_index; index < last_index; index ++){
        ds >> records[index];
    }

    return true;
}

void ModbusFile::RegionOp::iterNext()
{
    cur_index += cur_count;
    cur_count = qMin(remainCount(), maxCount());
}

QModbusRequest ModbusFile::RegionOp::modbusRequest() const
{
    return (op_type == Read) ? readModbusRequset() : writeModbusRequset();
}

QModbusRequest ModbusFile::RegionOp::readModbusRequset() const
{
    ModbusFile* file = file_region->modbusFile();
    if(!file) return QModbusRequest();

    QByteArray data;

    QDataStream ds(&data, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);

    uint8_t byte_count = 7; // ref_type(1) + file_num(2) + rec_num(2) + rec_len(2).
    uint8_t ref_type = REF_TYPE;
    uint16_t file_num = file->fileNumber();
    uint16_t rec_num = cur_index;
    uint16_t rec_len = cur_count;

    ds << byte_count << ref_type;
    ds << file_num << rec_num << rec_len;

    return QModbusRequest(QModbusPdu::ReadFileRecord, data);
}

QModbusRequest ModbusFile::RegionOp::writeModbusRequset() const
{
    ModbusFile* file = file_region->modbusFile();
    if(!file) return QModbusRequest();

    QByteArray data;

    QDataStream ds(&data, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);

    uint8_t data_len = 7 /* ref_type(1) + file_num(2) + rec_num(2) + rec_len(2) */ + cur_count * sizeof(uint16_t);
    uint8_t ref_type = REF_TYPE;
    uint16_t file_num = file->fileNumber();
    uint16_t rec_num = cur_index;
    uint16_t rec_len = cur_count;

    ds << data_len << ref_type;
    ds << file_num << rec_num << rec_len;

    QVector<uint16_t>& records = file_region->records();

    uint16_t last_index = cur_index + cur_count;
    if(last_index > file_region->recordsCount()) return QModbusRequest();

    for(uint16_t index = cur_index; index < last_index; index ++){
        ds << records[index];
    }

    return QModbusRequest(QModbusPdu::WriteFileRecord, data);
}

uint16_t ModbusFile::RegionOp::remainCount() const
{
    uint16_t recs_count = file_region->recordsCount();
    if(cur_index >= recs_count) return 0;
    return recs_count - cur_index;
}

uint16_t ModbusFile::RegionOp::maxCount() const
{
    if(!modbus_dev) return 0;

    int max_pdu = modbus_dev->maxPduSize();

    if(op_type == Read){
        max_pdu -= 1 /* func */ + 1 /* data len */ + 1 /* resp len */ + 1 /* ref type */;
    }else{ // Write
        max_pdu -= 1 /* func */ + 1 /* data len */ + 1 /* ref type */ + 2 /* file num */ + 2 /* rec num */ + 2 /* rec len */;
    }

    // size in bytes -> size in records (2 byte).
    max_pdu /= 2;

    return (max_pdu >= 0) ? max_pdu : 0;
}

ModbusFileRegion::ModbusFileRegion(QObject* parent) : QObject(parent)
{
    modbus_file = nullptr;
    record_number = 0;
}

ModbusFileRegion::ModbusFileRegion(ModbusFile* mbFile, QObject* parent) : QObject(parent)
{
    modbus_file = mbFile;
    record_number = 0;
}

ModbusFileRegion::ModbusFileRegion(ModbusFile* mbFile, uint16_t recNum, uint16_t recsCount, QObject* parent) : QObject(parent)
{
    modbus_file = mbFile;
    record_number = recNum;
    record_data.resize(recsCount);
}

ModbusFileRegion::~ModbusFileRegion()
{
}

ModbusFile*ModbusFileRegion::modbusFile()
{
    return modbus_file;
}

void ModbusFileRegion::setModbusFile(ModbusFile* mbFile)
{
    modbus_file = mbFile;
}

uint16_t ModbusFileRegion::recordNumber() const
{
    return record_number;
}

void ModbusFileRegion::setRecordNumber(uint16_t recNum)
{
    record_number = recNum;
}

uint16_t ModbusFileRegion::recordsCount() const
{
    return record_data.size();
}

void ModbusFileRegion::setRecordsCount(uint16_t recsCount)
{
    record_data.resize(recsCount);
}

QByteArray ModbusFileRegion::data() const
{
    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);

    for(const quint16& data: record_data){
        ds << data;
    }

    return ba;
}

void ModbusFileRegion::setData(const QByteArray& data)
{
    // Необходимое число записей, вмещающих все данные.
    int recs_count = (data.size() + 1) / sizeof(uint16_t);
    // Число целых записей в массиве.
    int count = data.size() / sizeof(uint16_t);

    QDataStream ds(data);
    ds.setByteOrder(QDataStream::LittleEndian);

    record_data.resize(recs_count);

    // Цикл по записям в массиве.
    for(int i = 0; i < count; i ++){
        ds >> record_data[i];
    }

    // Если остался байт данных.
    if(recs_count != count){
        // Последний байт.
        uint8_t last_rec_byte = 0;
        // Считаем.
        ds >> last_rec_byte;
        // Присвоим.
        record_data[recs_count - 1] = last_rec_byte;
    }
}

const QVector<uint16_t>& ModbusFileRegion::records() const
{
    return record_data;
}

QVector<uint16_t>& ModbusFileRegion::records()
{
    return record_data;
}

void ModbusFileRegion::setRecords(const QVector<uint16_t>& recs)
{
    record_data = recs;
}

bool ModbusFileRegion::read()
{
    if(!modbus_file) return false;

    return modbus_file->readRegion(this);
}

bool ModbusFileRegion::write()
{
    if(!modbus_file) return false;

    return modbus_file->writeRegion(this);
}

void ModbusFileRegion::file_read_region()
{
    emit dataReaded();
}

void ModbusFileRegion::file_write_region()
{
    emit dataWrited();
}

void ModbusFileRegion::file_error_occured(ModbusErr error)
{
    emit errorOccured(error);
}
