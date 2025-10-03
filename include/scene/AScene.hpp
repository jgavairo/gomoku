#pragma once
#include "scene/Context.hpp"
#include <SFML/Graphics.hpp>
#include <functional>
#include "ui/Button.hpp"

namespace gomoku::scene {

class AScene {
public:
    explicit AScene(Context& context);
    virtual ~AScene() = default;

    virtual void onEnter() { }
    virtual void onExit() { }
    virtual void onThemeChanged() { }

    virtual bool handleInput(sf::Event& event) = 0;
    virtual void update(sf::Time& deltaTime) = 0;
    virtual void render(sf::RenderTarget& target) const = 0;

protected:
    void playSfx(const char* name, float volume = 100.f) const;
    void playMusic(const char* path, bool loop = true, float volume = 60.f) const;
    void initButtons(gomoku::ui::Button& btn, const char* key, sf::Vector2f pos, float scale, const std::function<void()>& callback);
    Context& context_;
};

} // namespace gomoku::scene

