#include <iostream>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <enet/enet.h>
#include <imgui.h>
#include <imgui_sfml/imgui-SFML.h>

#include "Application.h"
#include "Util/Array2D.h"
#include "Util/Profiler.h"
#include "Util/TimeStep.h"

int main()
{
    if (enet_initialize() != 0)
    {
        std::cerr << "Failed to init ENet.\n";
        return EXIT_FAILURE;
    }

    sf::RenderWindow window({1280, 720}, "SFML");
    window.setVerticalSyncEnabled(true);
    window.setActive(true);

    if (!ImGui::SFML::Init(window))
    {
        std::cerr << "Failed to init ImGUI::SFML\n";
        return EXIT_FAILURE;
    }

    sf::Clock clock;
    TimeStep fixed_updater{50};
    Profiler profiler;
    bool show_profiler = false;
    bool option_selected = false;

    Application app;

    while (window.isOpen())
    {
        for (sf::Event e{}; window.pollEvent(e);)
        {
            ImGui::SFML::ProcessEvent(e);
            app.on_event(window, e);
            if (e.type == sf::Event::Closed)
            {
                window.close();
            }
            else if (e.type == sf::Event::KeyReleased && e.key.code == sf::Keyboard::F1)
            {
                show_profiler = !show_profiler;
            }
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


        {
            auto& update_profiler = profiler.begin_section("Update");
            app.on_update(dt);
            update_profiler.end_section();
        }

        // Fixed-rate update
        {
            auto& fixed_update_profiler = profiler.begin_section("Fixed Update");
            fixed_updater.update([&](sf::Time dt) { app.on_fixed_update(dt); });
            fixed_update_profiler.end_section();
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