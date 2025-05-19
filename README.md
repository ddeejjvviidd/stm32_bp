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
4. Zprovoznění knihovny pro CubeIDE
5. Přidání knihovny do MATLABu
6. Použití knihovny pro mikrokontrolér
7. Použití knihovny pro MATLAB


## 1. Úvod

Jedná se o univerzální knihovnu pro přenos dat mezi mikrokontroléry STM32 a desktopovou aplikací v MATLABu. Knihovna podporuje přenos přes dvě komunikačních rozhraní. Klasické rozhraní UART a modernější vysokorychlostní rozhraní USB OTG FS. 

## 2. Předpoklady 

### Znalosti

- Knihovna slouží k usnadnění práce, neřeší za uživatele samotnou práci s programovacím jazykem C a MATLAB. Pro použití knihovny je nutno mít základní znalosti o principech fungování těchto jazyků, orientovat se v základních technických pojmech.
- Je předpokládáno, že ji uživatel chce použít již v existujícím projektu.

### Hardware

- Knihovna je navržena a plně kompatibilní s mikrokontroléry řady STM32 Nucleo L4xxxx. Funkčnost u ostatních řad není garantovaná. Mikrokontrolér musí mít k dispozici a podporovat:
  1. Rozhraní UART
  2. Rozhraní USB OTG FS
- Bez podpory těchto dvou rozhraní nebude knihovna použitelná.

### Software

Knihovnu pro mikrokontrolér je možno použít samostatně v kterémkoliv zdrojovém kódu, při použití jakéhokoliv vývojového prostředí. Dokumentace je ovšem sepsána s ohledem na použití oficiálního vývojového prostředí CubeIDE, které vyvíjí a poskytuje zdarma přímo firma STMicroelectronics, výrobce mikrokontrolérů. Návod se tedy nezabývá řešením případných problémů spojených s použitím jiného IDE. Je předpokládáno, že uživatel ovládá a používá CubeIDE. 

Knihovna pro desktopovou aplikaci je kompatibilní pouze s prostředím MATLAB. Je nutno mít koupenou/dostupnou validní licenci.

## 3. Účely použití

- Knihovna byla navržena pro usnadnění výuky v předmětech na univerzitě, aby studenti na předmětech nebyli nuceni řešit principy přenosu dat do počítače a mohli řešit pouze práci relevantní pro svůj obor. Knihovna zde slouží primárně jako prostředník pro odesílání načtených dat z logických obvodů a jejich následné zpracování v aplikaci v MATLABu.

- Obecně lze knihovnu v kombinaci s kompatibilním mikrokontrolérem použít pro přenos libovolných proměnných do desktopové aplikace. Následně také z desktopové aplikace odeslat data mikrokontroléru.

## 4. Zprovoznění knihovny pro CubeIDE

### **Krok 1**: Vložení souborů knihovny

- Knihovna pro mikrokontrolér má dva soubory.
  - Hlavičkový comms_data_rxtx.h
  - Zdrojový comms_data_rxtx.c

1. Soubor comms_data_rxtx.h vložit do vášprojekt/Core/Inc/
2. Soubor comms_data_rxtx.c vložit do vášprojekt/Core/Src/

![image](https://github.com/user-attachments/assets/f3e595ac-ad92-4f55-829d-717aa9f2432b)

### **Krok 2**: Include v hlavním souboru projektu

- V hlavím souboru Vašeho existujícího projektu includujte hlavičkový soubor comms_data_rxtx.h

![image](https://github.com/user-attachments/assets/b8a47970-2a60-4a32-b4d3-9bcd104a4168)

### **Krok 3**: Konfigurace rozhraní UART

1. V konfiguraci hardwaru (otevřít .ioc soubor) zvolte sekci Connectivity
2. Z dostupných možností vyberte LPUART1

![image](https://github.com/user-attachments/assets/21a0136f-2c3a-4da1-a6ec-266ce43933fe)

3. Nastavte základní parametry přenosu dle vzoru:

![image](https://github.com/user-attachments/assets/5c7d014a-3657-4779-a11c-8d6cc957a0e3)
 
### **Krok 4**: Konfigurace rozhraní USB OTG FS

1. V sekci Connectivity zvolte USB_OTG_FS
2. Povolte zařízení jako Device_Only

![image](https://github.com/user-attachments/assets/b89c2853-6b54-4748-80eb-ee8b81ce8d66)

3. V sekci Middleware and Software zvolte USB_DEVICE
4. Vyberte ze seznamu typ zařízení VCP - Virtual Com Port

![image](https://github.com/user-attachments/assets/d1b43568-2b9f-4055-8a3f-70b553dc5eb0)

### **Krok 5**: Uložit 

- Uložte novou konfiguraci hardwaru a zvolte ano pro aktualizování a vygenerování obslužného kódu

