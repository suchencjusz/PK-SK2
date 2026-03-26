#pragma once
#include <functional>
#include <string>
#include <vector>
#include <QObject>
#include "SymulacjaUAR.h"
#include "KonfiguracjaUAR.h"

class UslugiUAR : public QObject
{
    Q_OBJECT

public:
    // Typy
    using CallbackProbki = std::function<void(const ProbkaUAR&)>;

    explicit UslugiUAR(QObject* parent = nullptr);

    // Sterowanie symulacja
    void start();
    void stop();
    void reset();

    bool czyUruchomiona() const;

    // Konfiguracja czasu
    void ustawInterwalSymulacjiMs(int TTms);
    int  pobierzInterwalSymulacjiMs() const;

    void ustawOknoObserwacjiS(int oknoS);
    int  pobierzOknoObserwacjiS() const;

    // Krok symulacji
    ProbkaUAR wykonajKrok();

    // Parametry modelu
    void ustawModelARX(const std::vector<double>& A,
        const std::vector<double>& B,
        int k,
        double sigma);


    void ustawLimityModelu(double uMin, double uMax, double yMin, double yMax);
    void ustawOgraniczanieSterowaniaModelu(bool wlacz);
    void ustawOgraniczanieWyjsciaModelu(bool wlacz);

    // Parametry PID
    void ustawPID(double kP, double Ti, double Td);
    void ustawTrybCalkowaniaPID(RegulatorPID::LiczCalk tryb);

    void resetCalkowaniaPID();
    void resetRozniczkowaniaPID();

    // Parametry generatora
    void ustawTypGeneratora(GeneratorWartosciZadanej::Typ typ);
    void ustawOkresRzeczywistyGeneratora(double TRZ);
    void ustawAmplitudaGeneratora(double A);
    void ustawSkladowaStalaGeneratora(double S);
    void ustawWypelnienieGeneratora(double p);

    // Konfiguracja pelna
    KonfiguracjaUAR pobierzKonfiguracje() const;
    void ustawKonfiguracje(const KonfiguracjaUAR& cfg);

    void zapiszKonfiguracjeJSON(const std::string& sciezka) const;
    void wczytajKonfiguracjeJSON(const std::string& sciezka);

    // Callback probki
    void ustawCallbackProbki(CallbackProbki cb) { onProbka_ = std::move(cb); }

private:
    // Obsluga timera
    void onTimerTick();

    SymulacjaUAR sym_;
    CallbackProbki onProbka_;

    ProbkaUAR ostatniaProbka_{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
};



