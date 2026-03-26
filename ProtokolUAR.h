#pragma once
#include <cstdint>
#include <vector>
#include <string>

enum TypWiadomosci : uint8_t {
    Konfiguracja = 1,
    Sterowanie = 2,
    ProbkaSymulacji = 3
};

// wylaczenie wyrownywania pamieci na potrzeby binarnych ramek sieciowych
#pragma pack(push, 1)
struct Naglowek {
    TypWiadomosci typ;
    uint32_t rozmiarDanych;
};
#pragma pack(pop)

std::vector<uint8_t> zbudujRamke(TypWiadomosci typ, const std::string& payload);

void uruchomTestProtokolu();