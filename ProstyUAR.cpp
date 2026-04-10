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
    siecioweY_ = 0.0;
    siecioweU_ = 0.0;
    siecioweW_ = 0.0;
}


double ProstyUAR::symuluj(double w)
{
    if (tryb_ == TrybPracy::SieciowyRegulator) {
        w_ = w;
        e_ = w_ - siecioweY_;
        u_ = regulator_.symuluj(e_);
        
        // Tlo - oryginalny ARX uruchomiony zeby plynnie dzialalo po rozlaczeniu
        model_.symuluj(u_);
        
        y_ = siecioweY_;
    } 
    else if (tryb_ == TrybPracy::SieciowyObiekt) {
        // Ignorujemy 'w', biore parametr W i U z sieci by rysowac / symulowac
        w_ = w; // w tle liczy w z generatora, po to zeby zachowac plynne przejscie po odlaczeniu
        
        double uLokalne_ = regulator_.symuluj(w_ - y_); // w tle
        
        w_ = siecioweW_; // na zewnatrz w grafice z sieci
        u_ = siecioweU_;
        
        e_ = w_ - y_; // nie musimy aktualizowac dla trybu okna ale to pomoze w odswiezniu 
        y_ = model_.symuluj(u_);
    } 
    else {
        w_ = w;
        e_ = w_ - y_;
        u_ = regulator_.symuluj(e_);
        y_ = model_.symuluj(u_);
    }

    uP_ = regulator_.pobierzUP();
    uI_ = regulator_.pobierzUI();
    uD_ = regulator_.pobierzUD();

    return y_;
}

