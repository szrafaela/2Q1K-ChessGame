#pragma once
#include <string>
#include "Piece.h"

class Player {
public:
    Player(const std::string& name, Color color);
    std::string getName() const;
    Color getColor() const;
    void setName(const std::string& newName);

private:
    std::string name;
    Color color;
};
