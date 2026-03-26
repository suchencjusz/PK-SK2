#include "Testy_Wlasne.h"
#include <QDebug>
#include <cmath>
#include <vector>
#include "ModelARX.h"
#include "RegulatorPID.h"
#include "GeneratorWartosciZadanej.h"
#include "SymulacjaUAR.h"

namespace
{
    constexpr double EPS = 1e-6;

    bool approx(double a, double b, double eps = EPS)
    {
        return std::fabs(a - b) <= eps;
    }

    class TestRunner
    {
    public:
        static void reset()
        {
            total_ = 0;
            passed_ = 0;
        }

        static void add(bool ok, const char* name, const char* details = nullptr)
        {
            ++total_;
            if (ok)
            {
                ++passed_;
                return;
            }

            if (details)
                qWarning().noquote() << "FAIL:" << name << "->" << details;
            else
                qWarning().noquote() << "FAIL:" << name;
        }

        static void summary()
        {
            qInfo().noquote() << "Testy zakonczone powodzeniem:" << passed_ << "/" << total_;
        }

    private:
        static int total_;
        static int passed_;
    };

    int TestRunner::total_ = 0;
    int TestRunner::passed_ = 0;

// ograniczenie sterowania (u) w ARX przed obliczeniami
    void test_arx_clamp_u()
    {
        ModelARX m1({ 0.0 }, { 1.0 }, 1, 0.0);
        m1.ustawLimity(-1.0, 1.0, -100.0, 100.0);
        m1.ustawOgraniczanieU(true);
        m1.symuluj(5.0);
        const double y1 = m1.symuluj(5.0);

        ModelARX m2({ 0.0 }, { 1.0 }, 1, 0.0);
        m2.ustawLimity(-1.0, 1.0, -100.0, 100.0);
        m2.ustawOgraniczanieU(true);
        m2.symuluj(1.0);
        const double y2 = m2.symuluj(1.0);

        const bool ok = approx(y1, y2) && approx(y1, 1.0);
        TestRunner::add(ok, "ARX_OgraniczenieSterowania", "u powinno byc ograniczone do 1.0");
    }

// ograniczenie wyjscia (y) w ARX po obliczeniach
    void test_arx_clamp_y()
    {
        ModelARX m({ 0.0 }, { 1.0 }, 1, 0.0);
        m.ustawLimity(-10.0, 10.0, -0.5, 0.5);
        m.ustawOgraniczanieY(true);
        m.symuluj(1.0);
        const double y = m.symuluj(1.0);

        const bool ok = approx(y, 0.5);
        TestRunner::add(ok, "ARX_OgraniczenieWyjscia", "y powinno byc ograniczone do 0.5");
    }

// wylaczone ograniczanie sterowania (u) w ARX
    void test_arx_disable_u_clamp()
    {
        ModelARX m({ 0.0 }, { 1.0 }, 1, 0.0);
        m.ustawLimity(-1.0, 1.0, -10.0, 10.0);
        m.ustawOgraniczanieU(false);
        m.symuluj(5.0);
        const double y = m.symuluj(5.0);

        const bool ok = approx(y, 5.0);
        TestRunner::add(ok, "ARX_WylaczoneOgraniczenieSterowania", "ograniczanie u powinno byc wylaczone");
    }

// zmiana opoznienia transportowego k w trakcie pracy
    void test_arx_change_delay()
    {
        ModelARX m({ 0.0 }, { 1.0 }, 2, 0.0);
        m.ustawOgraniczanieU(false);
        m.ustawOgraniczanieY(false);

        m.symuluj(1.0);
        const double y1 = m.symuluj(1.0);
        m.ustawOpoznienie(1);
        const double y2 = m.symuluj(1.0);

        const bool ok = approx(y1, 0.0) && approx(y2, 1.0);
        TestRunner::add(ok, "ARX_ZmianaOpoznieniaK", "zmiana k powinna wplywac na opoznienie");
    }

// sigma = 0 daje deterministyczne wyniki ARX
    void test_arx_sigma_zero_deterministic()
    {
        ModelARX m({ -0.4 }, { 0.6 }, 1, 0.0);
        std::vector<double> in = { 0.0, 1.0, 1.0, 1.0, 0.5 };
        std::vector<double> out1;
        for (double u : in)
            out1.push_back(m.symuluj(u));

        m.reset();
        std::vector<double> out2;
        for (double u : in)
            out2.push_back(m.symuluj(u));

        bool ok = out1.size() == out2.size();
        for (std::size_t i = 0; ok && i < out1.size(); ++i)
            ok = approx(out1[i], out2[i]);

        TestRunner::add(ok, "ARX_SigmaZero_Deterministyczny", "brak szumu powinien byc deterministyczny");
    }

// TI = 0 wylacza calke bez kasowania sumy
    void test_pid_ti_zero_memory()
    {
        RegulatorPID pid(0.0, 1.0, 0.0);
        pid.symuluj(1.0);
        pid.symuluj(1.0);
        pid.symuluj(1.0);
        const double uI_before = pid.pobierzUI();

        pid.setStalaCalk(0.0);
        pid.symuluj(1.0);
        const double uI_zero = pid.pobierzUI();

        pid.setStalaCalk(1.0);
        pid.symuluj(0.0);
        const double uI_after = pid.pobierzUI();

        const bool ok = approx(uI_before, 3.0) && approx(uI_zero, 0.0) && approx(uI_after, 3.0);
        TestRunner::add(ok, "PID_TiZero_BezKasowaniaSumy", "TI=0 powinno wylaczyc calke bez kasowania sumy");
    }

//resetCalkowania zeruje skladowa I
    void test_pid_reset_calkowania()
    {
        RegulatorPID pid(0.0, 1.0, 0.0);
        pid.symuluj(1.0);
        pid.symuluj(1.0);
        pid.resetCalkowania();

        const double uI = pid.pobierzUI();
        const double u = pid.symuluj(0.0);

        const bool ok = approx(uI, 0.0) && approx(u, 0.0);
        TestRunner::add(ok, "PID_ResetCalkowania", "resetCalkowania powinien wyzerowac uI");
    }

// zmiana trybu calkowania nie powoduje skoku uI
    void test_pid_tryb_switch_preserves_ui()
    {
        RegulatorPID pid(0.0, 2.0, 0.0);
        pid.symuluj(1.0);
        pid.symuluj(1.0);
        const double uI_before = pid.pobierzUI();

        pid.setLiczCalk(RegulatorPID::LiczCalk::Wewnetrzne);
        const double uI_after_switch = pid.pobierzUI();
        pid.symuluj(0.0);
        const double uI_after_step = pid.pobierzUI();

        pid.setLiczCalk(RegulatorPID::LiczCalk::Zewnetrzne);
        pid.symuluj(0.0);
        const double uI_final = pid.pobierzUI();

        const bool ok = approx(uI_before, uI_after_switch) && approx(uI_after_switch, uI_after_step)
            && approx(uI_after_step, uI_final);
        TestRunner::add(ok, "PID_ZmianaTrybuCalkowania", "zmiana trybu calkowania nie powinna skakac uI");
    }

// wypelnienie prostokata zmienia proporcje stanu wysokiego
    void test_GEN_Prostokat_Wypelnienie()
    {
        GeneratorWartosciZadanej gen;
        gen.ustawTyp(GeneratorWartosciZadanej::Typ::Prostokat);
        gen.ustawAmplituda(1.0);
        gen.ustawSkladowaStala(0.0);
        gen.ustawOkresRzeczywisty(1.0);
        gen.ustawInterwalSymulacjiMs(100);

        gen.ustawWypelnienie(0.2);
        int high1 = 0;
        for (int i = 0; i < 10; ++i)
        {
            const double v = gen.generuj(i);
            if (approx(v, 1.0))
                ++high1;
        }

        gen.ustawWypelnienie(0.8);
        int high2 = 0;
        for (int i = 0; i < 10; ++i)
        {
            const double v = gen.generuj(i);
            if (approx(v, 1.0))
                ++high2;
        }

        const bool ok = high2 > high1;
        TestRunner::add(ok, "GEN_Prostokat_Wypelnienie", "wieksze p powinno dawac wiecej probek wysokich");
    }

// okres sinusa zgodny z TRZ i TTms
    void test_GEN_Sinus_Okres()
    {
        GeneratorWartosciZadanej gen;
        gen.ustawTyp(GeneratorWartosciZadanej::Typ::Sinus);
        gen.ustawAmplituda(1.0);
        gen.ustawSkladowaStala(0.0);
        gen.ustawOkresRzeczywisty(2.0);
        gen.ustawInterwalSymulacjiMs(200);

        const double w0 = gen.generuj(0);
        const double w5 = gen.generuj(5);
        const double w10 = gen.generuj(10);

        const bool ok = approx(w0, w10, 1e-3) && approx(w5, 0.0, 1e-3);
        TestRunner::add(ok, "GEN_Sinus_Okres", "wartosci powinny byc okresowe i zgodne z sinusem");
    }

//  czas symulacji rosnie o TTms/1000 na krok
    void test_symulacja_czas_kroku()
    {
        SymulacjaUAR sim;
        sim.ustawInterwalSymulacjiMs(200);
        const double t0 = sim.wykonajKrok().t;
        const double t1 = sim.wykonajKrok().t;
        const double t2 = sim.wykonajKrok().t;

        const bool ok = approx(t0, 0.0) && approx(t1, 0.2, 1e-6) && approx(t2, 0.4, 1e-6);
        TestRunner::add(ok, "SIM_CzasKroku", "czas powinien rosnac o TTms/1000");
    }

// reset zeruje czas i stan symulacji
    void test_symulacja_reset()
    {
        SymulacjaUAR sim;
        sim.ustawInterwalSymulacjiMs(200);
        sim.wykonajKrok();
        sim.wykonajKrok();
        sim.reset();
        const double t = sim.wykonajKrok().t;

        const bool ok = approx(t, 0.0);
        TestRunner::add(ok, "SIM_Reset", "reset powinien zerowac czas symulacji");
    }
}

void TestyWlasne::uruchom()
{
    TestRunner::reset();

    test_arx_clamp_u();
    test_arx_clamp_y();
    test_arx_disable_u_clamp();
    test_arx_change_delay();
    test_arx_sigma_zero_deterministic();
    test_pid_ti_zero_memory();
    test_pid_reset_calkowania();
    test_pid_tryb_switch_preserves_ui();
    test_GEN_Prostokat_Wypelnienie();
    test_GEN_Sinus_Okres();
    test_symulacja_czas_kroku();
    test_symulacja_reset();

    TestRunner::summary();
}

