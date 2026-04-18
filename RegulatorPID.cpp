#include "RegulatorPID.h"

RegulatorPID::RegulatorPID(double k, double Ti = 0.0, double Td = 0.0)
    : kP_(k)
    , Ti_(Ti)
    , Td_(Td)
    , uP_(0.0)
    , uI_(0.0)
    , uD_(0.0)
    , ePrev_(0.0)
    , sumE_(0.0)
    , trybCalk_(LiczCalk::Zewnetrzne)
{
}

// Konfiguracja
void RegulatorPID::setWzmocnienie(double k)
{
    kP_ = k;
}

void RegulatorPID::setStalaCalk(double Ti)
{
    Ti_ = Ti;
}

void RegulatorPID::setStalaRoznicz(double Td)
{
    Td_ = Td;
}

void RegulatorPID::setLiczCalk(LiczCalk tryb)
{
    if (tryb == trybCalk_)
        return;

    if (tryb == LiczCalk::Wewnetrzne && trybCalk_ == LiczCalk::Zewnetrzne)
    {
        if (Ti_ > 0.0)
            uI_ = sumE_ / Ti_;
        else
            uI_ = 0.0;
    }
    else if (tryb == LiczCalk::Zewnetrzne && trybCalk_ == LiczCalk::Wewnetrzne)
    {
        sumE_ = uI_ * Ti_;
    }

    trybCalk_ = tryb;
}

// Reset
void RegulatorPID::resetCalkowania()
{
    sumE_ = 0.0;
    uI_ = 0.0;
}

void RegulatorPID::resetRozniczkowania()
{
    ePrev_ = 0.0;
    uD_ = 0.0;
}

void RegulatorPID::reset()
{
    uP_ = 0.0;
    uI_ = 0.0;
    uD_ = 0.0;
    sumE_ = 0.0;
    ePrev_ = 0.0;
}

// Symulacja
double RegulatorPID::symuluj(double e)
{
    uP_ = kP_ * e;

    if (Ti_ > 0.0)
    {
        if (trybCalk_ == LiczCalk::Zewnetrzne)
        {
            sumE_ += e;
            uI_ = sumE_ / Ti_;
        }
        else
        {
            uI_ += e / Ti_;
        }
    }
    else
    {
        uI_ = 0.0;
    }

    if (Td_ > 0.0)
    {
        uD_ = Td_ * (e - ePrev_);
    }
    else
    {
        uD_ = 0.0;
    }

    ePrev_ = e;

    return uP_ + uI_ + uD_;
}

