#include "ProstyUAR.h"
ProstyUAR::ProstyUAR(ModelARX& model, RegulatorPID& regulator)
    : model_(model)
    , regulator_(regulator)
    , w_(0.0)
    , y_(0.0)
    , e_(0.0)
    , u_(0.0)
    , uP_(0.0)
    , uI_(0.0)
    , uD_(0.0)
{
}


void ProstyUAR::reset()
{
    w_ = 0.0;
    y_ = 0.0;
    e_ = 0.0;
    u_ = 0.0;
    uP_ = 0.0;
    uI_ = 0.0;
    uD_ = 0.0;
}


double ProstyUAR::symuluj(double w)
{
    w_ = w;

    e_ = w_ - y_;

    u_ = regulator_.symuluj(e_);
    uP_ = regulator_.pobierzUP();
    uI_ = regulator_.pobierzUI();
    uD_ = regulator_.pobierzUD();

    y_ = model_.symuluj(u_);

    return y_;
}

