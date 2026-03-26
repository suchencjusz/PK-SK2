#pragma once
#include <cstddef>

class GeneratorWartosciZadanej
{
public:
    // Typ sygnalu
    enum class Typ
    {
        Sinus,
        Prostokat
    };

    GeneratorWartosciZadanej();

    // Konfiguracja
    void ustawTyp(Typ typ);
    void ustawOkresRzeczywisty(double TRZ);
    void ustawInterwalSymulacjiMs(int TTms);
    void ustawAmplituda(double A);
    void ustawSkladowaStala(double S);
    void ustawWypelnienie(double p);

    // Generowanie
    double generuj(std::size_t i) const;

    // Gettery
    Typ pobierzTyp() const { return typ_; }
    double pobierzOkresRzeczywisty() const { return TRZ_; }
    int pobierzInterwalSymulacjiMs() const { return TTms_; }
    double pobierzAmplituda() const { return A_; }
    double pobierzSkladowaStala() const { return S_; }
    double pobierzWypelnienie() const { return p_; }

private:
    // Przeliczenia
    void przeliczOkresDyskretny();

    // Parametry
    Typ typ_;
    double TRZ_;
    int TTms_;
    int T_;
    double A_;
    double S_;
    double p_;
};

