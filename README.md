# DIY Blockchain — v0.1

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

### Maišos funkcija
- Naudojama v1.1 anksčiau sukurta hash funkcija 
- Hash Funkcijos [repo](https://github.com/tootless/UNI_BLCH_HF/tree/v.1.1)

### Vartotojai

- Sugeneruojama 1 000 vartotojų.
- Kiekvienas vartotojas turi unikalų ID, randomai sugen. vardą, viešąjį raktą ir pradinį balansą.
- Pradinis balansas yra atsitiktinis sveikasis skaičius nuo 100 iki 1 000 000.

### Transakcijos

- Sugeneruojama 10 000 transakcijų.
- Kiekviena transakcija turi:
  - Siuntėjo ID;
  - Gavėjo ID;
  - Pervedamą sumą;
  - Siuntėjo sekos numerį (`sequence`);
  - Unikalų transakcijos ID (maiša).

### Blokai

Kiekvienas blokas turi:
- Versiją;
- Laiko žymą (`timestamp`);
- Ankstesnio bloko maišą (`prev_hash`);
- Transakcijų šakninę maišą (`tx_root`);
- Nonce;
- Difficulty;
- Transakcijų sąrašą.

Vienas blokas turi <=100 transakcijų.

Pirmasis blokas yra Genesis blokas. Kiekvienas naujas blokas nurodo prieš tai priimto bloko maišą, todėl blokai sudaro susietą grandinę.  
Genesis blokas nėra PoW dalis, jis yra hardcoded į pačią bloko grandinę.

### Proof of Work

Bloko maiša skaičiuojama keičiant nonce reikšmę, kol gaunama maiša, atitinkanti nustatytą difficulty.  
Difficulty nurodo reikalaujamų nulinių pradinių šešioliktainių skaitmenų skaičių.  
Numatytoji difficulty reikšmė – 3.

### Transakcijos maiša

Transakcijos ID apskaičiuojamas iš šių laukų:
- Siuntėjo ID;
- Gavėjo ID;
- Transakcijos sumos;
- Sekos numerio.

### Bloko maiša

Bloko maiša apskaičiuojama iš serializuotos antraštės:
1. Version;
2. Timestamp;
3. Prev_hash
4. Tx_root;
5. Nonce;
6. Difficulty.

## Atsitiktinių duomenų generavimas (Seed)

Naudojamas fiksuotas atsitiktinių skaičių generatoriaus seed:  
**Seed: 3719**  
Seed perduodamas vartotojų ir transakcijų generatoriams per bendrą `std::mt19937` generatorių.  

## Pradinės būsenos išsaugojimas

Pradiniai vartotojų balansai išsaugomi prieš pradedant vykdyti transakcijas.

Ši būsena naudojama:
- Pradinei bendrai lėšų sumai apskaičiuoti.
- Galutinei lėšų sumai palyginti.
- Patikrinti, ar transakcijų vykdymo metu nebuvo sukurta arba prarasta lėšų, kadangi ji pradžioje ir pabaigoje turi būti vienoda.

## 3. Programos išvestis

Programa pateikia kiekvieno priimto bloko suvestinę:  

- Bloko aukštį;
- Transakcijų skaičių;
- `prev_hash`;
- `tx_root`;
- Bloko maišą;
- Difficulty;
- Rastą nonce;
- Bandymų skaičių;
- Kasimo laiką.

Programos pabaigoje pateikiama galutinė suvestinė:  

- Vartotojų skaičius;
- Sugeneruotų transakcijų skaičius;
- Priimtų blokų skaičius;
- Į blokus įtrauktų transakcijų skaičius;
- Atmestų transakcijų skaičius;
- Likusių transakcijų skaičius;
- Pradinė lėšų suma;
- Galutinė lėšų suma;
- Bendras programos vykdymo laikas;
- Atmestų ir likusių transakcijų priežastys.

### Vieno bloko išvesties pavyzdys
```text
--- Priimtas blokas ---  
Height: x  
Transakciju sk.: 100  
prev_hash: ...  
tx_root: ...  
Bloko hash: 000...  
Difficulty: 3  
Nonce: x  
Bandymu sk.: x  
Kasimo laikas: x s  
```

### Galutinės suvestinės pavyzdys
```text
Rezultatai:  
  
Vartotoju sk.: 1000  
Transakciju sk.: 10000  
Priimti blokai, be genesis: 100  
Transakcijos baseine: x  
Transakcijos blokuose: x  
Atmestos transakcijos: x  
Transakcijos vis dar baseinuose: x  
Pirmutine total balance x  
Finaline total balance: x  
Programos laikas: x s  
  
Atmestu tran. priezastys:  
  not enough balance: x  
  
Pasilikusiu tran. priezastys:  
  not enough balance: x  
  
total balance check praejo
```

## 4. Versija

**v0.1 — Initial Release**

Pirmoji prototipo versija, veikianti grandinė, įgyvendinanti vartotojų generavimą, transakcijų validavimą, blokų kūrimą, Proof of Work ir bendros lėšų sumos išsaugojimo patikrinimą.