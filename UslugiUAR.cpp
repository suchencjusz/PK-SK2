#include "UslugiUAR.h"
#include "ProtokolUAR.h"
#include <QDateTime>

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

    // W trybie regulatora sieciowego pierwszy krok wysylamy od razu,
    // aby uruchomic obiekt po drugiej stronie.
    if (trybPracy_ == ProstyUAR::TrybPracy::SieciowyRegulator) {
        wykonajKrok();
    }
}

void UslugiUAR::stop()
{
    sym_.stop();
}

void UslugiUAR::reset()
{
    sym_.reset();
    ostatniaProbka_ = ProbkaUAR{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    m_krokSieciowySymulacji = 0;
    m_ostatniKrokProbkiSieciowej = 0;
    m_nadeszlaNowaProbkaSieciowa = false;
    m_lastReceiveTimeMs = 0;
    m_oczekujeResetuI = false;
    m_oczekujeResetuD = false;
    m_oczekujeStartuSieciowego = false;

    m_y_k1 = 0.0;
    m_y_k2 = 0.0;
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

    qDebug() << "[UslugiUAR] Wysylam PakietSterowania: krok=" << p.krok << "flagi=" << static_cast<int>(p.flagiSterowania);
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

        if (m_oczekujeStartuSieciowego)
            flagi |= SterowanieStartSymulacji;
        if (m_oczekujeResetuI)
            flagi |= SterowanieResetI;
        if (m_oczekujeResetuD)
            flagi |= SterowanieResetD;

        qDebug() << "[UslugiUAR] Regulator wykonal krok -> m_krokSieciowySymulacji=" << m_krokSieciowySymulacji << " wysylam sterowanie z flagami=" << static_cast<int>(flagi);
        wyslijPakietSterowaniaSieciowego(flagi);
        m_oczekujeStartuSieciowego = false;
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

    if (trybPracy_ != ProstyUAR::TrybPracy::SieciowyRegulator) {
        if (onProbka_) {
            onProbka_(ostatniaProbka_);
        }
    }

    return ostatniaProbka_;
}

// Tick timera
void UslugiUAR::onTimerTick()
{
    if (!sym_.czyUruchomiona())
        return;

    if (trybPracy_ == ProstyUAR::TrybPracy::SieciowyRegulator) {
        if (m_krokSieciowySymulacji > 0 && !m_nadeszlaNowaProbkaSieciowa) {
            // PAKIET ZAGUBIONY! Odpalamy "Zero-Order Hold" i ZAMRAŻAMY PID
            sym_.ustawPauzeRegulatora(true);

            // Trzymamy ostatnio znaną wartość y
            double y_zgadywane = m_y_k1;

            ustawSiecioweY(y_zgadywane);

            // Aktualizujemy historię (przy Zero-Order Hold po prostu powielamy starą wartość)
            m_y_k2 = m_y_k1;
            m_y_k1 = y_zgadywane;

            // Dorysowujemy brakującą próbkę na wykresie (zamykamy zawieszony obieg)
            ostatniaProbka_.y = y_zgadywane;
            ostatniaProbka_.e = ostatniaProbka_.w - y_zgadywane;
            ostatniaProbka_.zgadywana = true; // Zgłaszamy fałszywkę dla markera na wykresie

            if (onProbka_) {
                onProbka_(ostatniaProbka_);
            }
        } else {
            // PAKIET DOSTARCZONY (lub to pierwszy krok) - PID ODWIESZONY
            sym_.ustawPauzeRegulatora(false);
        }

        m_nadeszlaNowaProbkaSieciowa = false;
    }

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

void UslugiUAR::resetSymulacjiSieciowej()
{
    if (trybPracy_ != ProstyUAR::TrybPracy::SieciowyRegulator)
        return;

    wyslijPakietSterowaniaSieciowego(SterowanieResetSymulacji);
}

void UslugiUAR::stopSymulacjiSieciowej()
{
    if (trybPracy_ != ProstyUAR::TrybPracy::SieciowyRegulator)
        return;

    wyslijPakietSterowaniaSieciowego(SterowanieStopSymulacji);
}

void UslugiUAR::startSymulacjiSieciowej()
{
    if (trybPracy_ != ProstyUAR::TrybPracy::SieciowyRegulator)
        return;

    m_oczekujeStartuSieciowego = true;
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
        qDebug() << "[UslugiUAR] Odebrano PakietSterowania: krok=" << pakiet.krok << "flagi=" << static_cast<int>(pakiet.flagiSterowania) << " m_krokSieciowySymulacji=" << m_krokSieciowySymulacji;
        if (pakiet.rolaNadawcy != RolaInstancjiSieciowej::Regulator) {
            emit bladDekodowaniaRamki("Niepoprawna rola nadawcy dla pakietu Sterowanie.");
            return;
        }

        if ((pakiet.flagiSterowania & SterowanieResetSymulacji) != 0) {
            sym_.reset();
            m_krokSieciowySymulacji = 0;
            m_ostatniKrokProbkiSieciowej = 0;
            m_nadeszlaNowaProbkaSieciowa = false;
            m_lastReceiveTimeMs = 0;
            m_oczekujeResetuI = false;
            m_oczekujeResetuD = false;
            m_oczekujeStartuSieciowego = false;
            emit symulacjaZresetowanaZSieci();
        }

        if ((pakiet.flagiSterowania & SterowanieStopSymulacji) != 0) {
            sym_.stop();
            return;
        }

        if ((pakiet.flagiSterowania & SterowanieStartSymulacji) != 0) {
            sym_.start();
        }

        ustawSiecioweU(pakiet.u);
        ustawSiecioweW(pakiet.w);

        // Odbiorca wykonuje reset dokladnie wtedy, gdy przyjdzie odpowiednia flaga.
        if ((pakiet.flagiSterowania & SterowanieResetI) != 0)
            sym_.regulator().resetCalkowania();
        if ((pakiet.flagiSterowania & SterowanieResetD) != 0)
            sym_.regulator().resetRozniczkowania();

        const bool nowyKrok = (pakiet.krok > m_krokSieciowySymulacji);
        qDebug() << "[UslugiUAR] nowyKrok?" << nowyKrok << "pakiet.krok=" << pakiet.krok << "m_krokSieciowySymulacji=" << m_krokSieciowySymulacji;
        if (!nowyKrok)
            return;

        m_krokSieciowySymulacji = pakiet.krok;

        // Uruchamiamy krok tylko dla nowej probki sterowania.
        naPojedynczyKrokSieciowyObiektu();
        emit krokSieciowyZrealizowany();
        return;
    }

    auto pakiet = std::get<PakietProbkiSymulacji>(msg.payload);
    qDebug() << "[UslugiUAR] Odebrano ProbkaSymulacji: krok=" << pakiet.krok << " m_ostatniKrokProbkiSieciowej=" << m_ostatniKrokProbkiSieciowej;
    if (pakiet.rolaNadawcy != RolaInstancjiSieciowej::Obiekt) {
        emit bladDekodowaniaRamki("Niepoprawna rola nadawcy dla pakietu ProbkaSymulacji.");
        return;
    }

    if (pakiet.krok <= m_ostatniKrokProbkiSieciowej) {
        // Stara/duplikowana probka - ignorujemy, aby nie robic jitteru.
        return;
    }


    ustawSiecioweY(pakiet.y);

    // [DODANE] Zapisujemy prawdziwą historię do ekstrapolacji
    m_y_k2 = m_y_k1;
    m_y_k1 = pakiet.y;

    // Zamykamy obieg danych z tego kroku i aktualizujemy wykres
    if (trybPracy_ == ProstyUAR::TrybPracy::SieciowyRegulator) {
        ostatniaProbka_.y = pakiet.y;
        ostatniaProbka_.e = ostatniaProbka_.w - pakiet.y; // korekta wizualna uchybu na wykresie
        ostatniaProbka_.zgadywana = false; // Mamy prawdziwe dane, kasujemy flagę!

        if (onProbka_) {
            onProbka_(ostatniaProbka_);
        }
    }

    // Mierzymy rzeczywisty interwał przychodzenia ramek i porownujemy
    // z oczekiwanym interwalem symulacji (TTms). Emitujemy opoznienie w ms.
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    int32_t delayMs = 0;
    if (m_lastReceiveTimeMs > 0) {
        const qint64 interArrival = nowMs - m_lastReceiveTimeMs;
        const int expected = sym_.pobierzInterwalSymulacjiMs();
        delayMs = static_cast<int32_t>(interArrival - static_cast<qint64>(expected));
    }
    m_lastReceiveTimeMs = nowMs;

    // Emitujemy opoznienie w milisekundach (dodatnie => przyjscie mniej czesto niz oczekiwane)
    emit opoznienieWyliczone(delayMs);

    m_ostatniKrokProbkiSieciowej = pakiet.krok;
    m_nadeszlaNowaProbkaSieciowa = true;
    emit krokSieciowyZrealizowany();
}


