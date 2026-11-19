# DIY Blockchain — v0.3

## 1. Reikalavimai

- C++17 suderinamas kompiliatorius.
- `g++` arba kitas C++17 palaikantis kompiliatorius.

## 1.1. Kompiliavimas ir paleidimas

Su lygiagretumo palaikymu:
```bash
g++ -std=c++17 -O2 -pthread test_batch_mining.cpp -o tests && ./tests
```

Linux / macOS:

```bash
g++ -std=c++17 -O2 main.cpp -o diyblockchain
./blockchain
```

Windows (MinGW):

```bash
g++ -std=c++17 -O2 main.cpp -o diyblockchain.exe
./blockchain.exe
```

## Atsitiktinių duomenų generavimas (Seed)

Naudojamas fiksuotas atsitiktinių skaičių generatoriaus seed:  
**Seed: 3719**  
Seed perduodamas vartotojų ir transakcijų generatoriams per bendrą `std::mt19937` generatorių.  

## v0.3 pakeitimai
Disclaimer: Ši versija daryta labai paprastai su DI pagalba ir neatitinka užduoties reikalavimų.

### Lygiagretus kasimas
Ši versija buvo padaryta įdomumo dėlei.
Tokiu principu Claude sugalvojo daryti lygiagretų kasimą:

- Duomenų lenktynės (race conditions) – nėra. Kiekviena gija rašo tik į atskirą atminties adresą. Vienintelis bendras kintamasis yra `std::atomic<bool> found`. Jis tikrinamas su memory_order_relaxed (greitas skaitymas be sinchrono), o rašymui naudojamas compare_exchange_strong – tai užtikrina, kad tik viena gija gali nustatyti found = true.

- Sustabdymas. Kiekviena gija tikrina found.load() prieš kiekvieną bandymą. Kai pirmasis kandidatas išsprendžia, kiti baigia einamą bandymą ir išeina. Tai reiškia, kad attempts gali viršyti limitą iki batch_size * 5.

- Laimėtojo pasirinkimas. Po join() visos gijos baigusios – tada select_winner() vykdomas viename sraute, be race condition. Jei keli kandidatai išsprendė (lygiagrečiai), select_winner parenka pagal mažiausią bandymų skaičių arba mažesnį hash.

- Palyginimas. Abu naudoja tuos pačius create_five_candidates parametrus. Nuoseklusis laikas yra suma per visus kandidatus; lygiagrečiojo – sienos laikas (wall time), kuris turėtų būti ~5 kartais mažesnis su pakankamai laisvu CPU.

## Programos išvestis

Lygiagretus kasimas v0.2
Thread count: 16

--- Nuoseklus kasimas ---
Pool: 9 tx
Rastas: taip
Bandymai: 18758
Raundai:  1
Laikas:   0.812630 s
Blokas priimtas. Pool liko: 0

--- Lygiagretus kasimas ---
Pool: 9 tx
Rastas: taip
Bandymai: 1009
Raundai:  1
Laikas:   0.097044 s  (sienos laikas)
Blokas priimtas. Pool liko: 0

=== Palyginimas: nuoseklus vs lygiagretus ===
Difficulty: 3, naudotojai: 20, tx: 100
[Nuoseklus]   laikas=0.924796 s, bandymai=11600, rastas=taip
[Lygiagretus] laikas=0.801882 s (sienos laikas), bandymai=9563, rastas=taip

nuoseklus vs lygiagretus
Nuoseklus:    praejo
Lygiagretus:  praejo

### Note
Bandymų skaicius skiriasi, nes lygiagretus kasimas sustoja kai tik pirmasis kandidatas išsprendžia, o kiti gali dar nespėti baigti savo iteracijos.

## Versijų istorija

**v0.3 - DI Lygiagretus Release**
Antroji (v0.2) versija, bet su lygiagrečiu kasimu penkių kandidatų blokų.

**v0.2 - Main Release**

Antroji versija, su papildytu centralizuotu (blockchain.hpp) transakcijų bei blokų validavimu, batch kandidatų blokų mining, merkle root. Iš esmės saugesnė ir pastovesnė versija.

**v0.1 — Initial Release**

Pirmoji prototipo versija, veikianti grandinė, įgyvendinanti vartotojų generavimą, transakcijų validavimą, blokų kūrimą, Proof of Work ir bendros lėšų sumos išsaugojimo patikrinimą.