#include "UslugiUAR.h"

UslugiUAR::UslugiUAR(QObject* parent)
    : QObject(parent)
    , sym_(this)
{
    connect(&sym_, &SymulacjaUAR::tick, this, &UslugiUAR::onTimerTick);
}

// Sterowanie symulacja
void UslugiUAR::start()
{
    sym_.start();
}

void UslugiUAR::stop()
{
    sym_.stop();
}

void UslugiUAR::reset()
{
    sym_.reset();
    ostatniaProbka_ = ProbkaUAR{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
}

// Stan symulacji
bool UslugiUAR::czyUruchomiona() const
{
    return sym_.czyUruchomiona();
}

// Konfiguracja czasu
void UslugiUAR::ustawInterwalSymulacjiMs(int TTms)
{
    sym_.ustawInterwalSymulacjiMs(TTms);
}

int UslugiUAR::pobierzInterwalSymulacjiMs() const
{
    return sym_.pobierzInterwalSymulacjiMs();
}

void UslugiUAR::ustawOknoObserwacjiS(int oknoS)
{
    sym_.ustawOknoObserwacjiS(oknoS);
}

int UslugiUAR::pobierzOknoObserwacjiS() const
{
    return sym_.pobierzOknoObserwacjiS();
}

ProbkaUAR UslugiUAR::wykonajKrok()
{
    ostatniaProbka_ = sym_.wykonajKrok();

    if (onProbka_)
        onProbka_(ostatniaProbka_);

    return ostatniaProbka_;
}

// Tick timera
void UslugiUAR::onTimerTick()
{
    if (!sym_.czyUruchomiona())
        return;

    wykonajKrok();
}

// Parametry modelu
void UslugiUAR::ustawModelARX(const std::vector<double>& A,
    const std::vector<double>& B,
    int k,
    double sigma)
{
    sym_.model().ustawWspolczynnikiA(A);
    sym_.model().ustawWspolczynnikiB(B);
    sym_.model().ustawOpoznienie(k);
    sym_.model().ustawSigma(sigma);
}

void UslugiUAR::ustawLimityModelu(double uMin, double uMax, double yMin, double yMax)
{
    sym_.model().ustawLimity(uMin, uMax, yMin, yMax);
}

void UslugiUAR::ustawOgraniczanieSterowaniaModelu(bool wlacz)
{
    sym_.model().ustawOgraniczanieU(wlacz);
}

void UslugiUAR::ustawOgraniczanieWyjsciaModelu(bool wlacz)
{
    sym_.model().ustawOgraniczanieY(wlacz);
}

// Parametry PID
void UslugiUAR::ustawPID(double kP, double Ti, double Td)
{
    sym_.regulator().setWzmocnienie(kP);
    sym_.regulator().setStalaCalk(Ti);
    sym_.regulator().setStalaRoznicz(Td);
}

void UslugiUAR::ustawTrybCalkowaniaPID(RegulatorPID::LiczCalk tryb)
{
    sym_.regulator().setLiczCalk(tryb);
}

void UslugiUAR::resetCalkowaniaPID()
{
    sym_.regulator().resetCalkowania();
}

void UslugiUAR::resetRozniczkowaniaPID()
{
    sym_.regulator().resetRozniczkowania();
}

// Parametry generatora
void UslugiUAR::ustawTypGeneratora(GeneratorWartosciZadanej::Typ typ)
{
    sym_.generator().ustawTyp(typ);
}

void UslugiUAR::ustawOkresRzeczywistyGeneratora(double TRZ)
{
    sym_.generator().ustawOkresRzeczywisty(TRZ);
}

void UslugiUAR::ustawAmplitudaGeneratora(double A)
{
    sym_.generator().ustawAmplituda(A);
}

void UslugiUAR::ustawSkladowaStalaGeneratora(double S)
{
    sym_.generator().ustawSkladowaStala(S);
}

void UslugiUAR::ustawWypelnienieGeneratora(double p)
{
    sym_.generator().ustawWypelnienie(p);
}

// Konfiguracja pelna
KonfiguracjaUAR UslugiUAR::pobierzKonfiguracje() const
{
    return sym_.pobierzKonfiguracje();
}

void UslugiUAR::ustawKonfiguracje(const KonfiguracjaUAR& cfg)
{
    sym_.ustawKonfiguracje(cfg);
}

void UslugiUAR::zapiszKonfiguracjeJSON(const std::string& sciezka) const
{
    sym_.zapiszKonfiguracjeJSON(sciezka);
}

void UslugiUAR::wczytajKonfiguracjeJSON(const std::string& sciezka)
{
    sym_.wczytajKonfiguracjeJSON(sciezka);
}


