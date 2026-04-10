#include "SymulacjaUAR.h"

SymulacjaUAR::SymulacjaUAR(QObject* parent)
    : QObject(parent)
    , model_({ -0.4, 0.0, 0.0 }, { 0.6, 0.0, 0.0 }, 1, 0.0)
    , regulator_(0.5, 5.0, 0.2)
    , uar_(model_, regulator_)
    , generator_()
    , TTms_(200)
    , oknoObserwacjiS_(10)
    , i_(0)
    , t_(0.0)
    , uruchomiona_(false)
    , ostatniaProbka_{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }
    , timer_(new QTimer(this))
{
    // Symulacja (czas + timer)
    generator_.ustawInterwalSymulacjiMs(TTms_);
    timer_->setInterval(TTms_);
    connect(timer_, &QTimer::timeout, this, &SymulacjaUAR::tick);

    // ARX (limity)
    model_.ustawLimity(-10.0, 10.0, -10.0, 10.0);

    // PID (tryb calkowania)
    regulator_.setLiczCalk(RegulatorPID::LiczCalk::Zewnetrzne);

    // Generator wartosci zadanej
    generator_.ustawTyp(GeneratorWartosciZadanej::Typ::Prostokat);
    generator_.ustawOkresRzeczywisty(10.0);
    generator_.ustawAmplituda(1.0);
    generator_.ustawSkladowaStala(0.0);
    generator_.ustawWypelnienie(0.5);
}

// Reset / krok
void SymulacjaUAR::reset()
{
    i_ = 0;
    t_ = 0.0;

    ostatniaProbka_ = ProbkaUAR{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    model_.reset();
    regulator_.reset();
    uar_.reset();
}

// Czas symulacji
void SymulacjaUAR::ustawInterwalSymulacjiMs(int TTms)
{
    if (TTms <= 0)
        TTms_ = 200;
    else
        TTms_ = TTms;

    generator_.ustawInterwalSymulacjiMs(TTms_);
    timer_->setInterval(TTms_);
}

void SymulacjaUAR::ustawOknoObserwacjiS(int oknoS)
{
    if (oknoS < 10)
        oknoObserwacjiS_ = 10;
    else if (oknoS > 50)
        oknoObserwacjiS_ = 50;
    else
        oknoObserwacjiS_ = oknoS;
}

// Sterowanie
void SymulacjaUAR::start()
{
    uruchomiona_ = true;
    if (tryb_ == ProstyUAR::TrybPracy::SieciowyObiekt) {
        timer_->stop();
    } else {
        timer_->start();
    }
}

void SymulacjaUAR::stop()
{
    uruchomiona_ = false;
    timer_->stop();
}

void SymulacjaUAR::ustawTrybPracy(ProstyUAR::TrybPracy tryb)
{
    tryb_ = tryb;
    uar_.ustawTrybPracy(tryb);

    if (tryb_ == ProstyUAR::TrybPracy::SieciowyObiekt) {
        timer_->stop(); // Takty dyktuje regulator, wiec my przestajemy uzywac naszego zegara
    } else if (uruchomiona_) {
        timer_->start(); // Jeśli stacjonalnie/regulator, wracamy do taktowania wlasnym zegarem
    }
}

ProbkaUAR SymulacjaUAR::wykonajKrok()
{
    // Czas symulacji (ciagly, niezalezny od zmian TTms)
    const double t = t_;

    // 1. Pobierz wartosc zadana z generatora i wykonaj krok petli UAR
    uar_.symuluj(generator_.generuj(i_));

    // 2. Pobierz sygnaly wewnetrzne do wizualizacji
    ProbkaUAR p;
    p.t = t;
    p.w = uar_.pobierzW();
    p.y = uar_.pobierzY();
    p.e = uar_.pobierzE();
    p.u = uar_.pobierzU();
    p.uP = uar_.pobierzUP();
    p.uI = uar_.pobierzUI();
    p.uD = uar_.pobierzUD();

    ostatniaProbka_ = p;

    // W trybie obiektu krok wykonuje sie na ramke z sieci, ale nadal musimy przesuwac os czasu
    // do rysowania wykresow i spojnosc kroku symulacji tla.
    i_++;
    t_ += static_cast<double>(TTms_) / 1000.0;

    return p;
}

// Konfiguracja
KonfiguracjaUAR SymulacjaUAR::pobierzKonfiguracje() const
{
    KonfiguracjaUAR cfg;

    // Symulacja
    cfg.symulacjaTTms = TTms_;
    cfg.oknoObserwacjiS = oknoObserwacjiS_;

    // ARX
    cfg.A = model_.pobierzA();
    cfg.B = model_.pobierzB();
    cfg.k = model_.pobierzK();
    cfg.sigma = model_.pobierzSigma();

    model_.pobierzLimity(cfg.uMin, cfg.uMax, cfg.yMin, cfg.yMax);
    cfg.ograniczU = model_.czyOgraniczanieU();
    cfg.ograniczY = model_.czyOgraniczanieY();

    // PID
    cfg.kP = regulator_.pobierzWzmocnienie();
    cfg.Ti = regulator_.pobierzStalaCalk();
    cfg.Td = regulator_.pobierzStalaRoznicz();
    cfg.trybCalk = regulator_.pobierzTrybCalk();

    // Generator
    cfg.typGeneratora = generator_.pobierzTyp();
    cfg.TRZ = generator_.pobierzOkresRzeczywisty();
    cfg.A_gen = generator_.pobierzAmplituda();
    cfg.S_gen = generator_.pobierzSkladowaStala();
    cfg.p_gen = generator_.pobierzWypelnienie();

    return cfg;
}

void SymulacjaUAR::ustawKonfiguracje(const KonfiguracjaUAR& cfg)
{
    // ARX: wymuszenie min. 3 wspolczynnikow
    auto A = cfg.A;
    auto B = cfg.B;
    if (A.empty())
        A = model_.pobierzA();
    if (B.empty())
        B = model_.pobierzB();
    if (A.size() < 3)
        A.resize(3, 0.0);
    if (B.size() < 3)
        B.resize(3, 0.0);

    // Symulacja
    ustawInterwalSymulacjiMs(cfg.symulacjaTTms);
    ustawOknoObserwacjiS(cfg.oknoObserwacjiS);

    // ARX
    model_.ustawWspolczynnikiA(A);
    model_.ustawWspolczynnikiB(B);
    model_.ustawOpoznienie(cfg.k);
    model_.ustawSigma(cfg.sigma);
    model_.ustawLimity(cfg.uMin, cfg.uMax, cfg.yMin, cfg.yMax);
    model_.ustawOgraniczanieU(cfg.ograniczU);
    model_.ustawOgraniczanieY(cfg.ograniczY);

    // PID
    regulator_.setWzmocnienie(cfg.kP);
    regulator_.setStalaCalk(cfg.Ti);
    regulator_.setStalaRoznicz(cfg.Td);
    regulator_.setLiczCalk(cfg.trybCalk);

    // Generator
    generator_.ustawTyp(cfg.typGeneratora);
    generator_.ustawOkresRzeczywisty(cfg.TRZ);
    generator_.ustawAmplituda(cfg.A_gen);
    generator_.ustawSkladowaStala(cfg.S_gen);
    generator_.ustawWypelnienie(cfg.p_gen);
}

// Zapis / odczyt
void SymulacjaUAR::zapiszKonfiguracjeJSON(const std::string& nazwaPliku) const
{
    KonfiguracjaUAR cfg = pobierzKonfiguracje();
    cfg.zapiszDoPlikuJSON(nazwaPliku);
}

void SymulacjaUAR::wczytajKonfiguracjeJSON(const std::string& nazwaPliku)
{
    const KonfiguracjaUAR cfg = KonfiguracjaUAR::wczytajZPlikuJSON(nazwaPliku);
    ustawKonfiguracje(cfg);
}

