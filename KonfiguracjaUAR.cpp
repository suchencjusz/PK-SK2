#include "KonfiguracjaUAR.h"
#include <fstream>
#include <stdexcept>
#include "Json.hpp"

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

// Serializacja/Deserializacja protokolu
std::string KonfiguracjaUAR::serializujDoJSON() const
{
    ordered_json j;
    ordered_json cfg;

    ordered_json sym;
    sym["Interwal TTms"] = symulacjaTTms;
    sym["okno Obserwacji S"] = oknoObserwacjiS;
    cfg["SYMULACJA"] = sym;

    ordered_json arx;
    arx["Wspolczynniki wielomianu A"] = A;
    arx["Wspolczynniki wielomianu B"] = B;
    arx["Opoznienie transportowe (k)"] = k;
    arx["Zaklocenie (sigma)"] = sigma;
    arx["Wlacz ograniczenie sterowania (u)"] = ograniczU;
    arx["Min u"] = uMin;
    arx["Max u"] = uMax;
    arx["Wlacz ograniczenie wyjscia (y)"] = ograniczY;
    arx["Min y"] = yMin;
    arx["Max y"] = yMax;
    cfg["PARAMETRY ARX"] = arx;

    ordered_json pid;
    pid["Wzmocnienie (Kp)"] = kP;
    pid["Calkowanie (Ti)"] = Ti;
    pid["Rozniczkowanie (Td)"] = Td;
    pid["Tryb calkowania"] =
        (trybCalk == RegulatorPID::LiczCalk::Zewnetrzne ? "Zewnetrzne" : "Wewnetrzne");
    cfg["PARAMETRY PID"] = pid;

    ordered_json gen;
    gen["Typ sygnalu"] =
        (typGeneratora == GeneratorWartosciZadanej::Typ::Sinus
            ? "Sinus"
            : "Prostokat");
    gen["Amplituda (A)"] = A_gen;
    gen["Okres (TRZ)"] = TRZ;
    gen["Wypelnienie (p)"] = p_gen;
    gen["Skladowa stala (S)"] = S_gen;
    cfg["GENERATOR WARTOSCI ZADANEJ"] = gen;

    j["KONFIGURACJA:"] = cfg;
    return j.dump(); 
}

KonfiguracjaUAR KonfiguracjaUAR::wczytajZJSON(const std::string& jsonString)
{
    json j = json::parse(jsonString);
    KonfiguracjaUAR cfg;

    if (j.contains("KONFIGURACJA:"))
    {
        const json& root = j["KONFIGURACJA:"];

        if (root.contains("SYMULACJA"))
        {
            const json& js = root["SYMULACJA"];
            cfg.symulacjaTTms = js.value("Interwal TTms", cfg.symulacjaTTms);
            cfg.oknoObserwacjiS = js.value("okno Obserwacji S", cfg.oknoObserwacjiS);
        }

        if (root.contains("PARAMETRY ARX"))
        {
            const json& jm = root["PARAMETRY ARX"];
            if (jm.contains("Wspolczynniki wielomianu A"))
                jm.at("Wspolczynniki wielomianu A").get_to(cfg.A);
            if (jm.contains("Wspolczynniki wielomianu B"))
                jm.at("Wspolczynniki wielomianu B").get_to(cfg.B);

            cfg.k = jm.value("Opoznienie transportowe (k)", cfg.k);
            cfg.sigma = jm.value("Zaklocenie (sigma)", cfg.sigma);
            cfg.ograniczU = jm.value("Wlacz ograniczenie sterowania (u)", cfg.ograniczU);
            cfg.uMin = jm.value("Min u", cfg.uMin);
            cfg.uMax = jm.value("Max u", cfg.uMax);
            cfg.ograniczY = jm.value("Wlacz ograniczenie wyjscia (y)", cfg.ograniczY);
            cfg.yMin = jm.value("Min y", cfg.yMin);
            cfg.yMax = jm.value("Max y", cfg.yMax);
        }

        if (root.contains("PARAMETRY PID"))
        {
            const json& jp = root["PARAMETRY PID"];
            cfg.kP = jp.value("Wzmocnienie (Kp)", cfg.kP);
            cfg.Ti = jp.value("Calkowanie (Ti)", cfg.Ti);
            cfg.Td = jp.value("Rozniczkowanie (Td)", cfg.Td);

            const std::string trybStr = jp.value("Tryb calkowania", std::string("Zewnetrzne"));
            cfg.trybCalk = (trybStr == "Wewnetrzne")
                ? RegulatorPID::LiczCalk::Wewnetrzne
                : RegulatorPID::LiczCalk::Zewnetrzne;
        }

        const char* genKey = "GENERATOR WARTOSCI ZADANEJ";
        if (root.contains(genKey))
        {
            const json& jg = root[genKey];

            const std::string typStr = jg.value("Typ sygnalu", std::string("Sinus"));
            cfg.typGeneratora =
                (typStr == "Prostokat")
                ? GeneratorWartosciZadanej::Typ::Prostokat
                : GeneratorWartosciZadanej::Typ::Sinus;

            cfg.A_gen = jg.value("Amplituda (A)", cfg.A_gen);
            cfg.TRZ = jg.value("Okres (TRZ)", cfg.TRZ);
            cfg.p_gen = jg.value("Wypelnienie (p)", cfg.p_gen);
            cfg.S_gen = jg.value("Skladowa stala (S)", cfg.S_gen);
        }
    }
    return cfg;
}

// Zapis
void KonfiguracjaUAR::zapiszDoPlikuJSON(const std::string& sciezka) const
{
    std::ofstream ofs(sciezka);
    if (!ofs)
    {
        throw std::runtime_error("Nie mozna otworzyc pliku do zapisu: " + sciezka);
    }
    
    // Uzywamy istniejącej serializacji z wymuszeniem prety-print w oparciu o parser
    json wczytanyString = json::parse(serializujDoJSON());
    ofs << wczytanyString.dump(4);
}

// Odczyt
KonfiguracjaUAR KonfiguracjaUAR::wczytajZPlikuJSON(const std::string& sciezka)
{
    std::ifstream ifs(sciezka);
    if (!ifs)
    {
        throw std::runtime_error("Nie mozna otworzyc pliku do odczytu: " + sciezka);
    }
    
    std::string zawartosc((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
                           
    return wczytajZJSON(zawartosc);
}

