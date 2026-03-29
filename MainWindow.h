#pragma once
#include <QMainWindow>
#include "UslugiUAR.h"
#include "qcustomplot.h"

namespace Ui { class MainWindow; }
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QLCDNumber;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    enum class TrybPracy{
        Stacjonarny,
        SieciowyRegulator,
        SieciowyObiekt
    };

private slots:
    // Akcje przyciskow
    void onBtnStart();
    void onBtnStop();
    void onBtnReset();
    void onBtnZapisz();
    void onBtnWczytaj();

    // Okna / aktualizacje
    void otworzOknoParametrowARX();
    void aktualizujParametryOnline();

    void on_btnTrybSieciowy_clicked();

private:
    // Budowa UI / wykresow
    void budujInterfejs();
    void konfigurujWykresy();

    // Synchronizacja / odswiezanie
    void odswiezGUIzKonfiguracji();
    void aktualizujWyswietlaczeNumeryczne(const ProbkaUAR& p);
    void wyczyscWykresy();
    void onProbka(const ProbkaUAR& p);
    void wyzerujWyswietlacze();

private:
    // Backend / sterowanie
    Ui::MainWindow* ui_ = nullptr;
    UslugiUAR uslugi_;

    int oknoObserwacjiS_ = 10;
    int interwalMs_ = 200; // cache TTms do osi X (margines max 3 probki)

    // Wykresy
    QCustomPlot* plotWY_  = nullptr;
    QCustomPlot* plotE_   = nullptr;
    QCustomPlot* plotU_   = nullptr;
    QCustomPlot* plotPID_ = nullptr;

    // LCD
    QLCDNumber* lcdW_  = nullptr;
    QLCDNumber* lcdY_  = nullptr;
    QLCDNumber* lcdE_  = nullptr;
    QLCDNumber* lcdU_  = nullptr;
    QLCDNumber* lcdUP_ = nullptr;
    QLCDNumber* lcdUI_ = nullptr;
    QLCDNumber* lcdUD_ = nullptr;

    // Symulacja
    QSpinBox* spinInterwal_ = nullptr;
    QSpinBox* spinOkno_     = nullptr;

    // Generator
    QComboBox*       comboGenTyp_        = nullptr;
    QDoubleSpinBox*  spinGenAmpl_        = nullptr;
    QDoubleSpinBox*  spinGenOkres_       = nullptr;
    QDoubleSpinBox*  spinGenWypelnienie_ = nullptr;
    QDoubleSpinBox*  spinGenOffset_      = nullptr;

    // PID
    QDoubleSpinBox* spinKp_       = nullptr;
    QDoubleSpinBox* spinTi_       = nullptr;
    QDoubleSpinBox* spinTd_       = nullptr;
    QComboBox*      comboTrybCalk_ = nullptr;

    //sieci
    void aktualizujDostepnoscKontrolek();
    void ustawStatusWydajnosci(bool wyrabiaSie);

    TrybPracy trybPracy_ = TrybPracy::Stacjonarny;
};

