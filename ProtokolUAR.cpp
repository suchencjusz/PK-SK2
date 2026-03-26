#include "ProtokolUAR.h"
#include "KonfiguracjaUAR.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

std::string doHex(const std::vector<uint8_t>& data) {
    std::stringstream ss;
    for (uint8_t b : data) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    }
    return ss.str();
}

std::vector<uint8_t> zHex(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

std::vector<uint8_t> zbudujRamke(TypWiadomosci typ, const std::string& payload) {
    Naglowek naglowek;
    naglowek.typ = typ;
    naglowek.rozmiarDanych = static_cast<uint32_t>(payload.size());

    std::vector<uint8_t> ramka;
    ramka.reserve(sizeof(Naglowek) + payload.size());

    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&naglowek);
    ramka.insert(ramka.end(), ptr, ptr + sizeof(Naglowek));
    ramka.insert(ramka.end(), payload.begin(), payload.end());
    
    return ramka;
}

void uruchomTestProtokolu()
{
    std::cout << "1. Wygeneruj ramke KONFIGURACJA i wypisz jako HEX\n";
    std::cout << "2. Wczytaj ramke z ciagu znakow HEX i zdeserializuj\n";
    std::cout << "> Wybierz opcje (1/2): ";
    
    int wybor;
    if (!(std::cin >> wybor)) return;

    if (wybor == 1) {
        KonfiguracjaUAR konf;
        konf.kP = 7.77;
        konf.symulacjaTTms = 123;
        
        std::string payload = konf.serializujDoJSON();
        std::vector<uint8_t> ramka = zbudujRamke(TypWiadomosci::Konfiguracja, payload);
        
        std::cout << "\n[NADAWCA] Utworzono ramke. Skopiuj ponizszy kod HEX:\n";
        std::cout << doHex(ramka) << "\n\n";
        
    } else if (wybor == 2) {
        std::cout << "\n[ODBIORCA] Wklej kod HEX wygenerowany na nadawcy: ";
        std::string hexInput;
        std::cin >> hexInput;
        
        std::vector<uint8_t> ramka = zHex(hexInput);
        if (ramka.size() < sizeof(Naglowek)) {
            std::cout << "Blad: Wklejony ciag jest mniejszy niz rozmiar samego naglowka!\n";
            return;
        }
        
        // odczyt naglowka
        const Naglowek* naglowek = reinterpret_cast<const Naglowek*>(ramka.data());
        std::cout << "\n -> Odczytano Naglowek z bajtow:\n";
        std::cout << "    Typ wiadomosci: " << (int)naglowek->typ << "\n";
        std::cout << "    Rozmiar danych: " << naglowek->rozmiarDanych << " B\n";
        
        if (naglowek->typ == TypWiadomosci::Konfiguracja) {
            if (ramka.size() < sizeof(Naglowek) + naglowek->rozmiarDanych) {
                std::cout << "Blad: Za malo bajtow w paczce (payload zostal uciety)!\n";
                return;
            }
            
            // wyluskanie i zdeserializowanie samego tekstu JSON
            std::string payload(ramka.begin() + sizeof(Naglowek), 
                                ramka.begin() + sizeof(Naglowek) + naglowek->rozmiarDanych);
                                
            KonfiguracjaUAR odtworzona = KonfiguracjaUAR::wczytajZJSON(payload);

            std::cout << " -> Konfiguracja odtworzona poprawnie\n";
            std::cout << "    Odczytane kP = " << odtworzona.kP << "\n";
            std::cout << "    Odczytane TTms = " << odtworzona.symulacjaTTms << "\n";
        }
    } else {
        std::cout << "Nieznana opcja.\n";
    }
}
