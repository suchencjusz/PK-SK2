#include "ProtokolUAR.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <stdexcept>

void ProtokolUAR::ustawBlad(std::string *blad, const std::string &tresc)
{
    if (blad)
        *blad = tresc;
}

std::vector<uint8_t> ProtokolUAR::zbudujRamkeWewnetrzna(TypWiadomosci typ, const std::string &payload)
{
    Naglowek naglowek;
    naglowek.typ = typ;
    naglowek.rozmiarDanych = static_cast<uint32_t>(payload.size());

    std::vector<uint8_t> ramka;
    ramka.reserve(sizeof(Naglowek) + payload.size());

    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&naglowek);
    ramka.insert(ramka.end(), ptr, ptr + sizeof(Naglowek));
    ramka.insert(ramka.end(), payload.begin(), payload.end());

    return ramka;
}

bool ProtokolUAR::odczytajNaglowek(const std::vector<uint8_t> &ramka,
                                   Naglowek &naglowek,
                                   std::string *blad)
{
    if (ramka.size() < sizeof(Naglowek))
    {
        ustawBlad(blad, "Ramka krotsza niz naglowek.");
        return false;
    }

    std::memcpy(&naglowek, ramka.data(), sizeof(Naglowek));

    const std::size_t oczekiwanyRozmiar =
        sizeof(Naglowek) + static_cast<std::size_t>(naglowek.rozmiarDanych);

    if (ramka.size() != oczekiwanyRozmiar)
    {
        ustawBlad(blad, "Niepoprawny rozmiar ramki wzgledem naglowka.");
        return false;
    }

    return true;
}

bool ProtokolUAR::odczytajPayload(const std::vector<uint8_t> &ramka,
                                  TypWiadomosci oczekiwanyTyp,
                                  std::string &payload,
                                  std::string *blad)
{
    Naglowek naglowek;
    if (!odczytajNaglowek(ramka, naglowek, blad))
        return false;

    if (naglowek.typ != oczekiwanyTyp)
    {
        ustawBlad(blad, "Niepoprawny typ wiadomosci w ramce.");
        return false;
    }

    const char *dataPtr = reinterpret_cast<const char *>(ramka.data() + sizeof(Naglowek));
    payload.assign(dataPtr, dataPtr + naglowek.rozmiarDanych);
    return true;
}

std::string ProtokolUAR::doHex(const std::vector<uint8_t> &data)
{
    std::stringstream ss;
    for (uint8_t b : data)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return ss.str();
}

std::vector<uint8_t> ProtokolUAR::zHex(const std::string &hex)
{
    std::vector<uint8_t> bytes;
    for (std::size_t i = 0; i < hex.length(); i += 2)
    {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::vector<uint8_t> ProtokolUAR::zbudujRamkeKonfiguracji(const KonfiguracjaUAR &cfg)
{
    return zbudujRamkeWewnetrzna(TypWiadomosci::Konfiguracja, cfg.serializujDoJSON());
}

bool ProtokolUAR::odczytajRamkeKonfiguracji(const std::vector<uint8_t> &ramka,
                                            KonfiguracjaUAR &cfg,
                                            std::string *blad)
{
    std::string payload;
    if (!odczytajPayload(ramka, TypWiadomosci::Konfiguracja, payload, blad))
        return false;

    try
    {
        cfg = KonfiguracjaUAR::wczytajZJSON(payload);
        return true;
    }
    catch (const std::exception &e)
    {
        ustawBlad(blad, std::string("Blad parsowania konfiguracji: ") + e.what());
        return false;
    }
}

std::vector<uint8_t> ProtokolUAR::zbudujRamkeProbkiSymulacji(const PakietProbkiSymulacji &probka)
{
    std::string payload(sizeof(PakietProbkiSymulacji), '\0');
    std::memcpy(payload.data(), &probka, sizeof(PakietProbkiSymulacji));

    return zbudujRamkeWewnetrzna(TypWiadomosci::ProbkaSymulacji, payload);
}

bool ProtokolUAR::odczytajRamkeProbkiSymulacji(const std::vector<uint8_t> &ramka,
                                               PakietProbkiSymulacji &probka,
                                               std::string *blad)
{
    std::string payload;
    if (!odczytajPayload(ramka, TypWiadomosci::ProbkaSymulacji, payload, blad))
        return false;

    if (payload.size() != sizeof(PakietProbkiSymulacji))
    {
        ustawBlad(blad, "Niepoprawny rozmiar payloadu probki symulacji.");
        return false;
    }

    std::memcpy(&probka, payload.data(), sizeof(PakietProbkiSymulacji));
    return true;
}

std::vector<uint8_t> ProtokolUAR::zbudujRamkeSterowania(const PakietSterowania &sterowanie)
{
    std::string payload(sizeof(PakietSterowania), '\0');
    std::memcpy(payload.data(), &sterowanie, sizeof(PakietSterowania));

    return zbudujRamkeWewnetrzna(TypWiadomosci::Sterowanie, payload);
}

bool ProtokolUAR::odczytajRamkeSterowania(const std::vector<uint8_t> &ramka,
                                          PakietSterowania &sterowanie,
                                          std::string *blad)
{
    std::string payload;
    if (!odczytajPayload(ramka, TypWiadomosci::Sterowanie, payload, blad))
        return false;

    if (payload.size() != sizeof(PakietSterowania))
    {
        ustawBlad(blad, "Niepoprawny rozmiar payloadu pakietu sterowania.");
        return false;
    }

    std::memcpy(&sterowanie, payload.data(), sizeof(PakietSterowania));
    return true;
}

bool ProtokolUAR::odczytajTypWiadomosci(const std::vector<uint8_t> &ramka,
                                        TypWiadomosci &typ,
                                        std::string *blad)
{
    Naglowek naglowek;
    if (!odczytajNaglowek(ramka, naglowek, blad))
        return false;

    typ = naglowek.typ;
    return true;
}

bool ProtokolUAR::dekodujRamke(const std::vector<uint8_t> &ramka,
                               OdebranaWiadomosc &wiadomosc,
                               std::string *blad)
{
    TypWiadomosci typ;
    if (!odczytajTypWiadomosci(ramka, typ, blad))
        return false;

    wiadomosc.typ = typ;

    switch (typ)
    {
        case TypWiadomosci::ProbkaSymulacji:
        {
            PakietProbkiSymulacji p;
            if (!odczytajRamkeProbkiSymulacji(ramka, p, blad))
                return false;
            wiadomosc.payload = p;
            return true;
        }
        case TypWiadomosci::Sterowanie:
        {
            PakietSterowania s;
            if (!odczytajRamkeSterowania(ramka, s, blad))
                return false;
            wiadomosc.payload = s;
            return true;
        }
        case TypWiadomosci::Konfiguracja:
        {
            KonfiguracjaUAR cfg;
            if (!odczytajRamkeKonfiguracji(ramka, cfg, blad))
                return false;
            wiadomosc.payload = cfg;
            return true;
        }
    }

    ustawBlad(blad, "Nieobslugiwany typ wiadomosci.");
    return false;
}

void ProtokolUAR::uruchomTestProtokolu()
{
    std::cout << "1. Wygeneruj ramke KONFIGURACJA\n";
    std::cout << "2. Wygeneruj ramke PROBKA_SYMULACJI\n";
    std::cout << "3. Wygeneruj ramke STEROWANIE\n";
    std::cout << "4. Wczytaj ramke HEX i zdeserializuj (auto-typ)\n";
    std::cout << "> Wybierz opcje (1/2/3/4): ";

    int wybor;
    if (!(std::cin >> wybor))
        return;

    if (wybor == 1)
    {
        KonfiguracjaUAR konf;
        konf.kP = 7.77;
        konf.symulacjaTTms = 123;

        std::vector<uint8_t> ramka = zbudujRamkeKonfiguracji(konf);

        std::cout << "\n[NADAWCA] Utworzono ramke. Skopiuj ponizszy kod HEX:\n";
        std::cout << doHex(ramka) << "\n\n";
    }
    else if (wybor == 2)
    {
        PakietProbkiSymulacji p;
        p.krok = 12;
        p.t = 2.4;
        p.w = 1.5;
        p.y = 1.23;

        const std::vector<uint8_t> ramka = zbudujRamkeProbkiSymulacji(p);

        std::cout << "\n[NADAWCA] Utworzono ramke probki. Skopiuj ponizszy kod HEX:\n";
        std::cout << doHex(ramka) << "\n\n";
    }
    else if (wybor == 3)
    {
        PakietSterowania s;
        s.krok = 12;
        s.u = 0.88;
        s.uP = 0.66;
        s.uI = 0.20;
        s.uD = 0.02;

        const std::vector<uint8_t> ramka = zbudujRamkeSterowania(s);

        std::cout << "\n[NADAWCA] Utworzono ramke sterowania. Skopiuj ponizszy kod HEX:\n";
        std::cout << doHex(ramka) << "\n\n";
    }
    else if (wybor == 4)
    {
        std::cout << "\n[ODBIORCA] Wklej kod HEX wygenerowany na nadawcy: ";
        std::string hexInput;
        std::cin >> hexInput;

        std::vector<uint8_t> ramka = zHex(hexInput);

        OdebranaWiadomosc wiadomosc;
        std::string blad;
        if (!dekodujRamke(ramka, wiadomosc, &blad))
        {
            std::cout << "Blad: " << blad << "\n";
            return;
        }

        switch (wiadomosc.typ)
        {
        case TypWiadomosci::Konfiguracja:
        {
            const KonfiguracjaUAR &cfg = std::get<KonfiguracjaUAR>(wiadomosc.payload);

            std::cout << " -> Konfiguracja odtworzona poprawnie\n";
            std::cout << "    Odczytane kP = " << cfg.kP << "\n";
            std::cout << "    Odczytane TTms = " << cfg.symulacjaTTms << "\n";
            break;
        }
        case TypWiadomosci::ProbkaSymulacji:
        {
            const PakietProbkiSymulacji &p = std::get<PakietProbkiSymulacji>(wiadomosc.payload);

            std::cout << " -> Probka symulacji odczytana poprawnie\n";
            std::cout << "    krok = " << p.krok << "\n";
            std::cout << "    t = " << p.t << ", w = " << p.w << ", y = " << p.y << "\n";
            break;
        }
        case TypWiadomosci::Sterowanie:
        {
            const PakietSterowania &s = std::get<PakietSterowania>(wiadomosc.payload);

            std::cout << " -> Pakiet sterowania odczytany poprawnie\n";
            std::cout << "    krok = " << s.krok << "\n";
            std::cout << "    u = " << s.u
                      << " (uP=" << s.uP << ", uI=" << s.uI << ", uD=" << s.uD << ")\n";
            break;
        }
        default:
            std::cout << "Blad: Nieobslugiwany typ wiadomosci.\n";
            return;
        }
    }
    else
    {
        std::cout << "Nieznana opcja.\n";
    }
}
