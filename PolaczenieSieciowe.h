#ifndef POLACZENIESIECIOWE_H
#define POLACZENIESIECIOWE_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QByteArray>

class PolaczenieSieciowe : public QObject
{
    Q_OBJECT
public:
    explicit PolaczenieSieciowe(QObject *parent = nullptr);
    ~PolaczenieSieciowe();

    bool uruchomSerwer(uint16_t port);
    bool polaczZSerwerem(const QString& ip, uint16_t port, int timeoutMs = 3000);
    void wyslijRamke(const QByteArray& ramka);
    void rozlacz(bool raportuj = true);
    bool czyPolaczono() const;

signals:
    void serwerZnaleziony(const QString& ip, uint16_t port);
    void statystykiZaktualizowane(quint64 wyslane, quint64 odebrane);
    void ramkaOdebrana(const QByteArray& ramka);
    void stanZmieniony(const QString& stan);
    void zdarzenieTCP(const QString& log);
    void polaczenieUtracone(const QString& powod);

private slots:
    void onReadyRead();
    void onNowePolaczenie();
    void onRozlaczono();
    void onBladSerwera(QAbstractSocket::SocketError socketError);
    void onBladGniazda(QAbstractSocket::SocketError socketError);
    void onWyslijBroadcast();
    void onReadyReadUdp();

private:
    void skonfigurujGniazdoTcp(QTcpSocket* gniazdo);
    void przetworzBuforOdbioru();

    QTcpServer* m_serwer;
    QTcpSocket* m_gniazdo;
    QUdpSocket* m_gniazdoUdp;
    QTimer* m_timerBroadcast;

    quint64 m_wyslaneBajty;
    quint64 m_odebraneBajty;
    
    uint16_t m_aktywnyPortSerwera = 0;
    QByteArray m_buforOdbioru;
    bool m_mialAktywnePolaczenie = false;
};

#endif // POLACZENIESIECIOWE_H
