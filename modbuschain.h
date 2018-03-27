#ifndef MODBUSCHAIN_H
#define MODBUSCHAIN_H

#include <QObject>
#include "modbuserr.h"
#include <QList>
#include <QDebug>
#include <functional>


class ModbusChain : public QObject
{
    Q_OBJECT
public:

    enum State {
        Idle = 0,
        Executing,
        Done,
        Canceled,
        Error
    };

    typedef std::function <bool(void)> ExecFunc;

    template <typename Obj>
    using SuccFunc = void (Obj::*)();

    template <typename Obj>
    using FailFunc = void (Obj::*)(ModbusErr error);

    typedef SuccFunc<ModbusChain> ChainSuccSlot;
    typedef FailFunc<ModbusChain> ChainFailSlot;

    explicit ModbusChain(QObject *parent = 0);
    virtual ~ModbusChain();

    State state() const;

    bool clear();
    bool empty() const;
    int currentIndex() const;

    bool isDone() const;
    bool isExecuting() const;

    template <typename Obj, typename Exec>
    void append(Obj* object, SuccFunc<Obj> succ, FailFunc<Obj> fail, Exec exec);

signals:
    void success();
    void fail(ModbusErr error);
    void canceled();

public slots:
    bool exec();
    bool cancel();

private slots:
    void chainItemSucc();
    void chainItemFail(ModbusErr error);

private:
    void chainNext();

    class ChainItemSignals;

    struct ChainItem {

        template <typename Obj>
        ChainItem(Obj* o, SuccFunc<Obj> succ, FailFunc<Obj> fail, ExecFunc e);
        ChainItem(const ChainItem& item);
        ~ChainItem();

        bool exec();
        void connectSignals(ModbusChain* chain, ChainSuccSlot succ, ChainFailSlot fail);
        void disconnectSignals(ModbusChain* chain);

        ChainItemSignals* item_signals;
        ExecFunc exec_proc;
    };

    struct ChainItemSignals {

        ChainItemSignals(){}
        virtual ~ChainItemSignals(){}

        virtual ChainItemSignals* copy() const = 0;

        virtual void connectSignals(ModbusChain* chain, ChainSuccSlot succ, ChainFailSlot fail) = 0;
        virtual void disconnectSignals(ModbusChain* chain) = 0;
    };

    template <typename Obj>
    struct ChainItemSignalsTempl : public ChainItemSignals {

        ChainItemSignalsTempl(Obj* obj, SuccFunc<Obj> succ, FailFunc<Obj> fail);
        ~ChainItemSignalsTempl();

        ChainItemSignalsTempl* copy() const;

        void connectSignals(ModbusChain* chain, ChainSuccSlot succ, ChainFailSlot fail);
        void disconnectSignals(ModbusChain* chain);

        Obj* object;
        SuccFunc<Obj> succ_signal;
        FailFunc<Obj> fail_signal;
    };

    typedef QList<ChainItem> ChainList;

    bool need_cancel;
    State chain_state;
    ChainList* chain_list;
    int chain_index;
};


template <typename Obj, typename Exec>
void ModbusChain::append(Obj* object, SuccFunc<Obj> succ, FailFunc<Obj> fail, Exec exec)
{
    chain_list->append(ChainItem(object, succ, fail, exec));
}

template <typename Obj>
ModbusChain::ChainItem::ChainItem(Obj* o, SuccFunc<Obj> succ, FailFunc<Obj> fail, ExecFunc e)
{
    item_signals = new ChainItemSignalsTempl<Obj>(o, succ, fail);
    exec_proc = e;
}

template <typename Obj>
ModbusChain::ChainItemSignalsTempl<Obj>::ChainItemSignalsTempl(Obj* obj, SuccFunc<Obj> succ, FailFunc<Obj> fail)
    :ChainItemSignals()
{
    object = obj;
    succ_signal = succ;
    fail_signal = fail;
}

template <typename Obj>
ModbusChain::ChainItemSignalsTempl<Obj>::~ChainItemSignalsTempl()
{
}

template <typename Obj>
ModbusChain::ChainItemSignalsTempl<Obj>* ModbusChain::ChainItemSignalsTempl<Obj>::copy() const
{
    return new ChainItemSignalsTempl<Obj>(object, succ_signal, fail_signal);
}

template <typename Obj>
void ModbusChain::ChainItemSignalsTempl<Obj>::connectSignals(ModbusChain* chain, ChainSuccSlot succ, ChainFailSlot fail)
{
    QObject::connect(object, succ_signal, chain, succ);
    QObject::connect(object, fail_signal, chain, fail);
}

template <typename Obj>
void ModbusChain::ChainItemSignalsTempl<Obj>::disconnectSignals(ModbusChain* chain)
{
    QObject::disconnect(object, succ_signal, chain, nullptr);
    QObject::disconnect(object, fail_signal, chain, nullptr);
}

#endif // MODBUSCHAIN_H
