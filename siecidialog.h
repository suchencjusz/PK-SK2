#ifndef SIECIDIALOG_H
#define SIECIDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui { class SiecDialog; }

class SiecDialog : public QDialog
{
    Q_OBJECT

public:
    enum class RolaInstancji { Regulator, Obiekt };
    enum class TrybPolaczenia { Klient, Serwer };

    explicit SiecDialog(QWidget *parent = nullptr);
    ~SiecDialog() override;

    RolaInstancji pobierzRole() const;
    TrybPolaczenia pobierzTrybPolaczenia() const;
    QString pobierzAdresIP() const;
    uint16_t pobierzPort() const;

private slots:
    void onZmienionoTryb(int index);

private:
    Ui::SiecDialog *ui;
};

#endif // SIECIDIALOG_H
