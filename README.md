# DIY Blockchain — v0.2

## 1. Reikalavimai

- C++17 suderinamas kompiliatorius.
- `g++` arba kitas C++17 palaikantis kompiliatorius.

## 1.1. Kompiliavimas ir paleidimas

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

## 2. Modelio aprašas

## Atsitiktinių duomenų generavimas (Seed)

Naudojamas fiksuotas atsitiktinių skaičių generatoriaus seed:  
**Seed: 3719**  
Seed perduodamas per bendrą `std::mt19937` generatorių.  


## 3. v0.2 Pakeitimai ir aiškinimai

### 1. Testai
Testai, kuriuos galima rasti pagrindiniame main.cpp faile:
- Merkle Tree testai
  - test1: Patikrina, kad Merkle šaknis yra ne tuščia ir deterministinė. Testas vykdomas su 1, 2, 3 ir 5 transakcijomis.
  - test2: Pakeičiama viena transakcijos reikšmė ir patikrinama, kad transakcijos ID ir Merkle šaknis pasikeičia drastiškai.
- Validavimo testai
  - test1: Bandoma į grandinę įtraukti bloką su dvejomis transakcijomis, kurių bendra suma viršija siuntėjo balansą. Laukiama, kad add_block išmes exception, o grandinės būsena (balansai, dydis, transakcijų ID, sequence) liks nepakitusi.
  - test2: Du mokėjimai, kurie negali būti tame pačiame bloke dėl sumos, įtraukiami į pool. Iškasamas blokas su mine_next_block, priimamas add_block, laimėjusių transakcijų indeksai pašalinami iš pool.
- Penkių kandidatų batch mining testas
  1.  Sukuriami 10 naudotojų ir 50 transakcijų (seed iš std::random_device).
  2.  Transakcijos validuojamos ir sudedamos į baseiną per add_transaction (simuliuojamas tikras veikimas, kadangi jos visos validžios).
  3.  Sudaromi 5 kandidatai blokai: kiekvienas atsitiktinai atrenka transakcijas iš pool. Kandidatai remiasi tuo pačiu prev_hash ir pradiniais balansais.
  4.  Pirmas/initial round: kiekvienas kandidatas gauna 10 000 bandymų (nonce).
  5.  Jei nėra išsprendusių, kiekvienas neišsprendęs kandidatas gauna papildomą 20 000 bandymų. Tęsiama nuo paskutinio tikrinto nonce (candidate.next_nonce).
  6.  Sustabdoma, kai bent vienas kandidatas išsprendžia arba pasiekiamas bendras limitas 500 000 bandymų.
  7.  Išrenkamas laimėtojas: pirma – tas, su mažiausiu bandymų skaičiumi tarp išspręstų, lygybės atveju – mažesnis bloko hash (std::array).
  8.  Laimėtojo blokas iš naujo patikrinamas per blockchain.add_block su originaliais balansais.
  9.  Laimėtojo transakcijos pašalinamos iš pool.

### 2. Ar tai P2P konsensusas?
Ne. Štai kelios priežastys:
- Nėra tinklo. P2P reikalauja, kad tinklu būtų transliuojami blokai, validuojami kitų mazgų, čia visa tai nėra įgyvendinta.
- Laimėtojas yra išrenkamas vietinėje erdvėje, kaip minėta, tai nėra P2P.
- Šioje versijoje nėra lygiagretaus kasimo, kaip tikrose Bitcoin sistemose.

## 4. Versijų istorija

**v0.2 - Main Release**

Antroji versija, su papildytu centralizuotu (`blockchain.hpp`) transakcijų bei blokų validavimu, batch kandidatų blokų mining, merkle root. Iš esmės saugesnė ir pastovesnė versija.

**v0.1 — Initial Release**

Pirmoji prototipo versija, veikianti grandinė, įgyvendinanti vartotojų generavimą, transakcijų validavimą, blokų kūrimą, Proof of Work ir bendros lėšų sumos išsaugojimo patikrinimą.