#pragma once
#include "ModelARX.h"
#include "RegulatorPID.h"

class ProstyUAR
{
public:
    enum class TrybPracy {
        Stacjonarny,
        SieciowyRegulator,
        SieciowyObiekt
    };

    ProstyUAR(ModelARX& model, RegulatorPID& regulator);

    // Symulacja
    double symuluj(double w);

    // Ustawienia sieci
    void ustawTrybPracy(TrybPracy tryb) { tryb_ = tryb; }
    void ustawSiecioweY(double y) { siecioweY_ = y; }
    void ustawSiecioweU(double u) { siecioweU_ = u; }
    void ustawSiecioweW(double w) { siecioweW_ = w; }

    // Reset
    void reset();

    // Gettery
    double pobierzW()  const { return w_; }
    double pobierzY()  const { return y_; }
    double pobierzE()  const { return e_; }
    double pobierzU()  const { return u_; }
    double pobierzUP() const { return uP_; }
    double pobierzUI() const { return uI_; }
    double pobierzUD() const { return uD_; }

private:
    // Referencje do obiektow
    ModelARX& model_;
    RegulatorPID& regulator_;

    TrybPracy tryb_ = TrybPracy::Stacjonarny;
    double siecioweY_ = 0.0;
    double siecioweU_ = 0.0;
    double siecioweW_ = 0.0;

    // Stan
    double w_;
    double y_;
    double e_;
    double u_;

    double uP_;
    double uI_;
    double uD_;
};

