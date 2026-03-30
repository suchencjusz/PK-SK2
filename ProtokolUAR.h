#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <variant>
#include "KonfiguracjaUAR.h"

enum TypWiadomosci : uint8_t
{
    Konfiguracja = 1,
    Sterowanie = 2,
    ProbkaSymulacji = 3
};

// wylaczenie wyrownywania pamieci na potrzeby binarnych ramek sieciowych
#pragma pack(push, 1)
struct Naglowek
{
    TypWiadomosci typ;
    uint32_t rozmiarDanych;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PakietProbkiSymulacji
{
    uint64_t krok = 0;
    double t = 0.0;
    double w = 0.0;
    double y = 0.0;
};

struct PakietSterowania
{
    uint64_t krok = 0;
    double u = 0.0;
    double uP = 0.0;
    double uI = 0.0;
    double uD = 0.0;
};
#pragma pack(pop)

using PayloadWiadomosci = std::variant<KonfiguracjaUAR, PakietProbkiSymulacji, PakietSterowania>;

struct OdebranaWiadomosc
{
    TypWiadomosci typ = TypWiadomosci::Konfiguracja;
    PayloadWiadomosci payload = KonfiguracjaUAR{};
};

class ProtokolUAR
{
public:
    static std::vector<uint8_t> zbudujRamkeKonfiguracji(const KonfiguracjaUAR &cfg);
    static bool odczytajRamkeKonfiguracji(const std::vector<uint8_t> &ramka, KonfiguracjaUAR &cfg, std::string *blad = nullptr);

    static std::vector<uint8_t> zbudujRamkeProbkiSymulacji(const PakietProbkiSymulacji &probka);
    static bool odczytajRamkeProbkiSymulacji(const std::vector<uint8_t> &ramka, PakietProbkiSymulacji &probka, std::string *blad = nullptr);

    static std::vector<uint8_t> zbudujRamkeSterowania(const PakietSterowania &sterowanie);
    static bool odczytajRamkeSterowania(const std::vector<uint8_t> &ramka, PakietSterowania &sterowanie, std::string *blad = nullptr);

    static bool odczytajTypWiadomosci(const std::vector<uint8_t> &ramka, TypWiadomosci &typ, std::string *blad = nullptr);

    static bool dekodujRamke(const std::vector<uint8_t> &ramka, OdebranaWiadomosc &wiadomosc, std::string *blad = nullptr);

    static void uruchomTestProtokolu();

private:
    static void ustawBlad(std::string *blad, const std::string &tresc);
    static std::vector<uint8_t> zbudujRamkeWewnetrzna(TypWiadomosci typ, const std::string &payload);
    
    static bool odczytajNaglowek(const std::vector<uint8_t> &ramka, Naglowek &naglowek, std::string *blad);
    static bool odczytajPayload(const std::vector<uint8_t> &ramka, TypWiadomosci oczekiwanyTyp, std::string &payload, std::string *blad);
    
    static std::string doHex(const std::vector<uint8_t> &data);
    static std::vector<uint8_t> zHex(const std::string &hex);
};