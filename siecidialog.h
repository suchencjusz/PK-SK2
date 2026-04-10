#ifndef SIECIDIALOG_H
#define SIECIDIALOG_H

#include <QDialog>
#include <QString>

class PolaczenieSieciowe;

namespace Ui { class SiecDialog; }

class SiecDialog : public QDialog
{
    Q_OBJECT

public:
    enum class RolaInstancji { Regulator, Obiekt };
    enum class TrybPolaczenia { Klient, Serwer };

    explicit SiecDialog(PolaczenieSieciowe* polaczenie, QWidget *parent = nullptr);
    ~SiecDialog() override;

    RolaInstancji pobierzRole() const;
    TrybPolaczenia pobierzTrybPolaczenia() const;
    QString pobierzAdresIP() const;
    uint16_t pobierzPort() const;

private slots:
    void onZmienionoTryb(int index);
    void onBtnPolaczClicked();
    void onSerwerZnaleziony(const QString& ip, uint16_t port);
    void onWybierzSerwerZListy(class QListWidgetItem* wpis);

private:
    Ui::SiecDialog *ui;
    PolaczenieSieciowe* m_polaczenie;
};

#endif // SIECIDIALOG_H
