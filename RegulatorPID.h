#pragma once

class RegulatorPID
{
public:
    // Tryb calkowania
    enum class LiczCalk
    {
        Zewnetrzne,
        Wewnetrzne
    };

    explicit RegulatorPID(double k);
    RegulatorPID(double k, double Ti);
    RegulatorPID(double k, double Ti, double Td);

    // Symulacja
    double symuluj(double e);

    // Konfiguracja
    void setWzmocnienie(double k);
    void setStalaCalk(double Ti);
    void setStalaRoznicz(double Td);
    void setLiczCalk(LiczCalk tryb);

    // Reset
    void reset();
    void resetCalkowania();
    void resetRozniczkowania();

    // Gettery
    double pobierzWzmocnienie() const { return kP_; }
    double pobierzStalaCalk() const { return Ti_; }
    double pobierzStalaRoznicz() const { return Td_; }
    LiczCalk pobierzTrybCalk() const { return trybCalk_; }

    double pobierzUP() const { return uP_; }
    double pobierzUI() const { return uI_; }
    double pobierzUD() const { return uD_; }

private:
    // Nastawy
    double kP_;
    double Ti_;
    double Td_;

    // Skladowe
    double uP_;
    double uI_;
    double uD_;

    // Pamiec
    double ePrev_;
    double sumE_;

    LiczCalk trybCalk_;
};

