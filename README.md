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
5. Zprovoznění knihovny pro MATLAB
6. Princip
7. Použití knihovny pro mikrokontrolér
8. Použití knihovny pro MATLAB


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

### **Krok 6**: Předání dat knihovně

1. Přejděte do souboru vášprojekt/USB_DEVICE/App/usbd_cdc_if.c
2. Najděte v něm funkci CDC_Receive_FS();
3. Ve funkci připište následující řádek:

![image](https://github.com/user-attachments/assets/85a10a1c-a7f6-4399-83c9-e25defbe4aec)


## 5. Zprovoznění knihovny pro MATLAB

- Soubor comms_data_rxtx.m je nutné pouze vložit do stejného adresáře, jako je Vaše aplikace v MATLABu

![image](https://github.com/user-attachments/assets/6670a7e3-2ebd-463f-9693-25e7baaa7189)

## 6. Princip

- Knihovna umožňuje odesílání dat ve větších blocích, aby se zvýšila rychlost a efektivita přenosu.
- Uživatel přidává pomocí funkcí datové typy do vyrovnávací paměti (bufferu) v knihovně.
- Data jsou odeslána jako celek až po zavolání funkce pro odeslání dat.
- S každým datovým typem se odesílá i uživatelem zvolené ID, dle kterého je možno data na druhé straně zpracovat dle libosti.


## 7. Použití knihovny pro mikrokontrolér

### **Přidání dat pro odeslání**

Pro přidání dat k odeslání slouží funkce comms_append_int32();, které je nutno jako parametry předat:
- Zvolené ID dat
- Počet dat
- Ukazatel na data

Příklad přidání dat: 

![image](https://github.com/user-attachments/assets/80253eb1-e4cb-436c-8fff-42f579269102)

### **Odeslání dat** 

- Pro odeslání dat slouží funkce comms_send();
- Je optimální volat ji například z nekonečné smyčky programu, v případě přidání dat kdykoliv za běhu programu tak budou data vždy odeslána

![image](https://github.com/user-attachments/assets/d540f00d-df83-4e60-9441-34d5d330b5ee)

### **Příjem dat**

- Pro příjem dat slouží funkce comms_data_handler();
- Funkce je volána pro každou přijatou proměnnou.
- Funkci lze předefinovat ve Vašem uživatelském kódu, implementaci v knihovně pak bude kompilátor ignorovat, slouží přimárně jako příklad.
- Funkci je předávána vytvořená struktura obsahující načtená data, lze k nim přistupovat pomocí ukazatelů:

![image](https://github.com/user-attachments/assets/9f502ef3-7eda-4023-8686-a04cb6934113)

Struktura má následující podobu, obsahuje:
1. data_id (zvolené ID dat)
2. data_size (velikost v bajtech)
3. data_count (počet datových typů)
4. data (datové typy)

Dle data_size lze ve stavovém automatu rozklíčovat, zda se má ke konkrétním datům přistupovat jako k 4, 2 nebo 1 bajtovým proměnným.

![image](https://github.com/user-attachments/assets/9ed6cc80-c0cb-47e5-839d-be582658415f)

