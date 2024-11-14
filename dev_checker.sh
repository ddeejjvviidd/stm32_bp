#!/bin/bash

# Tento skript spravuje práva zařízení pro ttyACM zařízení.
# Obsahuje funkce pro nastavení a kontrolu práv a zobrazuje interaktivní textové menu.

# SUDO kontrola před spuštěním programu
if [[ $EUID -ne 0 ]]; then
   echo "Chyba: Tento skript musí být spuštěn s právy sudo."
   exit 1
fi

# Funkce pro nastavení práv
set_permissions() {
    device=$1
    sudo chmod 777 "$device"
    echo "Práva 777 byla nastavena pro zařízení: $device"
}

# Funkce pro zjištění stavu práv (vrací "yes" pokud jsou práva 777)
check_permissions() {
    device=$1
    perms=$(stat -c "%a" "$device" 2>/dev/null)
    if [[ "$perms" == "777" ]]; then
        echo "yes"
    else
        echo "no"
    fi
}

# Hlavní menu aplikace
show_menu() {
    device_status="waiting"

    # Smyčka zobrazující menu
    while true; do
        clear
        echo "------------------------------------"
        echo "   Správa práv pro ttyACM zařízení  "
        echo "------------------------------------"

        # Vyhledání zařízení
        devices=($(ls /dev/ttyACM* 2>/dev/null))
        if [[ ${#devices[@]} -eq 0 ]]; then
            echo "Žádné zařízení ttyACM není připojeno."
            echo "Čeká se na připojení zařízení..."
            sleep 5
            continue
        else
            device_status="connected"
        fi

        echo "Dostupná zařízení:"
        options=()
        i=1
        for device in "${devices[@]}"; do
            perm_status=$(check_permissions "$device")
            if [[ "$perm_status" == "yes" ]]; then
                options+=("$device - Nastaveno (777)")
            else
                options+=("$device - Nenastaveno")
            fi
            echo "$i) ${options[-1]}"
            ((i++))
        done
        echo "$i) Obnovit seznam zařízení"
        echo "$((i+1))) Ukončit aplikaci"

        # Načtení volby bez potřeby Enteru
        read -n 1 -p "Vyberte možnost: " choice
        echo  # nový řádek pro lepší zobrazení

        # Zpracování výběru
        if [[ "$choice" == "$i" ]]; then
            continue  # Obnoví seznam zařízení
        elif [[ "$choice" == "$((i+1))" ]]; then
            echo "Aplikace ukončena."
            break
        elif [[ "$choice" -ge 1 && "$choice" -lt "$i" ]]; then
            selected_device="${devices[$choice-1]}"
            if [[ -e "$selected_device" ]]; then
                set_permissions "$selected_device"
            else
                echo "Chyba: Zařízení $selected_device bylo odpojeno."
            fi
        else
            echo "Neplatná volba. Zkuste to znovu."
        fi
        sleep 1
    done
}

# Spuštění hlavní funkce
show_menu
