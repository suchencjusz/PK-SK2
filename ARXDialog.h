#pragma once
#include <QDialog>
#include <vector>
#include "KonfiguracjaUAR.h"

namespace Ui { class ARXDialog; }
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;

class ARXDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ARXDialog(const KonfiguracjaUAR& cfg, QWidget* parent = nullptr);
    ~ARXDialog() override;

    // Flagi zmian
    bool modelChanged() const;
    bool limitsChanged() const;
    bool flagsChanged() const;

    // Parametry modelu
    std::vector<double> pobierzA() const;
    std::vector<double> pobierzB() const;
    int pobierzK() const;
    double pobierzSigma() const;

    // Limity / flagi
    double pobierzUMin() const;
    double pobierzUMax() const;
    double pobierzYMin() const;
    double pobierzYMax() const;
    bool pobierzOgraniczU() const;
    bool pobierzOgraniczY() const;

private:
    // UI
    Ui::ARXDialog* ui_ = nullptr;

    std::vector<double> baseA_;
    std::vector<double> baseB_;
    int baseK_ = 1;
    double baseSigma_ = 0.0;

    double baseUMin_ = -10.0;
    double baseUMax_ = 10.0;
    double baseYMin_ = -10.0;
    double baseYMax_ = 10.0;
    bool baseOgrU_ = true;
    bool baseOgrY_ = true;

    QDoubleSpinBox* spinA1_ = nullptr;
    QDoubleSpinBox* spinA2_ = nullptr;
    QDoubleSpinBox* spinA3_ = nullptr;

    QDoubleSpinBox* spinB1_ = nullptr;
    QDoubleSpinBox* spinB2_ = nullptr;
    QDoubleSpinBox* spinB3_ = nullptr;

    QSpinBox* sbK_ = nullptr;
    QDoubleSpinBox* sbSig_ = nullptr;

    QCheckBox* chkOgraniczU_ = nullptr;
    QDoubleSpinBox* spinMinU_ = nullptr;
    QDoubleSpinBox* spinMaxU_ = nullptr;

    QCheckBox* chkOgraniczY_ = nullptr;
    QDoubleSpinBox* spinMinY_ = nullptr;
    QDoubleSpinBox* spinMaxY_ = nullptr;
};

