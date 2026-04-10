#include "UslugiUAR.h"
#include "ProtokolUAR.h"

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
    m_oczekujeResetuI = false;
    m_oczekujeResetuD = false;
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

void UslugiUAR::wyslijPakietSterowaniaSieciowego(uint8_t dodatkoweFlagi)
{
    if (trybPracy_ != ProstyUAR::TrybPracy::SieciowyRegulator)
        return;

    PakietSterowania p;
    p.rolaNadawcy = RolaInstancjiSieciowej::Regulator;
    p.krok = m_krokSieciowySymulacji;
    p.u = ostatniaProbka_.u;
    p.w = ostatniaProbka_.w;
    p.uP = ostatniaProbka_.uP;
    p.uI = ostatniaProbka_.uI;
    p.uD = ostatniaProbka_.uD;
    p.flagiSterowania = dodatkoweFlagi;

    const auto bytes = ProtokolUAR::zbudujRamkeSterowania(p);
    const QByteArray data(reinterpret_cast<const char*>(bytes.data()), static_cast<int>(bytes.size()));
    emit wyslijRamkeSieciowa(data);
}

ProbkaUAR UslugiUAR::wykonajKrok()
{
    ostatniaProbka_ = sym_.wykonajKrok();

    if (trybPracy_ != ProstyUAR::TrybPracy::SieciowyObiekt) {
        m_krokSieciowySymulacji++;
    }

    if (trybPracy_ == ProstyUAR::TrybPracy::SieciowyRegulator) {
        uint8_t flagi = SterowanieBrak;

        if (m_oczekujeResetuI)
            flagi |= SterowanieResetI;
        if (m_oczekujeResetuD)
            flagi |= SterowanieResetD;

        wyslijPakietSterowaniaSieciowego(flagi);
        m_oczekujeResetuI = false;
        m_oczekujeResetuD = false;
    } 
    else if (trybPracy_ == ProstyUAR::TrybPracy::SieciowyObiekt) {
        PakietProbkiSymulacji p;
        p.rolaNadawcy = RolaInstancjiSieciowej::Obiekt;
        p.krok = m_krokSieciowySymulacji;
        p.t = ostatniaProbka_.t;
        p.w = ostatniaProbka_.w;
        p.y = ostatniaProbka_.y;

        QByteArray data;
        auto bytes = ProtokolUAR::zbudujRamkeProbkiSymulacji(p);
        data = QByteArray(reinterpret_cast<const char*>(bytes.data()), static_cast<int>(bytes.size()));
        emit wyslijRamkeSieciowa(data);
    }

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

void UslugiUAR::naPojedynczyKrokSieciowyObiektu()
{
    if (trybPracy_ != ProstyUAR::TrybPracy::SieciowyObiekt)
        return;

    if (!sym_.czyUruchomiona()) {
        sym_.start();
    }

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

    if (trybPracy_ == ProstyUAR::TrybPracy::SieciowyRegulator) {
        m_oczekujeResetuI = true;

        if (!sym_.czyUruchomiona()) {
            uint8_t flagi = SterowanieBrak;
            if (m_oczekujeResetuI)
                flagi |= SterowanieResetI;
            if (m_oczekujeResetuD)
                flagi |= SterowanieResetD;

            wyslijPakietSterowaniaSieciowego(flagi);
            m_oczekujeResetuI = false;
            m_oczekujeResetuD = false;
        }
    }
}

void UslugiUAR::resetRozniczkowaniaPID()
{
    sym_.regulator().resetRozniczkowania();

    if (trybPracy_ == ProstyUAR::TrybPracy::SieciowyRegulator) {
        m_oczekujeResetuD = true;

        if (!sym_.czyUruchomiona()) {
            uint8_t flagi = SterowanieBrak;
            if (m_oczekujeResetuI)
                flagi |= SterowanieResetI;
            if (m_oczekujeResetuD)
                flagi |= SterowanieResetD;

            wyslijPakietSterowaniaSieciowego(flagi);
            m_oczekujeResetuI = false;
            m_oczekujeResetuD = false;
        }
    }
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

void UslugiUAR::przetworzRamkeSieciowa(const QByteArray& ramka)
{
    std::vector<uint8_t> bajty(ramka.begin(), ramka.end());
    OdebranaWiadomosc msg;
    std::string blad;

    if (!ProtokolUAR::dekodujRamke(bajty, msg, &blad)) {
        emit bladDekodowaniaRamki(QString::fromStdString(blad));
        return;
    }

    if (msg.typ == TypWiadomosci::Konfiguracja) {
        auto cfg = std::get<KonfiguracjaUAR>(msg.payload);
        ustawKonfiguracje(cfg);
        emit konfiguracjaZaktualizowanaZSieci();
        return;
    }

    if (trybPracy_ == ProstyUAR::TrybPracy::Stacjonarny) {
        // Po wyjsciu z trybu sieciowego moga dojsc pojedyncze opoznione ramki.
        return;
    }

    const bool lokalnieObiekt = (trybPracy_ == ProstyUAR::TrybPracy::SieciowyObiekt);
    const TypWiadomosci oczekiwanyTyp = lokalnieObiekt
                                            ? TypWiadomosci::Sterowanie
                                            : TypWiadomosci::ProbkaSymulacji;

    if (msg.typ != oczekiwanyTyp) {
        const QString lokalnaRola = lokalnieObiekt ? "Obiekt" : "Regulator";
        const QString oczekiwany = lokalnieObiekt ? "Sterowanie" : "ProbkaSymulacji";
        const QString odebrany =
            (msg.typ == TypWiadomosci::Sterowanie) ? "Sterowanie" : "ProbkaSymulacji";

        emit bladDekodowaniaRamki(
            QString("Niezgodna para ról: lokalnie %1, oczekiwano %2, odebrano %3. "
                    "To zwykle oznacza bledna pare (Regulator-Regulator lub Obiekt-Obiekt).")
                .arg(lokalnaRola)
                .arg(oczekiwany)
                .arg(odebrany));
        return;
    }

    if (lokalnieObiekt) {
        auto pakiet = std::get<PakietSterowania>(msg.payload);
        if (pakiet.rolaNadawcy != RolaInstancjiSieciowej::Regulator) {
            emit bladDekodowaniaRamki("Niepoprawna rola nadawcy dla pakietu Sterowanie.");
            return;
        }

        ustawSiecioweU(pakiet.u);
        ustawSiecioweW(pakiet.w);

        // Odbiorca wykonuje reset dokladnie wtedy, gdy przyjdzie odpowiednia flaga.
        if ((pakiet.flagiSterowania & SterowanieResetI) != 0)
            sym_.regulator().resetCalkowania();
        if ((pakiet.flagiSterowania & SterowanieResetD) != 0)
            sym_.regulator().resetRozniczkowania();

        const bool nowyKrok = (pakiet.krok != m_krokSieciowySymulacji);
        m_krokSieciowySymulacji = pakiet.krok;

        // Uruchamiamy krok tylko dla nowej probki sterowania.
        if (nowyKrok)
            naPojedynczyKrokSieciowyObiektu();
        return;
    }

    auto pakiet = std::get<PakietProbkiSymulacji>(msg.payload);
    if (pakiet.rolaNadawcy != RolaInstancjiSieciowej::Obiekt) {
        emit bladDekodowaniaRamki("Niepoprawna rola nadawcy dla pakietu ProbkaSymulacji.");
        return;
    }

    ustawSiecioweY(pakiet.y);

    // Opoznienie informacyjne (dla UI/wykresow)
    int32_t opoznienie = m_krokSieciowySymulacji - pakiet.krok;
    emit opoznienieWyliczone(opoznienie);
}


