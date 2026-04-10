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

    // Krok sieciowy
    void ustawTrybPracy(ProstyUAR::TrybPracy tryb)
    {
        trybPracy_ = tryb;
        sym_.ustawTrybPracy(tryb);

        if (trybPracy_ != ProstyUAR::TrybPracy::SieciowyRegulator) {
            m_oczekujeResetuI = false;
            m_oczekujeResetuD = false;
        }
    }
    void ustawSiecioweY(double y) { sym_.ustawSiecioweY(y); }
    void ustawSiecioweW(double w) { sym_.ustawSiecioweW(w); }
    void ustawSiecioweU(double u) { sym_.ustawSiecioweU(u); }
    void naPojedynczyKrokSieciowyObiektu();

    // Konfiguracja pelna
    KonfiguracjaUAR pobierzKonfiguracje() const;
    void ustawKonfiguracje(const KonfiguracjaUAR& cfg);

    void zapiszKonfiguracjeJSON(const std::string& sciezka) const;
    void wczytajKonfiguracjeJSON(const std::string& sciezka);

    // Callback probki
    void ustawCallbackProbki(CallbackProbki cb) { onProbka_ = std::move(cb); }

public slots:
    void przetworzRamkeSieciowa(const QByteArray& ramka);

signals:
    void wyslijRamkeSieciowa(const QByteArray& ramka);
    void opoznienieWyliczone(int32_t opoznienie);
    void bladDekodowaniaRamki(const QString& powod);
    void konfiguracjaZaktualizowanaZSieci();

private:
    // Obsluga timera
    void onTimerTick();
    void wyslijPakietSterowaniaSieciowego(uint8_t dodatkoweFlagi);

    SymulacjaUAR sym_;
    CallbackProbki onProbka_;

    ProstyUAR::TrybPracy trybPracy_ = ProstyUAR::TrybPracy::Stacjonarny;
    uint64_t m_krokSieciowySymulacji = 0;
    bool m_oczekujeResetuI = false;
    bool m_oczekujeResetuD = false;

    ProbkaUAR ostatniaProbka_{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
};



