# Bakalářská práce

## Název a téma
Návrh komunikačního rozhraní mezi mikrokontroléry STM32 a prostředím Matlab pro experimentální a vzdělávací účely

## Cílová skupina

Cílenou skupinou mé bakalářské práce mohou být:

- **Studenti technických oborů**
  - Studenti elektrotechniky, mechatroniky nebo příbuzných oborů
  - Základní až středně pokročilé znalosti programování mikrokontrolérů
 
- **Pedagogové a školitelé**
  - Učitelé, kteří chtějí použít tento systém pro výuku předmětů
  - Vysoká odbornost, ale nedostatek času
 
## Obsah dokumentace

Tento soubor slouží jako uživatelská příručka. Obsahuje tyto sekce:

1. Úvod
2. Předpoklady
3. Účely použití
4. Přidání knihovny pro mikrokontrolér
5. Přidání knihovny pro MATLAB
6. Použití knihovny pro mikrokontrolér
7. Použití knihovny pro MATLAB


## 1. Úvod

Jedná se o univerzální knihovnu pro přenos dat mezi mikrokontroléry STM32 a desktopovou aplikací v MATLABu. Knihovna podporuje přenos přes dvě komunikačních rozhraní. Klasické rozhraní UART a modernější vysokorychlostní rozhraní USB OTG FS. 

## 2. Předpoklady 

### Znalosti

- Knihovna slouží k usnadnění práce, neřeší za uživatele samotnou práci s programovacím jazykem C a MATLAB. Pro použití knihovny je nutno mít základní znalosti o principech fungování těchto jazyků, orientovat se v základních technických pojmech.

### Hardware

- Knihovna je navržena a plně kompatibilní s mikrokontroléry řady STM32 Nucleo L4xxxx. Funkčnost u ostatních řad není garantovaná. Mikrokontrolér musí mít k dispozici a podporovat:
  1. Rozhraní UART
  2. Rozhraní USB OTG FS
- Bez podpory těchto dvou rozhraní nebude knihovna použitelná.

### Software

Knihovnu pro mikrokontrolér je možno použít samostatně v kterémkoliv zdrojovém kódu, při použití jakéhokoliv vývojového prostředí. Dokumentace je ovšem sepsána s ohledem na použití oficiálního vývojového prostředí CubeIDE, které vyvíjí a poskytuje zdarma přímo firma STMicroelectronics, výrobce mikrokontrolérů. Návod se tedy nezabývá řešením případných problémů spojených s použitím jiného IDE. 

Knihovna pro desktopovou aplikaci je kompatibilní pouze s prostředím MATLAB. Je nutno mít koupenou/dostupnou validní licenci.

## 3. Účely použití

- Knihovna byla navržena pro usnadnění výuky v předmětech na univerzitě, aby studenti na předmětech nebyli nuceni řešit principy přenosu dat do počítače a mohli řešit pouze práci relevantní pro svůj obor. Knihovna zde slouží primárně jako prostředník pro odesílání načtených dat z logických obvodů a jejich následné zpracování v aplikaci v MATLABu.

- Obecně lze knihovnu v kombinaci s kompatibilním mikrokontrolérem použít pro přenos libovolných proměnných do desktopové aplikace. Následně také z desktopové aplikace odeslat data mikrokontroléru.

## 4. Přidání knihovny pro mikrokontrolér


