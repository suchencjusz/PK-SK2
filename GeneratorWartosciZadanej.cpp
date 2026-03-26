#include "GeneratorWartosciZadanej.h"
#include <cmath>

namespace
{
    constexpr double PI = 3.14159265358979323846;
}

GeneratorWartosciZadanej::GeneratorWartosciZadanej()
    : typ_(Typ::Sinus)
    , TRZ_(1.0)
    , TTms_(200)
    , T_(1)
    , A_(1.0)
    , S_(0.0)
    , p_(0.5)
{
    przeliczOkresDyskretny();
}

// Konfiguracja
void GeneratorWartosciZadanej::ustawTyp(Typ typ)
{
    typ_ = typ;
}

void GeneratorWartosciZadanej::ustawOkresRzeczywisty(double TRZ)
{
    if (TRZ <= 0.0)
        TRZ_ = 1.0;
    else
        TRZ_ = TRZ;

    przeliczOkresDyskretny();
}

void GeneratorWartosciZadanej::ustawInterwalSymulacjiMs(int TTms)
{
    if (TTms <= 0)
        TTms_ = 200;
    else
        TTms_ = TTms;

    przeliczOkresDyskretny();
}

void GeneratorWartosciZadanej::ustawAmplituda(double A)
{
    A_ = A;
}

void GeneratorWartosciZadanej::ustawSkladowaStala(double S)
{
    S_ = S;
}

void GeneratorWartosciZadanej::ustawWypelnienie(double p)
{
    if (p < 0.0)      p_ = 0.0;
    else if (p > 1.0) p_ = 1.0;
    else              p_ = p;
}

// Przeliczenia
void GeneratorWartosciZadanej::przeliczOkresDyskretny()
{
    if (TTms_ <= 0 || TRZ_ <= 0.0)
    {
        T_ = 1;
        return;
    }

    const double fT = 1000.0 / static_cast<double>(TTms_);
    double Treal = TRZ_ * fT;

    if (Treal < 1.0)
        Treal = 1.0;

    T_ = static_cast<int>(std::lround(Treal));
    if (T_ < 1)
        T_ = 1;
}

// Generowanie
double GeneratorWartosciZadanej::generuj(std::size_t i) const
{
    if (T_ <= 0)
        return S_;

    const int T = T_;
    const int idx = static_cast<int>(i % static_cast<std::size_t>(T));

    switch (typ_)
    {
    case Typ::Sinus:
    {
        const double arg = static_cast<double>(idx) * PI / static_cast<double>(T);
        const double w = A_ * std::sin(arg) + S_;
        return w;
    }

    case Typ::Prostokat:
    default:
    {
        const double prog = p_ * static_cast<double>(T);
        if (static_cast<double>(idx) < prog)
            return A_ + S_;
        else
            return S_;
    }
    }
}

