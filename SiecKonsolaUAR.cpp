#include "SiecKonsolaUAR.h"
#include "ProtokolUAR.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QElapsedTimer>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>

static std::vector<uint8_t> zbudujPrzykladowaRamke(int wybor)
{
    switch (wybor)
    {
    case 1:
    {
        KonfiguracjaUAR cfg;
        cfg.kP = 2.5;
        cfg.Ti = 4.0;
        cfg.Td = 0.2;
        cfg.symulacjaTTms = 200;
        return ProtokolUAR::zbudujRamkeKonfiguracji(cfg);
    }
    case 2:
    {
        PakietProbkiSymulacji p;
        p.krok = 10;
        p.t = 2.0;
        p.w = 1.2;
        p.y = 0.9;
        return ProtokolUAR::zbudujRamkeProbkiSymulacji(p);
    }
    case 3:
    {
        PakietSterowania s;
        s.krok = 10;
        s.u = 0.55;
        s.uP = 0.40;
        s.uI = 0.13;
        s.uD = 0.02;
        return ProtokolUAR::zbudujRamkeSterowania(s);
    }
    default:
        return {};
    }
}

static void wypiszWiadomosc(const OdebranaWiadomosc& wiadomosc)
{
    switch (wiadomosc.typ)
    {
    case TypWiadomosci::Konfiguracja:
    {
        const KonfiguracjaUAR& cfg = std::get<KonfiguracjaUAR>(wiadomosc.payload);
        std::cout << "[ODEBRANO] KONFIGURACJA: kP=" << cfg.kP
                  << ", Ti=" << cfg.Ti
                  << ", Td=" << cfg.Td
                  << ", TTms=" << cfg.symulacjaTTms << "\n";
        break;
    }
    case TypWiadomosci::ProbkaSymulacji:
    {
        const PakietProbkiSymulacji& p = std::get<PakietProbkiSymulacji>(wiadomosc.payload);
        std::cout << "[ODEBRANO] PROBKA: krok=" << p.krok
                  << ", t=" << p.t
                  << ", w=" << p.w
                  << ", y=" << p.y << "\n";
        break;
    }
    case TypWiadomosci::Sterowanie:
    {
        const PakietSterowania& s = std::get<PakietSterowania>(wiadomosc.payload);
        std::cout << "[ODEBRANO] STEROWANIE: krok=" << s.krok
                  << ", u=" << s.u
                  << " (uP=" << s.uP
                  << ", uI=" << s.uI
                  << ", uD=" << s.uD << ")\n";
        break;
    }
    default:
        std::cout << "[ODEBRANO] Nieznany typ wiadomosci\n";
        break;
    }
}

static bool odbierzPelnaRamke(QTcpSocket* socket,
    std::vector<uint8_t>& ramka,
    int timeoutMs,
    std::string& blad)
{
    if (!socket)
    {
        blad = "Brak socketu.";
        return false;
    }

    QByteArray bufor;
    bufor.reserve(1024);

    QElapsedTimer zegar;
    zegar.start();

    while (zegar.elapsed() < timeoutMs)
    {
        if (socket->bytesAvailable() > 0)
        {
            bufor.append(socket->readAll());
        }
        else if (!socket->waitForReadyRead(timeoutMs - static_cast<int>(zegar.elapsed())))
        {
            break;
        }

        if (bufor.size() < static_cast<int>(sizeof(Naglowek)))
            continue;

        Naglowek naglowek;
        std::memcpy(&naglowek, bufor.constData(), sizeof(Naglowek));

        const std::size_t pelnyRozmiar =
            sizeof(Naglowek) + static_cast<std::size_t>(naglowek.rozmiarDanych);

        if (bufor.size() < static_cast<int>(pelnyRozmiar))
            continue;

        ramka.assign(reinterpret_cast<const uint8_t*>(bufor.constData()),
            reinterpret_cast<const uint8_t*>(bufor.constData()) + pelnyRozmiar);
        return true;
    }

    blad = "Timeout podczas odbioru pelnej ramki.";
    return false;
}

static bool wyslijRamke(QTcpSocket* socket, const std::vector<uint8_t>& ramka)
{
    if (!socket || ramka.empty())
        return false;

    const char* data = reinterpret_cast<const char*>(ramka.data());
    const qint64 expected = static_cast<qint64>(ramka.size());
    const qint64 sent = socket->write(data, expected);
    if (sent != expected)
        return false;

    return socket->waitForBytesWritten(3000);
}


void SiecKonsolaUAR::uruchom()
{
    std::cout << "Tryb sieciowy CLI (QTcpSocket)\n";
    std::cout << "1. Serwer\n";
    std::cout << "2. Klient\n";
    std::cout << "> Wybierz role (1/2): ";

    int rola = 0;
    std::cin >> rola;
    if (!std::cin || (rola != 1 && rola != 2))
    {
        std::cout << "Niepoprawny wybor roli.\n";
        return;
    }

    int port = 0;
    std::cout << "> Podaj port: ";
    std::cin >> port;
    if (!std::cin || port <= 0 || port > 65535)
    {
        std::cout << "Niepoprawny port.\n";
        return;
    }

    QTcpSocket* socket = nullptr;
    QTcpServer serwer;
    QTcpSocket klient;

    if (rola == 1)
    {
        if (!serwer.listen(QHostAddress::Any, static_cast<quint16>(port)))
        {
            std::cout << "Nie mozna uruchomic serwera: " << serwer.errorString().toStdString() << "\n";
            return;
        }

        std::cout << "[SERWER] Nasluchiwanie na porcie " << port << "...\n";
        if (!serwer.waitForNewConnection(30000))
        {
            std::cout << "Timeout oczekiwania na polaczenie.\n";
            return;
        }

        socket = serwer.nextPendingConnection();
        std::cout << "[SERWER] Polaczono z "
                  << socket->peerAddress().toString().toStdString()
                  << ":" << socket->peerPort() << "\n";
    }
    else
    {
        std::string host;
        std::cout << "> Podaj IP serwera (np. 127.0.0.1): ";
        std::cin >> host;
        if (!std::cin || host.empty())
        {
            std::cout << "Niepoprawny adres hosta.\n";
            return;
        }

        klient.connectToHost(QString::fromStdString(host), static_cast<quint16>(port));
        if (!klient.waitForConnected(10000))
        {
            std::cout << "Nie mozna polaczyc: " << klient.errorString().toStdString() << "\n";
            return;
        }

        socket = &klient;
        std::cout << "[KLIENT] Polaczono z serwerem.\n";
    }

    std::cout << "\nSesja aktywna. Dzialanie w petli do wyboru opcji 0.\n";

    while (true)
    {
        if (!socket || socket->state() != QAbstractSocket::ConnectedState)
        {
            std::cout << "Polaczenie zostalo zamkniete.\n";
            return;
        }

        std::cout << "1. Wyslij KONFIGURACJA\n";
        std::cout << "2. Wyslij PROBKA_SYMULACJI\n";
        std::cout << "3. Wyslij STEROWANIE\n";
        std::cout << "4. Odbierz jedna ramke (timeout 30 s)\n";
        std::cout << "0. Zakoncz\n";
        std::cout << "> Wybor: ";

        int wybor = -1;
        std::cin >> wybor;
        if (!std::cin)
        {
            std::cout << "Blad odczytu z konsoli.\n";
            return;
        }

        if (wybor == 0)
        {
            std::cout << "Koniec sesji sieciowej.\n";
            return;
        }

        if (wybor >= 1 && wybor <= 3)
        {
            const std::vector<uint8_t> ramka = zbudujPrzykladowaRamke(wybor);
            if (ramka.empty())
            {
                std::cout << "Nie udalo sie zbudowac ramki.\n";
                continue;
            }

            if (!wyslijRamke(socket, ramka))
            {
                std::cout << "Blad wysylki ramki.\n";
                return;
            }

            std::cout << "[INFO] Wyslano " << ramka.size() << " B.\n";
            continue;
        }

        if (wybor == 4)
        {
            std::vector<uint8_t> odebrana;
            std::string blad;

            if (!odbierzPelnaRamke(socket, odebrana, 30000, blad))
            {
                std::cout << "Blad odbioru: " << blad << "\n";
                continue;
            }

            OdebranaWiadomosc wiadomosc;
            if (!ProtokolUAR::dekodujRamke(odebrana, wiadomosc, &blad))
            {
                std::cout << "Blad dekodowania: " << blad << "\n";
                continue;
            }

            wypiszWiadomosc(wiadomosc);
            continue;
        }

        std::cout << "Nieznana opcja.\n";
    }
}
