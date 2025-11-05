#include "Player.h"

Player::Player(const std::string& name, Color color)
    : name(name), color(color) {}

std::string Player::getName() const { return name; }
Color Player::getColor() const { return color; }