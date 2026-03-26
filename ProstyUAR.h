#pragma once
#include "ModelARX.h"
#include "RegulatorPID.h"

class ProstyUAR
{
public:
    ProstyUAR(ModelARX& model, RegulatorPID& regulator);

    // Symulacja
    double symuluj(double w);

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

    // Stan
    double w_;
    double y_;
    double e_;
    double u_;

    double uP_;
    double uI_;
    double uD_;
};

