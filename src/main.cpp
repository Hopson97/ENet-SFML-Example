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
    void handle_event(const sf::Event& event, sf::Window& window, bool& show_debug_info,
                      bool& close_requested);
} // namespace
int main()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "Failed to init ENet.\n";
        return EXIT_FAILURE;
    }

    sf::RenderWindow window(sf::VideoMode({1600, 900}),
                            "PROJECT_NAME_PLACEHOLDER - Press F1 for debug");
    window.setVerticalSyncEnabled(true);
    if (!ImGui::SFML::Init(window))
    {
        std::println(std::cerr, "Failed to init ImGUI::SFML.");
        return EXIT_FAILURE;
    }


    sf::Clock clock;
    Profiler profiler;
    bool show_profiler = false;
    bool option_selected = false;

    Application app;

    while (window.isOpen())
    {
        bool close_requested = false;

        while (auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);
            app.on_event(window, *event);
            handle_event(*event, window, show_profiler, close_requested);
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
        if (close_requested)
        {
            window.close();
        }

    }

    ImGui::SFML::Shutdown(window);
    enet_deinitialize();

    return EXIT_SUCCESS;
}

namespace
{
    void handle_event(const sf::Event& event, sf::Window& window, bool& show_debug_info,
                      bool& close_requested)
    {
        if (event.is<sf::Event::Closed>())
        {
            close_requested = true;
        }
        else if (auto* key = event.getIf<sf::Event::KeyPressed>())
        {
            switch (key->code)
            {
                case sf::Keyboard::Key::Escape:
                    close_requested = true;
                    break;

                case sf::Keyboard::Key::F1:
                    show_debug_info = !show_debug_info;
                    break;

                default:
                    break;
            }
        }
    }
} // namespace