#pragma once
#include <cstddef>
#include <string>
#include <QObject>
#include <QTimer>
#include "ModelARX.h"
#include "RegulatorPID.h"
#include "ProstyUAR.h"
#include "GeneratorWartosciZadanej.h"
#include "KonfiguracjaUAR.h"

struct ProbkaUAR
{
    double t;

    double w;
    double y;
    double e;
    double u;

    double uP;
    double uI;
    double uD;
};

class SymulacjaUAR : public QObject
{
    Q_OBJECT

public:
    explicit SymulacjaUAR(QObject* parent = nullptr);

    // Czas symulacji
    void ustawInterwalSymulacjiMs(int TTms);
    int pobierzInterwalSymulacjiMs() const { return TTms_; }

    void ustawOknoObserwacjiS(int oknoS);
    int pobierzOknoObserwacjiS() const { return oknoObserwacjiS_; }

    // Sterowanie
    void start();
    void stop();
    bool czyUruchomiona() const { return uruchomiona_; }

    // Reset / krok
    void reset();
    ProbkaUAR wykonajKrok();

    // Dostep do obiektow
    ModelARX& model() { return model_; }
    const ModelARX& model() const { return model_; }

    RegulatorPID& regulator() { return regulator_; }
    const RegulatorPID& regulator() const { return regulator_; }

    GeneratorWartosciZadanej& generator() { return generator_; }
    const GeneratorWartosciZadanej& generator() const { return generator_; }

    ProstyUAR& uar() { return uar_; }
    const ProstyUAR& uar() const { return uar_; }

    // Konfiguracja
    KonfiguracjaUAR pobierzKonfiguracje() const;
    void ustawKonfiguracje(const KonfiguracjaUAR& cfg);
    void zapiszKonfiguracjeJSON(const std::string& sciezka) const;
    void wczytajKonfiguracjeJSON(const std::string& sciezka);
    // Sieć
    void ustawTrybPracy(ProstyUAR::TrybPracy tryb);
    void ustawSiecioweY(double y) { uar_.ustawSiecioweY(y); }
    void ustawSiecioweU(double u) { uar_.ustawSiecioweU(u); }
    void ustawSiecioweW(double w) { uar_.ustawSiecioweW(w); }
signals:
    void tick();

private:
    ProstyUAR::TrybPracy tryb_ = ProstyUAR::TrybPracy::Stacjonarny;
    // Obiekty symulacji
    ModelARX                  model_;
    RegulatorPID              regulator_;
    ProstyUAR                 uar_;
    GeneratorWartosciZadanej  generator_;

    // Stan czasu
    int TTms_;
    int oknoObserwacjiS_;
    std::size_t i_;
    double t_;
    bool uruchomiona_;

    // Ostatnia probka
    ProbkaUAR ostatniaProbka_;
    QTimer* timer_ = nullptr;
};

