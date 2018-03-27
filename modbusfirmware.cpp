#include "modbusfirmware.h"
#include "modbusreg.h"
#include "modbusfile.h"
#include "modbuschain.h"

// Регистры ввода.
//! Базовый адрес регистров ввода.
#define BOOT_MODBUS_INPUT_REG_BASE 0x1
//! Регистр с размером FLASH-памяти.
#define BOOT_MODBUS_INPUT_REG_FLASH_SIZE (BOOT_MODBUS_INPUT_REG_BASE + 0)
//! Регистр с размером страницы FLASH-памяти.
#define BOOT_MODBUS_INPUT_REG_FLASH_PAGE_SIZE (BOOT_MODBUS_INPUT_REG_BASE + 1)
// Регистры хранения.
//! Базовый адрес регистров хранения.
#define BOOT_MODBUS_HOLD_REG_BASE 0x1
//! Регистр номера страницы.
#define BOOT_MODBUS_HOLD_REG_PAGE_NUMBER (BOOT_MODBUS_HOLD_REG_BASE + 0)
// Флаги.
//! Базовый адрес флагов.
#define BOOT_MODBUS_COIL_BASE 0x1
//! Флаг стирания страницы по адресу в регистре BOOT_MODBUS_HOLD_REG_PAGE_ADDRESS.
#define BOOT_MODBUS_COIL_PAGE_ERASE (BOOT_MODBUS_COIL_BASE + 0)
// Файлы.
//! Базовый адрес файлов.
#define BOOT_MODBUS_FILE_BASE 0x1
//! Файл текущей страницы памяти.
#define BOOT_MODBUS_FILE_PAGE (BOOT_MODBUS_FILE_BASE + 0)


ModbusFirmware::ModbusFirmware(QObject *parent) : ModbusObj(parent)
{
    conf_chain = nullptr;
    reg_flash_size = nullptr;
    reg_page_size = nullptr;
    reg_page_num = nullptr;
    reg_page_erase = nullptr;
    file_page = nullptr;
    file_rgn_page = nullptr;
    iter_chain = nullptr;

    op_iter.setModbusFirmware(this);
}

ModbusFirmware::ModbusFirmware(ModbusDev* dev, QObject* parent) : ModbusObj(dev, parent)
{
    conf_chain = nullptr;
    reg_flash_size = nullptr;
    reg_page_size = nullptr;
    reg_page_num = nullptr;
    reg_page_erase = nullptr;
    file_page = nullptr;
    file_rgn_page = nullptr;
    iter_chain = nullptr;

    op_iter.setModbusFirmware(this);
}

ModbusFirmware::~ModbusFirmware()
{
    if(conf_chain) delete conf_chain;
    if(reg_flash_size) delete reg_flash_size;
    if(reg_page_size) delete reg_page_size;
    if(reg_page_num) delete reg_page_num;
    if(reg_page_erase) delete reg_page_erase;
    if(file_page) delete file_page;
    if(file_rgn_page) delete file_rgn_page;
    if(iter_chain) delete iter_chain;
}

bool ModbusFirmware::isConfReaded() const
{
    return conf_chain && conf_chain->isDone();
}

bool ModbusFirmware::isExecuting() const
{
    return iter_chain && iter_chain->isExecuting();
}

quint32 ModbusFirmware::flashSize() const
{
    if(!reg_flash_size) return 0;
    return reg_flash_size->value();
}

quint32 ModbusFirmware::pageSize() const
{
    if(!reg_page_size) return 0;
    return reg_page_size->value();
}

quint32 ModbusFirmware::pagesCount() const
{
    quint32 flash_size = flashSize();
    quint32 page_size = pageSize();

    if(flash_size == 0 || page_size == 0) return 0;

    return flash_size * 1024 / page_size;
}

quint32 ModbusFirmware::pageNumber(quint32 addr) const
{
    quint32 pg_size = pageSize();
    if(!pg_size) return 0;

    if(addr >= flashBase()) addr -= flashBase();

    return addr / pg_size;
}

quint32 ModbusFirmware::pageAddress(quint32 pg_num) const
{
    return flashBase() + pg_num * pageSize();
}

quint32 ModbusFirmware::pageAlignedAddress(quint32 addr) const
{
    return addr & ~(pageSize() - 1);
}

quint32 ModbusFirmware::dataSize() const
{
    return op_iter.size;
}

quint32 ModbusFirmware::dataAddress() const
{
    return op_iter.address;
}

const QByteArray& ModbusFirmware::data() const
{
    return op_iter.buffer;
}

void ModbusFirmware::setData(const QByteArray& ba)
{
    op_iter.buffer = ba;
}

bool ModbusFirmware::readData(quint32 address, quint32 size)
{
    if(!modbusDev() || !modbusDev()->isValid()) return false;
    if(iter_chain && iter_chain->isExecuting()) return false;
    if(op_iter.running) return false;

    createReadOpObjects();

    if(!op_type != Read || iter_chain->empty()){

        iter_chain->clear();

        iter_chain->append(reg_page_num, &ModbusReg::dataWrited, &ModbusReg::errorOccured, [this]{
            return reg_page_num->write();
        });

        /*read_chain->append(reg_page_erase, &ModbusReg::dataWrited, &ModbusReg::errorOccured, [this]{
            return reg_page_erase->write();
        });*/

        iter_chain->append(file_rgn_page, &ModbusFileRegion::dataReaded, &ModbusFileRegion::errorOccured, [this]{
            return file_rgn_page->read();
        });
    }

    op_type = Read;
    op_iter.begin(address, size);

    emit progressSetMin(0);
    emit progressSetMax(op_iter.size);
    emit progressChanged(op_iter.cur_size);

    iterChainNext();

    return true;
}

bool ModbusFirmware::writeData(quint32 address, const QByteArray& ba)
{
    if(!modbusDev() || !modbusDev()->isValid()) return false;
    if(iter_chain && iter_chain->isExecuting()) return false;
    if(op_iter.running) return false;

    createWriteOpObjects();

    if(op_type != Write || iter_chain->empty()){

        iter_chain->clear();

        iter_chain->append(reg_page_num, &ModbusReg::dataWrited, &ModbusReg::errorOccured, [this]{
            return reg_page_num->write();
        });

        iter_chain->append(reg_page_erase, &ModbusReg::dataWrited, &ModbusReg::errorOccured, [this]{
            return reg_page_erase->write();
        });

        iter_chain->append(file_rgn_page, &ModbusFileRegion::dataWrited, &ModbusFileRegion::errorOccured, [this]{
            return file_rgn_page->write();
        });
    }

    op_type = Write;
    op_iter.begin(address, ba.size());
    op_iter.buffer = ba;

    emit progressSetMin(0);
    emit progressSetMax(op_iter.size);
    emit progressChanged(op_iter.cur_size);

    iterChainNext();

    return true;
}

bool ModbusFirmware::cancel()
{
    if(!modbusDev() || !modbusDev()->isValid()) return false;
    if(!iter_chain) return false;
    if(!iter_chain->isExecuting()) return false;
    if(!op_iter.running) return false;

    return iter_chain->cancel();
}

void ModbusFirmware::confRead()
{
    if(!modbusDev() || !modbusDev()->isValid()) return;

    if(!reg_flash_size)
        reg_flash_size = new ModbusReg(modbusDev(), QModbusDataUnit::InputRegisters, BOOT_MODBUS_INPUT_REG_FLASH_SIZE);

    if(!reg_page_size)
        reg_page_size = new ModbusReg(modbusDev(), QModbusDataUnit::InputRegisters, BOOT_MODBUS_INPUT_REG_FLASH_PAGE_SIZE);

    if(!conf_chain){
        conf_chain = new ModbusChain();

        conf_chain->append(reg_flash_size, &ModbusReg::dataReaded, &ModbusReg::errorOccured, [this]{
            return reg_flash_size->read();
        });

        conf_chain->append(reg_page_size, &ModbusReg::dataReaded, &ModbusReg::errorOccured, [this]{
            return reg_page_size->read();
        });

        connect(conf_chain, &ModbusChain::success, this, &ModbusFirmware::confChainSuccess);
        connect(conf_chain, &ModbusChain::fail, this, &ModbusFirmware::confChainFail);

    }

    if(!conf_chain->exec()){
        confChainFail(ModbusErr(ModbusErr::General, tr("ModbusFirmware"), tr("Update chain exec fail!")));
    }
}

void ModbusFirmware::confChainSuccess()
{
    if(conf_chain == nullptr){
        qDebug() << "ModbusFirmware: updateChainSuccess update_chain == nullptr!";
        return;
    }

    emit confReaded();
}

void ModbusFirmware::confChainFail(ModbusErr error)
{
    if(conf_chain == nullptr){
        qDebug() << "ModbusFirmware: updateChainFail update_chain == nullptr!";
        return;
    }

    emit confReadErrorOccured(error);
}

void ModbusFirmware::iterChainSuccess()
{
    if(iter_chain == nullptr){
        qDebug() << "ModbusFirmware: iterChainSuccess iter_chain == nullptr!";
        return;
    }

    if(op_type == Read){
        op_iter.appendReaded(file_rgn_page->data());
    }

    op_iter.next();

    emit progressChanged(op_iter.cur_size);

    if(op_iter.done()){

        op_iter.end();

        if(op_type == Read){
            emit dataReaded();
        }else{
            emit dataWrited();
        }
    }else{
        iterChainNext();
    }
}

void ModbusFirmware::iterChainFail(ModbusErr error)
{
    if(iter_chain == nullptr){
        qDebug() << "ModbusFirmware: iterChainFail iter_chain == nullptr!";
        return;
    }

    qDebug() << "ModbusFirmware: updateChainFail chain index:" << iter_chain->currentIndex();

    op_iter.end();

    if(op_type == Read){
        emit dataReadErrorOccured(error);
    }else{
        emit dataWriteErrorOccured(error);
    }
}

void ModbusFirmware::iterChainCanceled()
{
    if(iter_chain == nullptr){
        qDebug() << "ModbusFirmware: iterChainCanceled iter_chain == nullptr!";
        return;
    }

    op_iter.end();

    if(op_type == Read){
        emit dataReadCanceled();
    }else{
        emit dataWriteCanceled();
    }
}

void ModbusFirmware::iterChainNext()
{
    //qDebug() << ((op_type == Read) ? ("--- Reading ---") : ("--- Writing ---"));
    //qDebug() << "Page" << op_iter.page;
    //qDebug() << "Rec num" << op_iter.rec_num << "Rec count" << op_iter.rec_count;
    //qDebug() << "Skip before" << op_iter.skip_before << "Skip after" << op_iter.skip_after;

    reg_page_num->setValue(op_iter.page);
    file_rgn_page->setRecordNumber(op_iter.rec_num);
    file_rgn_page->setRecordsCount(op_iter.rec_count);

    if(op_type == Write){
        file_rgn_page->setData(op_iter.dataToWrite());
    }

    if(!iter_chain->exec()){
        iterChainFail(ModbusErr(ModbusErr::General, tr("ModbusFirmware"), tr("Error executing iter chain!")));
    }
}

void ModbusFirmware::createOpObjects()
{
    if(!reg_page_num)
        reg_page_num = new ModbusReg(modbusDev(), QModbusDataUnit::HoldingRegisters, BOOT_MODBUS_HOLD_REG_PAGE_NUMBER);

    if(!file_page)
        file_page = new ModbusFile(modbusDev(), BOOT_MODBUS_FILE_PAGE);

    if(!file_rgn_page)
        file_rgn_page = new ModbusFileRegion(file_page);

    if(!iter_chain){
        iter_chain = new ModbusChain();

        connect(iter_chain, &ModbusChain::success, this, &ModbusFirmware::iterChainSuccess);
        connect(iter_chain, &ModbusChain::fail, this, &ModbusFirmware::iterChainFail);
        connect(iter_chain, &ModbusChain::canceled, this, &ModbusFirmware::iterChainCanceled);
    }
}

void ModbusFirmware::createReadOpObjects()
{
    createOpObjects();
}

void ModbusFirmware::createWriteOpObjects()
{
    createOpObjects();

    if(!reg_page_erase){
        reg_page_erase = new ModbusReg(modbusDev(), QModbusDataUnit::Coils, BOOT_MODBUS_COIL_PAGE_ERASE);
        reg_page_erase->setValue(1);
    }
}
