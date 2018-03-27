#include "modbuschain.h"

ModbusChain::ModbusChain(QObject *parent) : QObject(parent)
{
    need_cancel = false;
    chain_state = Idle;
    chain_list = new ChainList();
    chain_index = 0;
}

ModbusChain::~ModbusChain()
{
    delete chain_list;
}

ModbusChain::State ModbusChain::state() const
{
    return chain_state;
}

bool ModbusChain::clear()
{
    if(chain_state == Executing) return false;

    chain_list->clear();

    return true;
}

bool ModbusChain::empty() const
{
    return chain_list->empty();
}

int ModbusChain::currentIndex() const
{
    return chain_index;
}

bool ModbusChain::isDone() const
{
    return chain_state == Done;
}

bool ModbusChain::isExecuting() const
{
    return chain_state == Executing;
}

bool ModbusChain::exec()
{
    if(chain_state == Executing) return false;
    if(chain_list->empty()) return false;

    need_cancel = false;
    chain_state = Executing;
    chain_index = 0;

    chainNext();

    return true;
}

bool ModbusChain::cancel()
{
    if(chain_state != Executing) return false;
    if(chain_list->empty()) return false;

    need_cancel = true;

    return true;
}

void ModbusChain::chainItemSucc()
{
    if(chain_list->empty()){
        qDebug() << "ModbusChain: chainItemSucc empty chain!";
        return;
    }

    if(chain_index >= chain_list->size()){
        qDebug() << "ModbusChain: chainItemSucc chain_index out of range!";
        return;
    }

    ChainItem item = (*chain_list)[chain_index];
    item.disconnectSignals(this);

    if(need_cancel){

        chain_state = Canceled;
        emit canceled();

    }else if(++ chain_index >= chain_list->size()){

        chain_state = Done;
        emit success();

    }else{
        chainNext();
    }
}

void ModbusChain::chainItemFail(ModbusErr error)
{
    if(chain_list->empty()){
        qDebug() << "ModbusChain: chainItemFail empty chain!";
        return;
    }

    if(chain_index >= chain_list->size()){
        qDebug() << "ModbusChain: chainItemFail chain_index out of range!";
        return;
    }

    chain_state = Error;

    ChainItem& item = (*chain_list)[chain_index];
    item.disconnectSignals(this);

    emit fail(error);
}

void ModbusChain::chainNext()
{
    if(chain_list->empty()){
        qDebug() << "ModbusChain: chainNext empty chain!";
        return;
    }

    if(chain_index >= chain_list->size()){
        qDebug() << "ModbusChain: chainNext chain_index out of range!";
        return;
    }

    ChainItem& item = (*chain_list)[chain_index];
    item.connectSignals(this, &ModbusChain::chainItemSucc, &ModbusChain::chainItemFail);

    if(!item.exec()){
        ModbusErr err(ModbusErr::General, tr("ModbusChain"), tr("Error executing chain item"));
        chainItemFail(err);
    }
}

ModbusChain::ChainItem::ChainItem(const ChainItem& item)
{
    exec_proc = item.exec_proc;
    item_signals = item.item_signals->copy();
}

ModbusChain::ChainItem::~ChainItem()
{
    delete item_signals;
}

bool ModbusChain::ChainItem::exec()
{
    return exec_proc();
}

void ModbusChain::ChainItem::connectSignals(ModbusChain* chain, ChainSuccSlot succ, ChainFailSlot fail)
{
    item_signals->connectSignals(chain, succ, fail);
}

void ModbusChain::ChainItem::disconnectSignals(ModbusChain* chain)
{
    item_signals->disconnectSignals(chain);
}
