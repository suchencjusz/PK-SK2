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

enum class RolaInstancjiSieciowej : uint8_t
{
    Nieznana = 0,
    Regulator = 1,
    Obiekt = 2
};

// Bitmaska jednorazowych komend dolaczanych do PakietSterowania.
// Pozwala przeslac kilka komend naraz w jednym pakiecie (np. reset I i reset D).
enum FlagiSterowaniaSieciowego : uint8_t
{
    SterowanieBrak = 0,          // 00000000
    SterowanieResetI = 1u << 0,  // 00000001
    SterowanieResetD = 1u << 1   // 00000010
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
    RolaInstancjiSieciowej rolaNadawcy = RolaInstancjiSieciowej::Nieznana;
    uint64_t krok = 0;
    double t = 0.0;
    double w = 0.0;
    double y = 0.0;
};

struct PakietSterowania
{
    RolaInstancjiSieciowej rolaNadawcy = RolaInstancjiSieciowej::Nieznana;
    uint64_t krok = 0;
    double u = 0.0;
    double w = 0.0; // Dodane zeby przesylac W
    double uP = 0.0;
    double uI = 0.0;
    double uD = 0.0;
    // OR bitowy flag z FlagiSterowaniaSieciowego.
    uint8_t flagiSterowania = SterowanieBrak;
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