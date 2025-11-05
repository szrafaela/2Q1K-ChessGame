#include "Game.h"
#include <iostream>
#include <fstream>

int main() {
    Game game;

    // 🔹 1. Betöltés JSON-ból (ha létezik)
    std::ifstream infile("savegame.json");
    if (infile.good()) {
        std::cout << "Korábbi mentés betöltése..." << std::endl;
        game.loadFromFile("savegame.json");
    } else {
        std::cout << "Új játék indítása..." << std::endl;
        game.start();
    }

    std::cout << "Sakkjáték elindult!" << std::endl;

    // 🔹 2. Szimulált lépések vagy interakció (ide jönne a UI vagy input kezelés)
    // Példa: automatikus mentés demonstrációja
    std::cout << "A játék fut. Kilépéshez nyomj Entert..." << std::endl;
    std::cin.get();

    // 🔹 3. Automatikus mentés kilépéskor
    std::cout << "Játék mentése JSON fájlba..." << std::endl;
    game.saveToFile("savegame.json");

    std::cout << "Mentés kész. Viszlát!" << std::endl;
    return 0;
}
