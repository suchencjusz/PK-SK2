#pragma once
#include <vector>
#include <string>
#include "RegulatorPID.h"
#include "GeneratorWartosciZadanej.h"

struct KonfiguracjaUAR
{
    // Symulacja
    int symulacjaTTms = 200;
    int oknoObserwacjiS = 10;

    // Model ARX
    std::vector<double> A;
    std::vector<double> B;
    int    k = 1;
    double sigma = 0.0;

    double uMin = -10.0;
    double uMax = 10.0;
    double yMin = -10.0;
    double yMax = 10.0;
    bool   ograniczU = true;
    bool   ograniczY = true;

    // Regulator PID
    double kP = 1.0;
    double Ti = 0.0;
    double Td = 0.0;

    RegulatorPID::LiczCalk trybCalk = RegulatorPID::LiczCalk::Zewnetrzne;

    // Generator wartosci zadanej
    GeneratorWartosciZadanej::Typ typGeneratora =
        GeneratorWartosciZadanej::Typ::Sinus;

    double TRZ = 1.0;
    double A_gen = 1.0;
    double S_gen = 0.0;
    double p_gen = 0.5;

    // Zapis / odczyt
    void zapiszDoPlikuJSON(const std::string& sciezka) const;
    static KonfiguracjaUAR wczytajZPlikuJSON(const std::string& sciezka);
};

