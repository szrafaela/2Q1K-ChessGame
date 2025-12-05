# Modern szoftverfejlesztési eszközök (GKNB_INTM006) 2025/26 1.Félév

---

## 2Q1K

### A játék rövid leírása

A 2Q1K-ChessGame egy kétjátékos sakkprogram, amelyet három egyetemi hallgató – két lány és egy fiú – közösen fejlesztett C++ nyelven. A játék célja, hogy egyszerű, szabályos és élvezetes sakkélményt nyújtson lokális játékosok számára. A projekt a klasszikus sakk szabályait követi, és konzolos felületen keresztül teszi lehetővé a játékot. A név – 2Q1K – kreatívan utal a csapat összetételére: két királynő és egy király.

## Történetleírás

A projekt egy kétszemélyes, digitális sakkjáték, amely lehetővé teszi, hogy két felhasználó valós időben, egymás ellen játszhasson egy interaktív, virtuális sakktáblán. A játék a klasszikus sakk szabályait követi: minden figura a hagyományos módon mozog, a cél pedig az ellenfél királyának mattolása. A rendszer biztosítja a felváltott lépéseket, automatikusan ellenőrzi azok szabályosságát, és visszajelzést ad sakk vagy sakkmatt esetén. A játékosok visszavonhatják lépéseiket, menthetik az aktuális állást, és később folytathatják a játszmát. A felület intuitív és letisztult, támogatva a stratégiai gondolkodás fejlődését.

---

## A játék működése

A játék két felhasználó között zajlik valós időben egy virtuális sakktáblán. A játékosok felváltva lépnek, a rendszer minden lépést automatikusan ellenőriz a klasszikus sakk szabályai szerint. A figurák interaktívan mozgathatók, és a rendszer visszajelzést ad sakk, sakkmatt vagy érvénytelen lépés esetén. Lehetőség van lépések visszavonására, a játékállás mentésére és későbbi betöltésére. A játszma automatikusan lezárul, ha matt vagy döntetlen állapot alakul ki, és a rendszer megjeleníti az eredményt.

## Technikai Specifikációk

### A játék által kezelt könyvtár

A játék a szabálykezeléshez, lépések nyilvántartásához és mentéséhez saját belső könyvtárakat használ, amelyek a figurák mozgását, a tábla állapotát és a játék logikáját kezelik.

### Tesztesetek

A játék működésének tesztelése során ellenőrizzük a szabályos és szabálytalan lépéseket, a sakk- és matthelyzetek felismerését, a lépések visszavonását, valamint a mentés és betöltés funkciók helyes működését.

# Build windows-on:
cmake -S . -B build-win -G "Ninja" -DCMAKE_BUILD_TYPE=Release

cmake --build build-win --config Release

# Tesztek futtatása windows-on:
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

cmake --build build --config Debug

ctest --test-dir build -C Debug

# Docker build: 

docker build -t chessgame .

# Stockfish
A gép ellen való játékhoz szükséges letölteni a stockfish binárist, docker image-be bele van építve, docker esetén müködik out of the box.
