
#include "ModelARX.h"
#include <algorithm>
#include <cmath>
#include <numeric>

// Pomocnicze
double ModelARX::clamp(double v, double vmin, double vmax)
{
    if (v < vmin) return vmin;
    if (v > vmax) return vmax;
    return v;
}

double ModelARX::losujNormalny()
{
    return normal_(rng_);
}

ModelARX::ModelARX(std::initializer_list<double> A,
    std::initializer_list<double> B,
    int k,
    double sigma)
    : k_(k < 1 ? 1 : k)
    , sigma_(sigma < 0.0 ? 0.0 : sigma)
    , A_(A)
    , B_(B)
    , uMin_(-10.0)
    , uMax_(10.0)
    , yMin_(-10.0)
    , yMax_(10.0)
    , ograniczU_(true)
    , ograniczY_(true)
    , rng_(12345u)
    , normal_(0.0, 1.0)
{
    const std::size_t nA = A_.size();
    const std::size_t nB = B_.size();

    uHist_.assign(static_cast<std::size_t>(k_) + nB, 0.0);
    yHist_.assign(nA, 0.0);
}

// Symulacja
double ModelARX::symuluj(double u)
{
    const double uWe = ograniczU_ ? clamp(u, uMin_, uMax_) : u;

    uHist_.push_front(uWe);
    const std::size_t docelowyRozmiarU =
        static_cast<std::size_t>(k_) + B_.size();
    if (uHist_.size() > docelowyRozmiarU)
    {
        uHist_.pop_back();
    }

    double sB = 0.0;
    if (!B_.empty())
    {
        auto itU = uHist_.begin();
        std::advance(itU, k_);

        sB = std::inner_product(B_.begin(), B_.end(), itU, 0.0);
    }

    double sA = 0.0;
    if (!A_.empty())
    {
        sA = std::inner_product(A_.begin(), A_.end(),
            yHist_.begin(), 0.0);
    }

    double y = sB - sA;
    if (sigma_ > 0.0)
    {
        y += sigma_ * losujNormalny();
    }

    double yWe = ograniczY_ ? clamp(y, yMin_, yMax_) : y;

    if (!yHist_.empty())
    {
        yHist_.push_front(yWe);
        if (yHist_.size() > A_.size())
        {
            yHist_.pop_back();
        }
    }

    return yWe;
}

// Konfiguracja
void ModelARX::ustawLimity(double uMin, double uMax,
    double yMin, double yMax)
{
    if (uMin > uMax)
        std::swap(uMin, uMax);
    if (yMin > yMax)
        std::swap(yMin, yMax);

    uMin_ = uMin;
    uMax_ = uMax;
    yMin_ = yMin;
    yMax_ = yMax;
}

void ModelARX::ustawSigma(double sigma)
{
    sigma_ = (sigma < 0.0) ? 0.0 : sigma;
}

void ModelARX::ustawWspolczynnikiA(const std::vector<double>& A)
{
    A_ = A;

    const std::size_t nowyRozmiar = A_.size();
    std::deque<double> nowaHistoria(nowyRozmiar, 0.0);

    const std::size_t ile = std::min(nowyRozmiar, yHist_.size());
    for (std::size_t i = 0; i < ile; ++i)
    {
        nowaHistoria[i] = yHist_[i];
    }

    yHist_.swap(nowaHistoria);
}

void ModelARX::ustawWspolczynnikiB(const std::vector<double>& B)
{
    B_ = B;

    const std::size_t docelowyRozmiarU =
        static_cast<std::size_t>(k_) + B_.size();
    std::deque<double> nowaHistoria(docelowyRozmiarU, 0.0);

    const std::size_t ile = std::min(docelowyRozmiarU, uHist_.size());
    for (std::size_t i = 0; i < ile; ++i)
    {
        nowaHistoria[i] = uHist_[i];
    }

    uHist_.swap(nowaHistoria);
}

void ModelARX::ustawOpoznienie(int k)
{
    if (k < 1)
    {
        k = 1;
    }

    if (k == k_)
    {
        return;
    }

    k_ = k;

    const std::size_t docelowyRozmiarU =
        static_cast<std::size_t>(k_) + B_.size();
    std::deque<double> nowaHistoria(docelowyRozmiarU, 0.0);

    const std::size_t ile = std::min(docelowyRozmiarU, uHist_.size());
    for (std::size_t i = 0; i < ile; ++i)
    {
        nowaHistoria[i] = uHist_[i];
    }

    uHist_.swap(nowaHistoria);
}

void ModelARX::ustawOgraniczanieU(bool wlacz)
{
    ograniczU_ = wlacz;
}

void ModelARX::ustawOgraniczanieY(bool wlacz)
{
    ograniczY_ = wlacz;
}

void ModelARX::reset()
{
    const std::size_t nA = A_.size();
    const std::size_t nB = B_.size();

    uHist_.assign(static_cast<std::size_t>(k_) + nB, 0.0);
    yHist_.assign(nA, 0.0);
}

