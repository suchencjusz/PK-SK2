#include "ARXDialog.h"
#include "ui_ARXDialog.h"
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFontMetrics>
#include <QSpinBox>

#include <cmath>

namespace
{
    bool eq(double a, double b)
    {
        return std::abs(a - b) <= 1e-12;
    }
}

ARXDialog::ARXDialog(const KonfiguracjaUAR& cfg, QWidget* parent)
    : QDialog(parent)
    , ui_(new Ui::ARXDialog())
    , baseA_({ -1.6, 0.79, -0.12 })
    , baseB_({ 0.2, 0.1, 0.0 })
    , baseK_(cfg.k < 1 ? 1 : cfg.k)
    , baseSigma_(cfg.sigma < 0.0 ? 0.0 : cfg.sigma)
    , baseUMin_(cfg.uMin)
    , baseUMax_(cfg.uMax)
    , baseYMin_(cfg.yMin)
    , baseYMax_(cfg.yMax)
    , baseOgrU_(cfg.ograniczU)
    , baseOgrY_(cfg.ograniczY)
{
    ui_->setupUi(this);
    adjustSize();

    spinA1_ = ui_->spinA1;
    spinA2_ = ui_->spinA2;
    spinA3_ = ui_->spinA3;
    spinB1_ = ui_->spinB1;
    spinB2_ = ui_->spinB2;
    spinB3_ = ui_->spinB3;
    sbK_    = ui_->sbK;
    sbSig_  = ui_->sbSig;

    chkOgraniczU_ = ui_->chkOgraniczU;
    spinMinU_ = ui_->spinMinU;
    spinMaxU_ = ui_->spinMaxU;

    chkOgraniczY_ = ui_->chkOgraniczY;
    spinMinY_ = ui_->spinMinY;
    spinMaxY_ = ui_->spinMaxY;

    if (cfg.A.size() >= 3) {
        baseA_[0] = cfg.A[0];
        baseA_[1] = cfg.A[1];
        baseA_[2] = cfg.A[2];
    } else {
        for (size_t i = 0; i < cfg.A.size() && i < 3; ++i)
            baseA_[i] = cfg.A[i];
    }

    if (cfg.B.size() >= 3) {
        baseB_[0] = cfg.B[0];
        baseB_[1] = cfg.B[1];
        baseB_[2] = cfg.B[2];
    } else {
        for (size_t i = 0; i < cfg.B.size() && i < 3; ++i)
            baseB_[i] = cfg.B[i];
    }

    spinA1_->setValue(baseA_[0]);
    spinA2_->setValue(baseA_[1]);
    spinA3_->setValue(baseA_[2]);
    spinB1_->setValue(baseB_[0]);
    spinB2_->setValue(baseB_[1]);
    spinB3_->setValue(baseB_[2]);
    sbK_->setValue(baseK_);
    sbSig_->setValue(baseSigma_);

    chkOgraniczU_->setChecked(baseOgrU_);
    chkOgraniczY_->setChecked(baseOgrY_);

    spinMinU_->setValue((baseUMin_ < -1000.0) ? -10.0 : baseUMin_);
    spinMaxU_->setValue((baseUMax_ > 1000.0) ? 10.0 : baseUMax_);
    spinMinY_->setValue((baseYMin_ < -1000.0) ? -10.0 : baseYMin_);
    spinMaxY_->setValue((baseYMax_ > 1000.0) ? 10.0 : baseYMax_);

    ui_->buttonBox->button(QDialogButtonBox::Ok)->setText("Zatwierdź");
    ui_->buttonBox->button(QDialogButtonBox::Cancel)->setText("Anuluj");

    connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    const QFontMetrics fm(font());
    const int ctrlHeight = fm.height() + 12;
    const int fieldWidth = 150;

    spinA1_->setMinimumHeight(ctrlHeight);
    spinA2_->setMinimumHeight(ctrlHeight);
    spinA3_->setMinimumHeight(ctrlHeight);
    spinB1_->setMinimumHeight(ctrlHeight);
    spinB2_->setMinimumHeight(ctrlHeight);
    spinB3_->setMinimumHeight(ctrlHeight);
    sbK_->setMinimumHeight(ctrlHeight);
    sbSig_->setMinimumHeight(ctrlHeight);
    spinMinU_->setMinimumHeight(ctrlHeight);
    spinMaxU_->setMinimumHeight(ctrlHeight);
    spinMinY_->setMinimumHeight(ctrlHeight);
    spinMaxY_->setMinimumHeight(ctrlHeight);

    sbK_->setFixedWidth(fieldWidth);
    sbSig_->setFixedWidth(fieldWidth);
    spinMinU_->setFixedWidth(fieldWidth);
    spinMaxU_->setFixedWidth(fieldWidth);
    spinMinY_->setFixedWidth(fieldWidth);
    spinMaxY_->setFixedWidth(fieldWidth);
}

ARXDialog::~ARXDialog()
{
    delete ui_;
}

// Wykrywanie zmian
bool ARXDialog::modelChanged() const
{
    const std::vector<double> newA = pobierzA();
    const std::vector<double> newB = pobierzB();
    const bool aChanged = !eq(newA[0], baseA_[0]) || !eq(newA[1], baseA_[1]) || !eq(newA[2], baseA_[2]);
    const bool bChanged = !eq(newB[0], baseB_[0]) || !eq(newB[1], baseB_[1]) || !eq(newB[2], baseB_[2]);
    const bool kChanged = pobierzK() != baseK_;
    const bool sigmaChanged = !eq(pobierzSigma(), baseSigma_);
    return aChanged || bChanged || kChanged || sigmaChanged;
}

bool ARXDialog::limitsChanged() const
{
    return !eq(pobierzUMin(), baseUMin_) ||
           !eq(pobierzUMax(), baseUMax_) ||
           !eq(pobierzYMin(), baseYMin_) ||
           !eq(pobierzYMax(), baseYMax_);
}

bool ARXDialog::flagsChanged() const
{
    return pobierzOgraniczU() != baseOgrU_ ||
           pobierzOgraniczY() != baseOgrY_;
}

// Gettery
std::vector<double> ARXDialog::pobierzA() const
{
    return { spinA1_->value(), spinA2_->value(), spinA3_->value() };
}

std::vector<double> ARXDialog::pobierzB() const
{
    return { spinB1_->value(), spinB2_->value(), spinB3_->value() };
}

int ARXDialog::pobierzK() const
{
    return sbK_->value();
}

double ARXDialog::pobierzSigma() const
{
    return sbSig_->value();
}

double ARXDialog::pobierzUMin() const
{
    const double minU = spinMinU_->value();
    const double maxU = spinMaxU_->value();
    return (minU <= maxU) ? minU : maxU;
}

double ARXDialog::pobierzUMax() const
{
    const double minU = spinMinU_->value();
    const double maxU = spinMaxU_->value();
    return (minU <= maxU) ? maxU : minU;
}

double ARXDialog::pobierzYMin() const
{
    const double minY = spinMinY_->value();
    const double maxY = spinMaxY_->value();
    return (minY <= maxY) ? minY : maxY;
}

double ARXDialog::pobierzYMax() const
{
    const double minY = spinMinY_->value();
    const double maxY = spinMaxY_->value();
    return (minY <= maxY) ? maxY : minY;
}

bool ARXDialog::pobierzOgraniczU() const
{
    return chkOgraniczU_->isChecked();
}

bool ARXDialog::pobierzOgraniczY() const
{
    return chkOgraniczY_->isChecked();
}

