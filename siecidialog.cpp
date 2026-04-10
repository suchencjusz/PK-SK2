#include "siecidialog.h"
#include "ui_siecidialog.h"
#include "PolaczenieSieciowe.h"
#include <QSettings>
#include <QHostAddress>
#include <QMessageBox>

SiecDialog::SiecDialog(PolaczenieSieciowe* polaczenie, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SiecDialog),
    m_polaczenie(polaczenie)
{
    ui->setupUi(this);
    setWindowTitle("Konfiguracja polaczenia sieciowego");

    ui->listWidget->clear();

    QSettings settings("Politechnika", "ProjektUAR");
    ui->comboInstancja->setCurrentIndex(settings.value("Siec/Instancja", 0).toInt());
    ui->comboTryb->setCurrentIndex(settings.value("Siec/Tryb", 0).toInt());
    ui->editIP->setText(settings.value("Siec/IP", "127.0.0.1").toString());
    ui->spinPort->setValue(settings.value("Siec/Port", 5555).toInt());
    ui->spinPortSerwer->setValue(settings.value("Siec/PortSerwera", 5555).toInt());

    //QOverload -> w klasie QComboBox są dwie funkcje currentIndexChanged (jedna int druga string). Teraz kompilatorowi mówimy, że chcemy tą z inta
    //https://doc.qt.io/qt-6/qoverload.html#qOverload
    connect(ui->comboTryb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SiecDialog::onZmienionoTryb);

    connect(ui->btnPolacz, &QPushButton::clicked, this, &SiecDialog::onBtnPolaczClicked);
    connect(ui->btnAnuluj, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->listWidget, &QListWidget::itemClicked, this, &SiecDialog::onWybierzSerwerZListy);

    if (m_polaczenie) {
        connect(m_polaczenie, &PolaczenieSieciowe::serwerZnaleziony, this, &SiecDialog::onSerwerZnaleziony);
    }

    onZmienionoTryb(ui->comboTryb->currentIndex());
}

void SiecDialog::onSerwerZnaleziony(const QString& ip, uint16_t port)
{
    if (ip.contains(':')) {
        return; // fix na ipv6
    }

    QString info = QString("%1:%2").arg(ip).arg(port);

    auto items = ui->listWidget->findItems(info, Qt::MatchExactly);
    if (items.isEmpty()) {
        ui->listWidget->addItem(info);
    }
}

void SiecDialog::onWybierzSerwerZListy(QListWidgetItem* wpis)
{
    if (!wpis) return;
    
    QString tekst = wpis->text();
    int lastColon = tekst.lastIndexOf(':');
    if (lastColon != -1) {
        QString ip = tekst.left(lastColon);
        QString port = tekst.mid(lastColon + 1);
        ui->editIP->setText(ip);
        ui->spinPort->setValue(port.toInt());
    }
}

SiecDialog::~SiecDialog()
{
    delete ui;
}

void SiecDialog::onBtnPolaczClicked()
{
    const QString ip = pobierzAdresIP().trimmed();

    if (pobierzTrybPolaczenia() == TrybPolaczenia::Klient) {
        QHostAddress addr;
        if (!addr.setAddress(ip) || addr.protocol() != QAbstractSocket::IPv4Protocol) {
            QMessageBox::warning(this,
                                 "Niepoprawny adres",
                                 "Podaj poprawny adres IPv4 (np. 127.0.0.1 albo 192.168.1.10).");
            return;
        }
    }

    QSettings settings("Politechnika", "ProjektUAR");
    settings.setValue("Siec/Instancja", ui->comboInstancja->currentIndex());
    settings.setValue("Siec/Tryb", ui->comboTryb->currentIndex());
    settings.setValue("Siec/IP", ip);
    settings.setValue("Siec/Port", ui->spinPort->value());
    settings.setValue("Siec/PortSerwera", ui->spinPortSerwer->value());

    bool sukces = true;
    if (m_polaczenie) {
        auto tryb = pobierzTrybPolaczenia();

        if (tryb == TrybPolaczenia::Serwer) {
            sukces = m_polaczenie->uruchomSerwer(static_cast<uint16_t>(ui->spinPortSerwer->value()));
        } else {
            sukces = m_polaczenie->polaczZSerwerem(ip, pobierzPort());
        }

        if (!sukces) {
            QMessageBox::warning(this,
                                 "Błąd połączenia",
                                 "Nie udało się nawiązać połączenia. Sprawdź IP/port i spróbuj ponownie.");
            return;
        }
    }

    accept();
}

void SiecDialog::onZmienionoTryb(int index)
{
    bool toSerwer = (index == 1);

    ui->grpSerwer->setEnabled(toSerwer);
    ui->grpPolaczenie->setEnabled(!toSerwer);
    ui->groupBox_2->setEnabled(!toSerwer);

    ui->grpSerwer->setVisible(toSerwer);
    ui->grpPolaczenie->setVisible(!toSerwer);
    ui->groupBox_2->setVisible(!toSerwer);
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
    if (pobierzTrybPolaczenia() == TrybPolaczenia::Serwer) {
        return static_cast<uint16_t>(ui->spinPortSerwer->value());
    }
    return static_cast<uint16_t>(ui->spinPort->value());
}
