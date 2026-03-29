#include "siecidialog.h"
#include "ui_siecidialog.h"

SiecDialog::SiecDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SiecDialog)
{
    ui->setupUi(this);
    setWindowTitle("Konfiguracja polaczenie sieciowego");

    //QOverload -> w klasie QComboBox są dwie funkcje currentIndexChanged (jedna int druga string). Teraz kompilatorowi mówimy, że chcemy tą z inta
    //https://doc.qt.io/qt-6/qoverload.html#qOverload
    connect(ui->comboTryb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SiecDialog::onZmienionoTryb);

    connect(ui->btnPolacz, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->btnAnuluj, &QPushButton::clicked, this, &QDialog::reject);

    onZmienionoTryb(ui->comboTryb->currentIndex());
}

SiecDialog::~SiecDialog()
{
    delete ui;
}

void SiecDialog::onZmienionoTryb(int index)
{
    bool toSerwer = (index == 1);
    ui->editIP->setEnabled(!toSerwer);
}

SiecDialog::RolaInstancji SiecDialog::pobierzRole() const
{
    return (ui->comboInstancja->currentIndex() == 0)
    ? RolaInstancji::Regulator
    : RolaInstancji::Obiekt;
}

SiecDialog::TrybPolaczenia SiecDialog::pobierzTrybPolaczenia() const
{
    return (ui->comboTryb->currentIndex() == 0)
    ? TrybPolaczenia::Klient
    : TrybPolaczenia::Serwer;
}

QString SiecDialog::pobierzAdresIP() const
{
    return ui->editIP->text();
}

uint16_t SiecDialog::pobierzPort() const
{
    return static_cast<uint16_t>(ui->spinPort->value());
}
