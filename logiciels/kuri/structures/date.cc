/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "date.hh"

#include <time.h>

#ifdef _MSC_VER
#    include <windows.h>

#    include <profileapi.h>

#    define CLOCK_REALTIME_COARSE 0

// https://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
static LARGE_INTEGER getFILETIMEoffset()
{
    SYSTEMTIME s;
    s.wYear = 1970;
    s.wMonth = 1;
    s.wDay = 1;
    s.wHour = 0;
    s.wMinute = 0;
    s.wSecond = 0;
    s.wMilliseconds = 0;

    FILETIME f;
    SystemTimeToFileTime(&s, &f);

    LARGE_INTEGER t;
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
    return (t);
}

static int clock_gettime(int /*X*/, struct timespec *tv)
{
    LARGE_INTEGER t;
    FILETIME f;
    double microseconds;
    static LARGE_INTEGER offset;
    static double frequencyToMicroseconds;
    static int initialized = 0;
    static BOOL usePerformanceCounter = 0;

    if (!initialized) {
        LARGE_INTEGER performanceFrequency;
        initialized = 1;
        usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
        if (usePerformanceCounter) {
            QueryPerformanceCounter(&offset);
            frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
        }
        else {
            offset = getFILETIMEoffset();
            frequencyToMicroseconds = 10.;
        }
    }
    if (usePerformanceCounter)
        QueryPerformanceCounter(&t);
    else {
        GetSystemTimeAsFileTime(&f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    microseconds = (double)t.QuadPart / frequencyToMicroseconds;
    t.QuadPart = (long long)microseconds;
    tv->tv_sec = t.QuadPart / 1000000;
    tv->tv_nsec = t.QuadPart % 1000000;
    return (0);
}
#endif

TempsSysteme maintenant_systeme()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME_COARSE, &t);

    TempsSysteme temps;
    temps.seconde = t.tv_sec;
    temps.nanoseconde = t.tv_nsec;

    return temps;
}

bool est_annee_bissextile(int64_t annee)
{
    if (annee % 100 == 0) {
        if (annee % 400 == 0) {
            return true;
        }

        return false;
    }

    return (annee % 4 == 0);
}

Date hui_systeme()
{
    auto inst = maintenant_systeme();

    auto seconde = inst.seconde % 60;
    auto minutes = (inst.seconde / 60) % 60;
    auto heures = (inst.seconde / 3600) % 24;

    auto annees = (inst.seconde / secondes_par_an());

    int jours_par_mois[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (est_annee_bissextile(annees)) {
        jours_par_mois[1] = 29;
    }

    auto secondes_mois = (inst.seconde - (annees * secondes_par_an()));
    auto secondes_ecoulees_mois = 0;
    auto mois = 0;

    for (auto j : jours_par_mois) {
        if (secondes_mois < (28 * 24 * 60 * 60)) {
            break;
        }

        secondes_ecoulees_mois += j * 24 * 60 * 60;
        secondes_mois -= j * 24 * 60 * 60;
        mois += 1;
    }

    auto jours = (inst.seconde - (annees * secondes_par_an()) - (secondes_ecoulees_mois)) /
                 (24 * 60 * 60);

    // On soustrait 3 car Epoch UTC commence un jeudi (4ème jour, index 3)
    auto jour_semaine = ((inst.seconde / (3600 * 24)) - 3) % 7;

    Date date;
    date.jour = static_cast<int>(jours + 1);
    date.mois = mois + 1;
    date.annee = static_cast<int>(annees + 1970);
    date.heure = static_cast<int>(heures) + 2;  // UTC
    date.minute = static_cast<int>(minutes);
    date.seconde = static_cast<int>(seconde);
    date.jour_semaine = static_cast<int>(jour_semaine);

    if (date.heure >= 24) {
        date.heure -= 24;
    }

    return date;
}
