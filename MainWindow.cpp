#include "MainWindow.h"
#include "ARXDialog.h"
#include "ui_MainWindow.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QPushButton>
#include <QLCDNumber>
#include <QMessageBox>
#include <QFontMetrics>
#include <QSpinBox>

#include <algorithm>
#include <cmath>

//sieci
#include "siecidialog.h"
#include "ProtokolUAR.h"

namespace {

QString opisRoliSieciowej(SiecDialog::RolaInstancji rola)
{
    switch (rola) {
    case SiecDialog::RolaInstancji::Regulator:
        return "Regulator";
    case SiecDialog::RolaInstancji::Obiekt:
        return "Obiekt";
    }

    return "Nieznana";
}

QString opisTrybuPolaczenia(SiecDialog::TrybPolaczenia tryb,
                            const QString& ip,
                            uint16_t port)
{
    switch (tryb) {
    case SiecDialog::TrybPolaczenia::Serwer:
        return QString("Serwer: port %1 (oczekiwanie na klienta)").arg(port);
    case SiecDialog::TrybPolaczenia::Klient:
        return QString("Klient: %1:%2").arg(ip).arg(port);
    }

    return "Tryb połączenia: nieznany";
}

QString formatujBitySieci(quint64 bajty)
{
    const double bity = static_cast<double>(bajty) * 8.0;

    if (bity < 1000.0)
        return QString::number(bity, 'f', 0) + " b";
    if (bity < 1000.0 * 1000.0)
        return QString::number(bity / 1000.0, 'f', 2) + " kb";
    if (bity < 1000.0 * 1000.0 * 1000.0)
        return QString::number(bity / (1000.0 * 1000.0), 'f', 2) + " mb";

    return QString::number(bity / (1000.0 * 1000.0 * 1000.0), 'f', 2) + " gb";
}

} // namespace


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , uslugi_(this)
{

    budujInterfejs();
    konfigurujWykresy();
    zainicjujWatchdog();

    // Zasil kontrolki wartosciami z backendu
    odswiezGUIzKonfiguracji();

    // Cache (os X + margines)
    interwalMs_ = spinInterwal_->value();
    oknoObserwacjiS_ = spinOkno_->value();

    uslugi_.ustawCallbackProbki([this](const ProbkaUAR& p) { onProbka(p); });
}

MainWindow::~MainWindow()
{
    delete ui_;
}

// Budowa UI

void MainWindow::budujInterfejs()
{
    ui_ = new Ui::MainWindow();
    ui_->setupUi(this);

    plotWY_  = ui_->plotWY;
    plotE_   = ui_->plotE;
    plotU_   = ui_->plotU;
    plotPID_ = ui_->plotPID;

    lcdW_  = ui_->lcdW;
    lcdY_  = ui_->lcdY;
    lcdE_  = ui_->lcdE;
    lcdU_  = ui_->lcdU;
    lcdUP_ = ui_->lcdUP;
    lcdUI_ = ui_->lcdUI;
    lcdUD_ = ui_->lcdUD;

    spinInterwal_ = ui_->spinInterwal;
    spinOkno_     = ui_->spinOkno;

    comboGenTyp_       = ui_->comboGenTyp;
    spinGenAmpl_       = ui_->spinGenAmpl;
    spinGenOkres_      = ui_->spinGenOkres;
    spinGenWypelnienie_ = ui_->spinGenWypelnienie;
    spinGenOffset_     = ui_->spinGenOffset;

    spinKp_       = ui_->spinKp;
    spinTi_       = ui_->spinTi;
    spinTd_       = ui_->spinTd;
    comboTrybCalk_ = ui_->comboTrybCalk;


    const QFontMetrics fm(font());
    const int ctrlHeight = fm.height() + 12;
    const int btnHeight = ctrlHeight + 2;

    ui_->btnStart->setMinimumHeight(btnHeight);
    ui_->btnStop->setMinimumHeight(btnHeight);
    ui_->btnReset->setMinimumHeight(btnHeight);
    ui_->btnResetI->setMinimumHeight(btnHeight);
    ui_->btnResetD->setMinimumHeight(btnHeight);
    ui_->btnZapisz->setMinimumHeight(btnHeight);
    ui_->btnWczytaj->setMinimumHeight(btnHeight);
    ui_->btnARX->setMinimumHeight(btnHeight);
    ui_->btnTrybSieciowy->setFixedHeight(btnHeight);
    ui_->btnRozlaczSiec->setFixedHeight(btnHeight);



    spinInterwal_->setMinimumHeight(ctrlHeight);
    spinOkno_->setMinimumHeight(ctrlHeight);
    spinKp_->setMinimumHeight(ctrlHeight);
    spinTi_->setMinimumHeight(ctrlHeight);
    spinTd_->setMinimumHeight(ctrlHeight);
    comboTrybCalk_->setMinimumHeight(ctrlHeight);
    comboGenTyp_->setMinimumHeight(ctrlHeight);
    spinGenAmpl_->setMinimumHeight(ctrlHeight);
    spinGenOkres_->setMinimumHeight(ctrlHeight);
    spinGenWypelnienie_->setMinimumHeight(ctrlHeight);
    spinGenOffset_->setMinimumHeight(ctrlHeight);


    auto setupLcd = [&](QLCDNumber* lcd) {
        lcd->setMinimumHeight(ctrlHeight);
    };

    setupLcd(lcdW_); setupLcd(lcdY_); setupLcd(lcdE_); setupLcd(lcdU_);
    setupLcd(lcdUP_); setupLcd(lcdUI_); setupLcd(lcdUD_);

    ui_->settingsPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    ui_->settingsPanel->adjustSize();
    const int panelWidth = ui_->settingsPanel->sizeHint().width();
    ui_->settingsPanel->setFixedWidth(panelWidth);
    const int scrollBarWidth = 12;
    ui_->settingsScroll->setFixedWidth(panelWidth + scrollBarWidth);

    // Polaczenia sygnalow
    connect(ui_->btnStart,  &QPushButton::clicked, this, &MainWindow::onBtnStart);
    connect(ui_->btnStop,   &QPushButton::clicked, this, &MainWindow::onBtnStop);
    connect(ui_->btnReset,  &QPushButton::clicked, this, &MainWindow::onBtnReset);
    connect(ui_->btnZapisz, &QPushButton::clicked, this, &MainWindow::onBtnZapisz);
    connect(ui_->btnWczytaj,&QPushButton::clicked, this, &MainWindow::onBtnWczytaj);

    connect(ui_->btnARX, &QPushButton::clicked, this, &MainWindow::otworzOknoParametrowARX);

    connect(spinOkno_, &QSpinBox::editingFinished, [this]() {
        if (m_trwaZdalnaSynchronizacjaKonfiguracji)
            return;
        oknoObserwacjiS_ = spinOkno_->value();
        uslugi_.ustawOknoObserwacjiS(spinOkno_->value());
        wyslijBiezacaKonfiguracjeSieciowa();
    });

    connect(spinInterwal_, &QSpinBox::editingFinished, [this]() {
        if (m_trwaZdalnaSynchronizacjaKonfiguracji)
            return;
        const int ms = spinInterwal_->value();
        uslugi_.ustawInterwalSymulacjiMs(ms);
        interwalMs_ = ms;
        wyslijBiezacaKonfiguracjeSieciowa();
    });

    const QList<QDoubleSpinBox*> params = {
        spinKp_, spinTi_, spinTd_,
        spinGenAmpl_, spinGenOkres_, spinGenWypelnienie_, spinGenOffset_
    };

    for (auto* p : params)
        connect(p, &QDoubleSpinBox::editingFinished, this, &MainWindow::aktualizujParametryOnline);

    connect(comboGenTyp_,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::aktualizujParametryOnline);

    connect(comboTrybCalk_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::aktualizujParametryOnline);

    connect(ui_->btnResetI, &QPushButton::clicked, [this]() { uslugi_.resetCalkowaniaPID(); });
    connect(ui_->btnResetD, &QPushButton::clicked, [this]() { uslugi_.resetRozniczkowaniaPID(); });

    //sieci ustawienia (UI łączy on_btnTrybSieciowy_clicked automatycznie, nie duplikujemy connect!)
    aktualizujDostepnoscKontrolek();

    polaczenieSieciowe_ = new PolaczenieSieciowe(this);
    connect(polaczenieSieciowe_, &PolaczenieSieciowe::zdarzenieTCP, this, &MainWindow::onTcpZdarzenie);
    connect(polaczenieSieciowe_, &PolaczenieSieciowe::stanZmieniony, this, &MainWindow::onTcpStanZmieniony);
    connect(polaczenieSieciowe_, &PolaczenieSieciowe::polaczenieUtracone, this, &MainWindow::onPolaczenieUtracone);
    connect(polaczenieSieciowe_, &PolaczenieSieciowe::ramkaOdebrana, this, &MainWindow::onTcpRamkaOdebrana);
    connect(polaczenieSieciowe_, &PolaczenieSieciowe::statystykiZaktualizowane, this, &MainWindow::onTcpStatystyki);
    
    // Nowe podłączenia logiki sieciowej UslugiUAR
    connect(&uslugi_, &UslugiUAR::wyslijRamkeSieciowa, polaczenieSieciowe_, &PolaczenieSieciowe::wyslijRamke);
    connect(&uslugi_, &UslugiUAR::opoznienieWyliczone, this, &MainWindow::aktualizujOpoznienie);
    connect(&uslugi_, &UslugiUAR::bladDekodowaniaRamki, [this](const QString& powod) {
        qDebug() << "[TCP] Błąd dekodowania:" << powod;

        if (powod.startsWith("Niezgodna para ról")) {
            rozlaczSiecAWARYJNIE(powod);
            return;
        }

        m_incydentyBlednegoDekodowania++;
    });
    connect(&uslugi_, &UslugiUAR::konfiguracjaZaktualizowanaZSieci, [this]() {
        m_trwaZdalnaSynchronizacjaKonfiguracji = true;
        odswiezGUIzKonfiguracji();
        m_trwaZdalnaSynchronizacjaKonfiguracji = false;
        aktualizujDostepnoscKontrolek();
    });

    ui_->lblStatusPolaczenia->setWordWrap(true);
    m_opisSesjiPolaczenia = "Tryb: Stacjonarny";
    m_ostatniStanPolaczenia = "Brak aktywnego połączenia";
    odswiezTelemetryPolaczenia();
}

// Wykresy
void MainWindow::konfigurujWykresy()
{
    auto setupPlot = [](QCustomPlot* p, const QString& title, const QString& yLabel) {
        p->plotLayout()->insertRow(0);
        p->plotLayout()->addElement(0, 0, new QCPTextElement(p, title, QFont("sans", 10, QFont::Bold)));

        p->xAxis->setLabel("Czas [s]");
        p->yAxis->setLabel(yLabel);

        p->legend->setVisible(true);
        QFont legendFont = p->font();
        legendFont.setPointSize(8);
        p->legend->setFont(legendFont);
        p->legend->setBrush(QBrush(QColor(255, 255, 255, 200)));
        p->legend->setBorderPen(Qt::NoPen);
        p->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

        p->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    };

    setupPlot(plotWY_, "Regulacja", "Wartosc");
    plotWY_->addGraph(); plotWY_->graph(0)->setPen(QPen(Qt::blue, 2)); plotWY_->graph(0)->setName("w(t)");
    plotWY_->addGraph(); plotWY_->graph(1)->setPen(QPen(Qt::red,  2)); plotWY_->graph(1)->setName("y(t)");

    setupPlot(plotE_, "Uchyb", "e(t)");
    plotE_->addGraph(); plotE_->graph(0)->setPen(QPen(Qt::magenta)); plotE_->graph(0)->setName("e(t)");

    setupPlot(plotU_, "Sterowanie", "u(t)");
    plotU_->addGraph(); plotU_->graph(0)->setPen(QPen(Qt::darkGreen)); plotU_->graph(0)->setName("u(t)");

    setupPlot(plotPID_, "Skladowe PID", "Wartosc");
    plotPID_->addGraph(); plotPID_->graph(0)->setPen(QPen(Qt::blue));                 plotPID_->graph(0)->setName("uP(t)");
    plotPID_->addGraph(); plotPID_->graph(1)->setPen(QPen(QColor(255, 100, 0)));      plotPID_->graph(1)->setName("uI(t)");
    plotPID_->addGraph(); plotPID_->graph(2)->setPen(QPen(Qt::magenta));              plotPID_->graph(2)->setName("uD(t)");
}

// Tick/Rysowanie
void MainWindow::onProbka(const ProbkaUAR& p)
{
    plotWY_->graph(0)->addData(p.t, p.w);
    plotWY_->graph(1)->addData(p.t, p.y);
    plotE_->graph(0)->addData(p.t, p.e);
    plotU_->graph(0)->addData(p.t, p.u);
    plotPID_->graph(0)->addData(p.t, p.uP);
    plotPID_->graph(1)->addData(p.t, p.uI);
    plotPID_->graph(2)->addData(p.t, p.uD);

    // Os X: okno obserwacji + margines max 3 probki
    const double windowWidth = static_cast<double>(oknoObserwacjiS_);


    const double dt = static_cast<double>(interwalMs_) / 1000.0;
    const double rightPad = std::min(3.0 * dt, windowWidth);
    const int targetN = (dt > 0.0)
                            ? std::max(1, static_cast<int>(std::lround(windowWidth / dt)))
                            : 1;

    QCPRange xRange;
    if (p.t > windowWidth) {
        const double xMax = p.t + rightPad;
        const double xMin = xMax - windowWidth;
        xRange = QCPRange(xMin, xMax);
    } else {
        xRange = QCPRange(0.0, p.t + rightPad);
    }

    plotWY_->xAxis->setRange(xRange);

    plotE_->xAxis->setRange(xRange);
    plotU_->xAxis->setRange(xRange);
    plotPID_->xAxis->setRange(xRange);

    // Usuwanie starych probek
    auto pruneGraph = [targetN](QCPGraph* graph) {
        const int count = graph->dataCount();
        if (count <= targetN)
            return;

        const int removeCount = std::min(2, count - targetN);
        auto data = graph->data();
        auto it = data->constBegin();
        std::advance(it, removeCount);
        data->removeBefore(it->key);
    };

    pruneGraph(plotWY_->graph(0));
    pruneGraph(plotWY_->graph(1));
    pruneGraph(plotE_->graph(0));
    pruneGraph(plotU_->graph(0));
    pruneGraph(plotPID_->graph(0));
    pruneGraph(plotPID_->graph(1));
    pruneGraph(plotPID_->graph(2));

    // SmartScale Y
    auto smartScale = [](QCustomPlot* plot) {
        bool found = false;
        QCPRange tight;

        for (int i = 0; i < plot->graphCount(); ++i) {
            bool valid = false;
            const QCPRange gr = plot->graph(i)->getValueRange(valid, QCP::sdBoth, plot->xAxis->range());
            if (valid) {
                if (!found) { tight = gr; found = true; }
                else { tight.expand(gr); }
            }
        }

        if (!found) {
            plot->yAxis->setRange(-1, 1);
            return;
        }

        double center = tight.center();
        double size = tight.size();
        if (size < 1e-6) size = 2.0;

        const double newSize = size * 1.25;
        plot->yAxis->setRange(center - newSize / 2.0, center + newSize / 2.0);
    };

    smartScale(plotWY_);
    smartScale(plotE_);
    smartScale(plotU_);
    smartScale(plotPID_);

    plotWY_->replot();
    plotE_->replot();
    plotU_->replot();
    plotPID_->replot();

    aktualizujWyswietlaczeNumeryczne(p);
}

void MainWindow::aktualizujWyswietlaczeNumeryczne(const ProbkaUAR& p)
{
    auto formatLcd = [](double value) {
        const int decimals = value < 0.0 ? 2 : 3;
        return QString::number(value, 'f', decimals);
    };

    lcdW_->display(formatLcd(p.w));
    lcdY_->display(formatLcd(p.y));
    lcdE_->display(formatLcd(p.e));
    lcdU_->display(formatLcd(p.u));
    lcdUP_->display(formatLcd(p.uP));
    lcdUI_->display(formatLcd(p.uI));
    lcdUD_->display(formatLcd(p.uD));
}

void MainWindow::wyzerujWyswietlacze()
{
    const QString zero = QString::number(0.0, 'f', 3);
    lcdW_->display(zero);
    lcdY_->display(zero);
    lcdE_->display(zero);
    lcdU_->display(zero);
    lcdUP_->display(zero);
    lcdUI_->display(zero);
    lcdUD_->display(zero);
}

void MainWindow::wyczyscWykresy()
{
    auto clearPlot = [](QCustomPlot* p) {
        for (int i = 0; i < p->graphCount(); ++i)
            p->graph(i)->data()->clear();
        p->replot();
    };

    clearPlot(plotWY_);
    clearPlot(plotE_);
    clearPlot(plotU_);
    clearPlot(plotPID_);
}

// Przyciski

void MainWindow::onBtnStart()
{
    aktualizujParametryOnline();
    m_watchdogUzbrojonyPoPierwszejRamce = false;
    m_odebranoRamkeZWatchdoga = true;
    m_incydentyBrakRamki = 0;
    uslugi_.start();
}

void MainWindow::onBtnStop()
{
    uslugi_.stop();
    m_watchdogUzbrojonyPoPierwszejRamce = false;
}

void MainWindow::onBtnReset()
{
    onBtnStop();
    uslugi_.reset();
    wyczyscWykresy();
    wyzerujWyswietlacze();
}

// PID/Generator
void MainWindow::aktualizujParametryOnline()
{
    if (m_trwaZdalnaSynchronizacjaKonfiguracji)
        return;

    uslugi_.ustawPID(spinKp_->value(), spinTi_->value(), spinTd_->value());

    uslugi_.ustawTrybCalkowaniaPID(
        comboTrybCalk_->currentIndex() == 0
            ? RegulatorPID::LiczCalk::Zewnetrzne
            : RegulatorPID::LiczCalk::Wewnetrzne
        );

    uslugi_.ustawTypGeneratora(
        comboGenTyp_->currentIndex() == 0
            ? GeneratorWartosciZadanej::Typ::Prostokat
            : GeneratorWartosciZadanej::Typ::Sinus
        );

    uslugi_.ustawAmplitudaGeneratora(spinGenAmpl_->value());
    uslugi_.ustawOkresRzeczywistyGeneratora(spinGenOkres_->value());
    uslugi_.ustawWypelnienieGeneratora(spinGenWypelnienie_->value());
    uslugi_.ustawSkladowaStalaGeneratora(spinGenOffset_->value());

    wyslijBiezacaKonfiguracjeSieciowa();
}

// Zapis/Odczyt

void MainWindow::onBtnZapisz()
{
    const QString f = QFileDialog::getSaveFileName(this, "Zapisz", "", "JSON (*.json)");
    if (f.isEmpty()) return;

    try {
        uslugi_.zapiszKonfiguracjeJSON(f.toStdString());
    } catch (...) {
        // celowo cicho (minimal)
    }
}

void MainWindow::onBtnWczytaj()
{
    const QString f = QFileDialog::getOpenFileName(this, "Wczytaj", "", "JSON (*.json)");
    if (f.isEmpty())
        return;

    try
    {
        uslugi_.wczytajKonfiguracjeJSON(f.toStdString());
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, "Blad wczytywania", QString::fromUtf8(e.what()));
        return;
    }
    catch (...)
    {
        QMessageBox::critical(this, "Blad wczytywania", "Nieznany blad podczas wczytywania pliku JSON.");
        return;
    }

    odswiezGUIzKonfiguracji();
    wyslijBiezacaKonfiguracjeSieciowa();
}


// Odswiezanie GUI

void MainWindow::odswiezGUIzKonfiguracji()
{
    const KonfiguracjaUAR cfg = uslugi_.pobierzKonfiguracje();

    // PID
    spinKp_->setValue(cfg.kP);
    spinTi_->setValue(cfg.Ti);
    spinTd_->setValue(cfg.Td);

    comboTrybCalk_->setCurrentIndex(
        cfg.trybCalk == RegulatorPID::LiczCalk::Zewnetrzne ? 0 : 1
        );

    // Generator
    comboGenTyp_->setCurrentIndex(
        cfg.typGeneratora == GeneratorWartosciZadanej::Typ::Prostokat ? 0 : 1
        );

    spinGenAmpl_->setValue(cfg.A_gen);
    spinGenOkres_->setValue(cfg.TRZ);
    spinGenWypelnienie_->setValue(cfg.p_gen);
    spinGenOffset_->setValue(cfg.S_gen);

    // Symulacja (TTms)
    spinInterwal_->setValue(cfg.symulacjaTTms);
    interwalMs_ = cfg.symulacjaTTms;
    spinOkno_->setValue(cfg.oknoObserwacjiS);
    oknoObserwacjiS_ = spinOkno_->value();
}

// Okno ARX

void MainWindow::otworzOknoParametrowARX()
{
    const KonfiguracjaUAR cfg = uslugi_.pobierzKonfiguracje();

    ARXDialog dlg(cfg, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    bool zmieniono = false;

    if (dlg.modelChanged()) {
        uslugi_.ustawModelARX(dlg.pobierzA(), dlg.pobierzB(), dlg.pobierzK(), dlg.pobierzSigma());
        zmieniono = true;
    }

    if (dlg.limitsChanged()) {
        uslugi_.ustawLimityModelu(dlg.pobierzUMin(), dlg.pobierzUMax(), dlg.pobierzYMin(), dlg.pobierzYMax());
        zmieniono = true;
    }

    if (dlg.flagsChanged()) {
        uslugi_.ustawOgraniczanieSterowaniaModelu(dlg.pobierzOgraniczU());
        uslugi_.ustawOgraniczanieWyjsciaModelu(dlg.pobierzOgraniczY());
        zmieniono = true;
    }

    if (zmieniono)
        wyslijBiezacaKonfiguracjeSieciowa();
}

//sieci

void MainWindow::wyzerujStanSieci()
{
    m_ostatnieOpoznienieSieci = 0;
    m_incydentyBrakRamki = 0;
    m_incydentyBlednegoDekodowania = 0;
    m_incydentyOpoznienia = 0;
    m_odebranoRamkeZWatchdoga = false;
    m_watchdogUzbrojonyPoPierwszejRamce = false;
    m_telemetriaWyslaneBajty = 0;
    m_telemetriaOdebraneBajty = 0;
}

void MainWindow::odswiezTelemetryPolaczenia()
{
    const QString sesja = m_opisSesjiPolaczenia.isEmpty()
                              ? QString("Tryb: Stacjonarny")
                              : m_opisSesjiPolaczenia;
    const QString stan = m_ostatniStanPolaczenia.isEmpty()
                             ? QString("Brak aktywnego połączenia")
                             : m_ostatniStanPolaczenia;

    const QString info = QString("%1\nStan połączenia: %2\n\nTelemetria:\nWysłano: %3\nOdebrano: %4")
                             .arg(sesja)
                             .arg(stan)
                             .arg(formatujBitySieci(m_telemetriaWyslaneBajty))
                             .arg(formatujBitySieci(m_telemetriaOdebraneBajty));
    ui_->lblStatusPolaczenia->setText(info);
}

void MainWindow::aktualizujDostepnoscKontrolek()
{
    const bool trybStacjonarny = (trybPracy_ == TrybPracy::Stacjonarny);
    const bool siecPolaczona = (polaczenieSieciowe_ && polaczenieSieciowe_->czyPolaczono());

    const bool wlaczGrupySterowania = (trybPracy_ != TrybPracy::SieciowyObiekt);
    const bool wlaczGrupeARX = (trybPracy_ != TrybPracy::SieciowyObiekt);

    ui_->grpGen->setEnabled(wlaczGrupySterowania);
    ui_->grpPID->setEnabled(wlaczGrupySterowania);
    ui_->grpSim->setEnabled(wlaczGrupySterowania);
    ui_->grpFile->setEnabled(wlaczGrupySterowania);
    ui_->grpARX->setEnabled(wlaczGrupeARX);

    const bool mozeWystartowac = trybStacjonarny || (trybPracy_ == TrybPracy::SieciowyRegulator && siecPolaczona);
    
    ui_->btnStart->setEnabled(mozeWystartowac);

    ui_->btnTrybSieciowy->setEnabled(trybStacjonarny);
    ui_->btnRozlaczSiec->setEnabled(!trybStacjonarny);
}

void MainWindow::on_btnTrybSieciowy_clicked()
{
    SiecDialog dlg(polaczenieSieciowe_, this);

    if (dlg.exec() == QDialog::Accepted) {
        auto rola = dlg.pobierzRole();
        auto tryb = dlg.pobierzTrybPolaczenia();

        QString ip = dlg.pobierzAdresIP();
        uint16_t port = dlg.pobierzPort();

        if (rola == SiecDialog::RolaInstancji::Regulator) {
            trybPracy_ = TrybPracy::SieciowyRegulator;
            uslugi_.ustawTrybPracy(ProstyUAR::TrybPracy::SieciowyRegulator);
        } else {
            trybPracy_ = TrybPracy::SieciowyObiekt;
            uslugi_.ustawTrybPracy(ProstyUAR::TrybPracy::SieciowyObiekt);
        }

        wyzerujStanSieci();
        m_odebranoRamkeZWatchdoga = true; // zeby od razu nie wybalac bledu
        
        aktualizujDostepnoscKontrolek();

        m_opisSesjiPolaczenia = QString("Tryb sieciowy: %1")
                        .arg(opisRoliSieciowej(rola));
        m_ostatniStanPolaczenia = opisTrybuPolaczenia(tryb, ip, port);
        odswiezTelemetryPolaczenia();

        if (rola == SiecDialog::RolaInstancji::Regulator &&
            polaczenieSieciowe_ && polaczenieSieciowe_->czyPolaczono()) {
            wyslijBiezacaKonfiguracjeSieciowa();
        }

    }
}

void MainWindow::on_btnRozlaczSiec_clicked()
{
    if (trybPracy_ == TrybPracy::Stacjonarny) {
        return;
    }

    if (QMessageBox::question(this,
                              "Przejście do trybu stacjonarnego",
                              "Czy na pewno chcesz zakończyć pracę sieciową i przejść do trybu stacjonarnego?",
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    if (polaczenieSieciowe_) {
        polaczenieSieciowe_->rozlacz();
    }

    uslugi_.ustawTrybPracy(ProstyUAR::TrybPracy::Stacjonarny);
    trybPracy_ = TrybPracy::Stacjonarny;

    wyzerujStanSieci();

    m_opisSesjiPolaczenia = "Tryb: Stacjonarny";
    m_ostatniStanPolaczenia = "Ręczne rozłączenie";
    odswiezTelemetryPolaczenia();
    ui_->lblStatusWydajnosci->setText("🟢 Symulacja wyrabia");
    ui_->lblStatusWydajnosci->setStyleSheet("color: green; font-weight: bold;");

    aktualizujDostepnoscKontrolek();
}

void MainWindow::onTcpZdarzenie(const QString& log)
{
    qDebug() << "[TCP]" << log;
}

void MainWindow::onTcpStanZmieniony(const QString& stan)
{
    m_ostatniStanPolaczenia = stan;
    odswiezTelemetryPolaczenia();
    aktualizujDostepnoscKontrolek();

    if (polaczenieSieciowe_ && polaczenieSieciowe_->czyPolaczono() &&
        trybPracy_ == TrybPracy::SieciowyRegulator) {
        wyslijBiezacaKonfiguracjeSieciowa();
    }
}

void MainWindow::onPolaczenieUtracone(const QString& powod)
{
    if (trybPracy_ != TrybPracy::Stacjonarny) {
        rozlaczSiecAWARYJNIE(powod);
    }
}

void MainWindow::onTcpRamkaOdebrana(const QByteArray& ramka)
{
    m_watchdogUzbrojonyPoPierwszejRamce = true;
    m_odebranoRamkeZWatchdoga = true;
    m_incydentyBrakRamki = 0;

    uslugi_.przetworzRamkeSieciowa(ramka);
}

void MainWindow::onTcpStatystyki(quint64 wyslane, quint64 odebrane)
{
    m_telemetriaWyslaneBajty = wyslane;
    m_telemetriaOdebraneBajty = odebrane;

    odswiezTelemetryPolaczenia();
}

void MainWindow::aktualizujOpoznienie(int32_t opoznienie)
{
    m_ostatnieOpoznienieSieci = opoznienie;

    if (std::abs(opoznienie) <= 2) {
        ui_->lblStatusWydajnosci->setText(QString("🟢 Symulacja wyrabia (Opóźń: %1)").arg(opoznienie));
        ui_->lblStatusWydajnosci->setStyleSheet("color: green; font-weight: bold;");
    } else {
        ui_->lblStatusWydajnosci->setText(QString("🔴 Symulacja NIE wyrabia (Opóźń: %1)").arg(opoznienie));
        ui_->lblStatusWydajnosci->setStyleSheet("color: red; font-weight: bold;");
    }
}

void MainWindow::zainicjujWatchdog()
{
    m_watchdogPolaczenia = new QTimer(this);
    m_watchdogPolaczenia->setInterval(1000);

    connect(m_watchdogPolaczenia, &QTimer::timeout, this, &MainWindow::onWatchdogPolaczenia);

    m_watchdogPolaczenia->start();
}

void MainWindow::wyslijBiezacaKonfiguracjeSieciowa()
{
    if (m_trwaZdalnaSynchronizacjaKonfiguracji)
        return;

    if (trybPracy_ == TrybPracy::Stacjonarny)
        return;

    if (!polaczenieSieciowe_ || !polaczenieSieciowe_->czyPolaczono())
        return;

    const KonfiguracjaUAR cfg = uslugi_.pobierzKonfiguracje();
    const auto ramka = ProtokolUAR::zbudujRamkeKonfiguracji(cfg);
    polaczenieSieciowe_->wyslijRamke(
        QByteArray(reinterpret_cast<const char*>(ramka.data()), static_cast<int>(ramka.size())));
}

void MainWindow::onWatchdogPolaczenia()
{
    if (trybPracy_ == TrybPracy::Stacjonarny) {
        return;
    }

    if (!polaczenieSieciowe_ || !polaczenieSieciowe_->czyPolaczono() || !uslugi_.czyUruchomiona()) {
        // Jeśli nie jesteśmy w pełni połączeni (np serwer czeka na klienta)
        m_odebranoRamkeZWatchdoga = true; 
        return;
    }

    if (!m_watchdogUzbrojonyPoPierwszejRamce) {
        // Nie rozlaczaj przed pierwsza ramka od drugiej strony
        m_odebranoRamkeZWatchdoga = true;
        return;
    }

    // 1. Sprawdzenie braku ramek
    if (!m_odebranoRamkeZWatchdoga) {
        m_incydentyBrakRamki++;
        qDebug() << "[WATCHDOG] Brak ramki w ostatniej sekundzie. Incydenty:" << m_incydentyBrakRamki;
    } else {
        m_incydentyBrakRamki = 0; 
    }
    m_odebranoRamkeZWatchdoga = false;

    // 2. Sprawdzenie opoznienia czasowego
    int opoznienieMS = std::abs(m_ostatnieOpoznienieSieci) * interwalMs_; // np. 100 * 200 = 20000ms
    if (opoznienieMS >= 3000) { // jesli opoznienie powyzej 3 sekund
        m_incydentyOpoznienia++;
        qDebug() << "[WATCHDOG] Opóźnienie wykracza poza 3s. Incydenty:" << m_incydentyOpoznienia;
    } else {
        m_incydentyOpoznienia = 0; // zeruj jesli sie unormowalo
    }

    // 3. Reakcja na incydenty
    if (m_incydentyBrakRamki > 10) {
        rozlaczSiecAWARYJNIE("Brak odpowiedzi od drugiej strony po 10 sekundach (Time-out).");
    } else if (m_incydentyBlednegoDekodowania >= 3) {
        rozlaczSiecAWARYJNIE("Zbyt wiele uszkodzonych/błędnych ramek (Błąd protokołu).");
    } else if (m_incydentyOpoznienia >= 5) {
        rozlaczSiecAWARYJNIE("Maksymalne dopuszczalne opóźnienie symulacji (pow. 3s) zostało przekroczone przez minimum 5 sekund.");
    }
}

void MainWindow::rozlaczSiecAWARYJNIE(const QString& powod)
{
    if (trybPracy_ == TrybPracy::Stacjonarny || m_trwaAwaryjneRozlaczenie) {
        return;
    }
    m_trwaAwaryjneRozlaczenie = true;

    qWarning() << "[WATCHDOG] AWARYJNE ROZŁĄCZENIE:" << powod;

    wyzerujStanSieci();
    
    if (polaczenieSieciowe_) {
        polaczenieSieciowe_->rozlacz();
    }
    
    uslugi_.ustawTrybPracy(ProstyUAR::TrybPracy::Stacjonarny);
    trybPracy_ = TrybPracy::Stacjonarny;
    aktualizujDostepnoscKontrolek();
    
    // Powiadom uzyszkodnika komunikatem / wyzeruj status
    m_opisSesjiPolaczenia = "Tryb: Stacjonarny";
    m_ostatniStanPolaczenia = "Awaryjne rozłączenie: " + powod;
    odswiezTelemetryPolaczenia();
    QMessageBox::warning(this, "Problem sieciowy", 
                         "Połączenie sieciowe zostało awaryjnie przerwane.\nPowód: " + powod + 
                         "\n\nProgram przeszedł samoczynnie w tryb lokalny.");
    m_trwaAwaryjneRozlaczenie = false;
}


void MainWindow::on_btnResetI_clicked()
{

}

