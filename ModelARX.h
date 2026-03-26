#pragma once
#include <initializer_list>
#include <vector>
#include <deque>
#include <random>

class ModelARX
{
public:
    ModelARX(std::initializer_list<double> A,
        std::initializer_list<double> B,
        int k = 1,
        double sigma = 0.0);

    // Symulacja
    double symuluj(double u);

    // Konfiguracja
    void ustawLimity(double uMin, double uMax,
        double yMin, double yMax);

    void ustawSigma(double sigma);

    void ustawWspolczynnikiA(const std::vector<double>& A);
    void ustawWspolczynnikiB(const std::vector<double>& B);
    void ustawOpoznienie(int k);

    void ustawOgraniczanieU(bool wlacz);
    void ustawOgraniczanieY(bool wlacz);

    void reset();

    // Gettery
    const std::vector<double>& pobierzA() const { return A_; }
    const std::vector<double>& pobierzB() const { return B_; }
    int pobierzK() const { return k_; }
    double pobierzSigma() const { return sigma_; }

    void pobierzLimity(double& uMin, double& uMax,
        double& yMin, double& yMax) const
    {
        uMin = uMin_;
        uMax = uMax_;
        yMin = yMin_;
        yMax = yMax_;
    }

    bool czyOgraniczanieU() const { return ograniczU_; }
    bool czyOgraniczanieY() const { return ograniczY_; }

private:
    // Parametry modelu
    int    k_;
    double sigma_;

    std::vector<double> A_;
    std::vector<double> B_;

    // Limity
    double uMin_, uMax_;
    double yMin_, yMax_;

    bool ograniczU_;
    bool ograniczY_;

    // Pamiec sygnalow
    std::deque<double> uHist_;
    std::deque<double> yHist_;

    // Szum
    std::mt19937 rng_;
    std::normal_distribution<double> normal_;

    // Pomocnicze
    static double clamp(double v, double vmin, double vmax);
    double losujNormalny();
};

