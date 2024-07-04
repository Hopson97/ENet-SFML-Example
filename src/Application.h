#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include "Server.h"

class Application
{
  public:
    Application();
    ~Application();

    [[no_discard]] bool on_start();

    void on_event(const sf::RenderWindow& window, const sf::Event& e);
    void on_update(sf::Time dt);
    void on_fixed_update(sf::Time dt);
    void on_render(sf::RenderWindow& window);

  private:
    Server server_;
    ENetHost* client_ = nullptr;
    ENetPeer* peer_ = nullptr;
};