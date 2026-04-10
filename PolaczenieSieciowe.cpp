#include "PolaczenieSieciowe.h"
#include "ProtokolUAR.h"
#include <QDebug>
#include <cstring>
#include <limits>

PolaczenieSieciowe::PolaczenieSieciowe(QObject *parent)
    : QObject(parent), m_serwer(new QTcpServer(this)), m_gniazdo(nullptr),
      m_gniazdoUdp(new QUdpSocket(this)), m_timerBroadcast(new QTimer(this)),
      m_wyslaneBajty(0), m_odebraneBajty(0), m_aktywnyPortSerwera(0)
{
    m_gniazdoUdp->bind(QHostAddress::AnyIPv4,
                       45454,
                       QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    connect(m_serwer, &QTcpServer::newConnection, this, &PolaczenieSieciowe::onNowePolaczenie);
    connect(m_serwer, &QTcpServer::acceptError, this, &PolaczenieSieciowe::onBladSerwera);
    connect(m_gniazdoUdp, &QUdpSocket::readyRead, this, &PolaczenieSieciowe::onReadyReadUdp);
    connect(m_timerBroadcast, &QTimer::timeout, this, &PolaczenieSieciowe::onWyslijBroadcast);
}

PolaczenieSieciowe::~PolaczenieSieciowe()
{
    rozlacz(false);

    if (m_serwer->isListening()) {
        m_serwer->close();
    }
    if (m_gniazdoUdp) {
        m_gniazdoUdp->close();
    }
    if (m_timerBroadcast) {
        m_timerBroadcast->stop();
    }
}

bool PolaczenieSieciowe::czyPolaczono() const
{
    return m_gniazdo && m_gniazdo->state() == QAbstractSocket::ConnectedState;
}

void PolaczenieSieciowe::skonfigurujGniazdoTcp(QTcpSocket* gniazdo)
{
    if (!gniazdo)
        return;

    // KeepAlive pomaga wykryc "martwe" polaczenie przy dluzszych sesjach.
    gniazdo->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    // LowDelay (TCP_NODELAY) zmniejsza opoznienia dla malych ramek sterowania.
    gniazdo->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(gniazdo, &QTcpSocket::readyRead, this, &PolaczenieSieciowe::onReadyRead);
    connect(gniazdo, &QTcpSocket::disconnected, this, &PolaczenieSieciowe::onRozlaczono);
    connect(gniazdo, &QTcpSocket::errorOccurred, this, &PolaczenieSieciowe::onBladGniazda);
}

bool PolaczenieSieciowe::uruchomSerwer(uint16_t port)
{
    rozlacz(false);

    m_aktywnyPortSerwera = port;

    if (m_serwer->listen(QHostAddress::AnyIPv4, port)) {
        emit stanZmieniony("Serwer nasłuchuje na porcie " + QString::number(port));
        emit zdarzenieTCP("Serwer uruchomiony.");

        m_timerBroadcast->start(1000); // 1 sekunda
        
        return true;
    } else {
        emit stanZmieniony("Błąd uruchamiania serwera.");
        emit zdarzenieTCP("Błąd serwera: " + m_serwer->errorString());
        m_aktywnyPortSerwera = 0;
        
        return false;
    }
}

bool PolaczenieSieciowe::polaczZSerwerem(const QString& ip, uint16_t port, int timeoutMs)
{
    rozlacz(false);

    m_aktywnyPortSerwera = 0;
    m_timerBroadcast->stop();
    m_buforOdbioru.clear();

    m_gniazdo = new QTcpSocket(this);
    skonfigurujGniazdoTcp(m_gniazdo);

    m_gniazdo->connectToHost(ip, port);
    emit stanZmieniony("Łączenie z " + ip + ":" + QString::number(port));

    if (!m_gniazdo->waitForConnected(timeoutMs)) {
        const QString powod = m_gniazdo
                                  ? m_gniazdo->errorString()
                                  : QString("Przerwano próbę połączenia.");
        emit stanZmieniony("Nie udało się połączyć z serwerem.");
        emit zdarzenieTCP("Błąd łączenia: " + powod);

        if (m_gniazdo) {
            disconnect(m_gniazdo, nullptr, this, nullptr);
            m_gniazdo->abort();
            m_gniazdo->deleteLater();
            m_gniazdo = nullptr;
        }

        return false;
    }

    m_mialAktywnePolaczenie = true;
    m_wyslaneBajty = 0;
    m_odebraneBajty = 0;
    emit statystykiZaktualizowane(m_wyslaneBajty, m_odebraneBajty);

    QString peerIp = m_gniazdo->peerAddress().toString();
    const quint32 peerIpv4 = m_gniazdo->peerAddress().toIPv4Address();
    if (peerIpv4 != 0) {
        peerIp = QHostAddress(peerIpv4).toString();
    }

    emit stanZmieniony(QString("Połączono z %1:%2")
                           .arg(peerIp)
                           .arg(m_gniazdo->peerPort()));
    emit zdarzenieTCP("Nawiązano połączenie z serwerem.");
    return true;
}

void PolaczenieSieciowe::rozlacz(bool raportuj)
{
    m_mialAktywnePolaczenie = false;
    m_buforOdbioru.clear();

    if (m_gniazdo) {
        disconnect(m_gniazdo, nullptr, this, nullptr);
        m_gniazdo->abort();
        m_gniazdo->deleteLater();
        m_gniazdo = nullptr;
    }
    if (m_serwer->isListening()) {
        m_serwer->close();
    }
    if (m_timerBroadcast) {
        m_timerBroadcast->stop();
    }
    m_aktywnyPortSerwera = 0;

    if (raportuj) {
        emit stanZmieniony("Rozłączono.");
        emit zdarzenieTCP("Połączenie sieciowe zakończone.");
    }
}

void PolaczenieSieciowe::onWyslijBroadcast()
{
    if (m_aktywnyPortSerwera > 0) {
        // Ramka UDP tylko w celu identyfikacji (nie wpływa na normalny protokół TCP)
        QByteArray info = QString("SERWER_UAR:%1").arg(m_aktywnyPortSerwera).toUtf8();
        m_gniazdoUdp->writeDatagram(info, QHostAddress::Broadcast, 45454); // Stały port UDP
    }
}

void PolaczenieSieciowe::onReadyReadUdp()
{
    while (m_gniazdoUdp->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_gniazdoUdp->pendingDatagramSize());
        QHostAddress nadawcaIP;
        quint16 nadawcaPort;

        m_gniazdoUdp->readDatagram(datagram.data(), datagram.size(), &nadawcaIP, &nadawcaPort);
        Q_UNUSED(nadawcaPort);
        
        QString tekst = QString::fromUtf8(datagram);
        if (tekst.startsWith("SERWER_UAR:")) {
            QStringList czesci = tekst.split(":");
            if (czesci.size() == 2) {
                uint16_t portTCP = czesci[1].toUShort();

                QString ipDoWyswietlenia;
                if (nadawcaIP.protocol() == QAbstractSocket::IPv4Protocol) {
                    ipDoWyswietlenia = nadawcaIP.toString();
                } else {
                    const quint32 ipv4 = nadawcaIP.toIPv4Address();
                    if (ipv4 == 0) {
                        continue;
                    }
                    ipDoWyswietlenia = QHostAddress(ipv4).toString();
                }

                if (ipDoWyswietlenia.contains(':')) {
                    continue;
                }

                emit serwerZnaleziony(ipDoWyswietlenia, portTCP);
            }
        }
    }
}

void PolaczenieSieciowe::wyslijRamke(const QByteArray& ramka)
{
    if (m_gniazdo && m_gniazdo->state() == QAbstractSocket::ConnectedState) {
        qint64 zapisane = m_gniazdo->write(ramka);
        if (zapisane > 0) {
            m_wyslaneBajty += static_cast<quint64>(zapisane);
            emit statystykiZaktualizowane(m_wyslaneBajty, m_odebraneBajty);

            if (zapisane != ramka.size()) {
                emit zdarzenieTCP("Uwaga: do bufora TCP trafiła tylko część ramki.");
            }
        } else {
            emit zdarzenieTCP("Błąd wysyłania ramki!");
        }
    } else {
        emit zdarzenieTCP("Brak aktywnego połączenia TCP do wysłania danych!");
    }
}

void PolaczenieSieciowe::onReadyRead()
{
    if (!m_gniazdo) return;

    const QByteArray dane = m_gniazdo->readAll();
    if (dane.isEmpty()) {
        return;
    }

    m_odebraneBajty += static_cast<quint64>(dane.size());
    emit statystykiZaktualizowane(m_wyslaneBajty, m_odebraneBajty);

    m_buforOdbioru.append(dane);
    przetworzBuforOdbioru();
}

void PolaczenieSieciowe::przetworzBuforOdbioru()
{
    constexpr int rozmiarNaglowka = static_cast<int>(sizeof(Naglowek));
    constexpr quint32 maksRozmiarPayload = 1024u * 1024u; // 1 MB zapasu

    // TCP jest strumieniem bajtow: jedna ramka moze przyjsc w kawalkach albo wiele ramek naraz.
    // Dlatego skladamy pelne ramki z bufora na podstawie naglowka i dopiero je emitujemy dalej.
    while (true) {
        if (m_buforOdbioru.size() < rozmiarNaglowka) {
            return;
        }

        Naglowek naglowek{};
        std::memcpy(&naglowek, m_buforOdbioru.constData(), sizeof(Naglowek));

        if (naglowek.rozmiarDanych > maksRozmiarPayload) {
            emit zdarzenieTCP("Błąd protokołu: zbyt duży payload ramki.");
            rozlacz(false);
            emit polaczenieUtracone("Odrzucono połączenie: niepoprawny rozmiar ramki.");
            return;
        }

        const quint64 rozmiarRamki64 =
            static_cast<quint64>(rozmiarNaglowka) + static_cast<quint64>(naglowek.rozmiarDanych);

        if (rozmiarRamki64 > static_cast<quint64>(std::numeric_limits<int>::max())) {
            emit zdarzenieTCP("Błąd protokołu: rozmiar ramki przekracza limit aplikacji.");
            rozlacz(false);
            emit polaczenieUtracone("Odrzucono połączenie: ramka przekracza limit rozmiaru.");
            return;
        }

        const int rozmiarRamki = static_cast<int>(rozmiarRamki64);
        if (m_buforOdbioru.size() < rozmiarRamki) {
            return;
        }

        const QByteArray ramka = m_buforOdbioru.left(rozmiarRamki);
        m_buforOdbioru.remove(0, rozmiarRamki);
        emit ramkaOdebrana(ramka);
    }
}

void PolaczenieSieciowe::onNowePolaczenie()
{
    if (m_gniazdo) {
        disconnect(m_gniazdo, nullptr, this, nullptr);
        m_gniazdo->abort();
        m_gniazdo->deleteLater();
    }
    
    m_gniazdo = m_serwer->nextPendingConnection();
    skonfigurujGniazdoTcp(m_gniazdo);

    m_mialAktywnePolaczenie = true;
    m_buforOdbioru.clear();
    m_wyslaneBajty = 0;
    m_odebraneBajty = 0;
    emit statystykiZaktualizowane(m_wyslaneBajty, m_odebraneBajty);

    QString peerIp = m_gniazdo->peerAddress().toString();
    const quint32 ipv4 = m_gniazdo->peerAddress().toIPv4Address();
    if (ipv4 != 0) {
        peerIp = QHostAddress(ipv4).toString();
    }

    emit stanZmieniony(QString("Klient połączony: %1:%2")
                           .arg(peerIp)
                           .arg(m_gniazdo->peerPort()));
    emit zdarzenieTCP("Klient połączył się z serwerem.");
}

void PolaczenieSieciowe::onRozlaczono()
{
    const bool byloPolaczenie = m_mialAktywnePolaczenie;
    m_mialAktywnePolaczenie = false;
    m_buforOdbioru.clear();

    emit stanZmieniony("Rozłączono.");
    emit zdarzenieTCP("Połączenie TCP zostało przerwane.");

    if (byloPolaczenie) {
        emit polaczenieUtracone("Utracono połączenie z drugą instancją aplikacji.");
    }
}

void PolaczenieSieciowe::onBladSerwera(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit stanZmieniony("Błąd serwera.");
    emit zdarzenieTCP("Błąd serwera: " + m_serwer->errorString());
}

void PolaczenieSieciowe::onBladGniazda(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);

    const QString opisBledu = m_gniazdo ? m_gniazdo->errorString() : QString("Nieznany błąd gniazda");
    const bool byloPolaczenie = m_mialAktywnePolaczenie;

    emit stanZmieniony("Błąd gniazda.");
    emit zdarzenieTCP("Błąd gniazda: " + opisBledu);

    if (!byloPolaczenie) {
        // Podczas zestawiania połączenia cleanup wykonuje polaczZSerwerem po waitForConnected().
        // Dzięki temu nie zrywamy obiektu gniazda w trakcie oczekiwania i unikamy crasha.
        return;
    }

    rozlacz(false);

    if (byloPolaczenie) {
        emit polaczenieUtracone("Połączenie przerwane z powodu błędu gniazda: " + opisBledu);
    }
}
