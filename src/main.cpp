#include <iostream>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <enet/enet.h>
#include <imgui.h>
#include <imgui_sfml/imgui-SFML.h>

#include "Application.h"
#include "Util/Profiler.h"

namespace
{
    void handle_event(const sf::Event& e, sf::Window& window, bool& show_profiler);
} // namespace

int main()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "Failed to init ENet.\n";
        return EXIT_FAILURE;
    }

    sf::RenderWindow window({1600, 900}, "ENet Example");
    window.setVerticalSyncEnabled(true);
    window.setActive(true);

    if (!ImGui::SFML::Init(window))
    {
        std::cerr << "Failed to init ImGUI::SFML\n";
        return EXIT_FAILURE;
    }

    sf::Clock clock;
    Profiler profiler;
    bool show_profiler = false;
    bool option_selected = false;

    Application app(window);

    while (window.isOpen())
    {
        for (sf::Event e{}; window.pollEvent(e);)
        {
            ImGui::SFML::ProcessEvent(e);
            app.on_event(window, e);
            handle_event(e, window, show_profiler);
        }
        auto dt = clock.restart();
        // Update
        ImGui::SFML::Update(window, dt);

        if (!option_selected)
        {
            if (ImGui::Begin("Connect"))
            {
                if (ImGui::Button("Host"))
                {
                    app.init_as_host();
                    option_selected = true;
                }
                else if (ImGui::Button("Client"))
                {
                    app.init_as_client();
                    option_selected = true;
                }
            }
            ImGui::End();
        }

        // Update
        {
            auto& update_profiler = profiler.begin_section("Update");
            app.on_update(dt);
            update_profiler.end_section();
        }


        // Render
        window.clear();
        {
            auto& render_profiler = profiler.begin_section("Render");
            app.on_render(window);
            render_profiler.end_section();
        }

        // Show profiler
        profiler.end_frame();
        if (show_profiler)
        {
            profiler.gui();
        }

        // End frame...
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown(window);
    enet_deinitialize();

    return EXIT_SUCCESS;
}

namespace
{
    void handle_event(const sf::Event& e, sf::Window& window, bool& show_profiler)
    {
        switch (e.type)
        {
            case sf::Event::Closed:
                window.close();
                break;

            case sf::Event::KeyReleased:
                switch (e.key.code)
                {
                    case sf::Keyboard::Escape:
                        window.close();
                        break;

                    case sf::Keyboard::F1:
                        show_profiler = !show_profiler;
                        break;

                    default:
                        break;
                }
                break;

            default:
                break;
        }
    }
} // namespace