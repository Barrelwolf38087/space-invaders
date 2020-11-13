#include <iostream>
#include <vector>
#include <list>

#include <SFML/Graphics.hpp>

// Embedded textures made with the xxd utility
#include "player.hpp"
#include "enemy.hpp"

// Compile-time constants
constexpr int winWidth = 1280;
constexpr int winHeight = 720;
constexpr int bulletWidth = 20;
constexpr int bulletHeight = 80;

constexpr int margin = 25;
constexpr int padding = 25;

constexpr float playerSpeed = 300;
constexpr float enemySpeed = 150;
constexpr float bulletSpeed = 700;

constexpr size_t numEnemies = 40;

class Application {
    using Event = sf::Event;

    sf::RenderWindow _win;
    sf::Image _windowIcon;
    sf::Texture _enemyTexture;
    sf::Texture _playerTexture;

    std::vector<sf::Sprite> _enemies;
    std::vector<sf::RectangleShape> _bullets;
    sf::Sprite _player;

    sf::Clock _clock;
    sf::Clock _fireCooldown;

    // 1 or -1 for left or right
    int _enemyDirection{1};
    bool _quit = false;
    bool _lost = false;

    void fire() {
        _bullets.emplace_back(sf::Vector2f{bulletWidth, bulletHeight});
        _bullets.back().setPosition(_player.getPosition().x + (0.5f * _player.getGlobalBounds().width) - (0.5f * bulletWidth),
                                    _win.getView().getSize().y - _player.getGlobalBounds().height - bulletHeight);
    }

    /**
     * Handles all of the game logic
     */
    void update() {

        // Used to keep a consistent movement speed regardless of framerate
        static sf::Time elapsed;

        elapsed = _clock.getElapsedTime();
        _clock.restart();

        // Event handling
        static sf::Event e{};
        while (_win.pollEvent(e)) {
            switch (e.type) {
                case Event::Closed:
                    _quit = true;
                    break;
                case Event::KeyPressed:
                    if (e.key.code == sf::Keyboard::Space && !_lost) {
                        sf::Time sinceLastFire{_fireCooldown.getElapsedTime()};

                        // So the player can't just hold space to instantly win
                        if (sinceLastFire.asMilliseconds() > 100) {
                            _fireCooldown.restart();
                            fire();
                        }
                    }
                default:
                    break;
            }
        }

        static float playerPotentialX;

        // Polling input for player movement
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            playerPotentialX = _player.getPosition().x - playerSpeed * elapsed.asSeconds();
            if (playerPotentialX < 0) {
                playerPotentialX = 0;
            }
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            playerPotentialX = _player.getPosition().x + playerSpeed * elapsed.asSeconds();
            if (playerPotentialX + _player.getGlobalBounds().width > _win.getView().getSize().x) {
                playerPotentialX = _win.getView().getSize().x - _player.getGlobalBounds().width;
            }
        }

        _player.setPosition(playerPotentialX, _player.getPosition().y);

        // Move the bullets up
        for (auto& bullet : _bullets) {
            bullet.setPosition(bullet.getPosition().x, bullet.getPosition().y - (elapsed.asSeconds() * bulletSpeed));
        }

        // Clean up bullets that leave the screen
        for (size_t i = 0; i < _bullets.size(); ++i) {
            if (_bullets[i].getPosition().y < -bulletHeight) {
                _bullets.erase(_bullets.begin() + i);
                --i;
            }
        }

        // If a bullet hits an enemy, delete them both
        for (size_t i = 0; i < _enemies.size(); ++i) {
            for (size_t j = 0; j < _bullets.size(); ++j) {
                if (_enemies[i].getGlobalBounds().intersects(_bullets[j].getGlobalBounds())) {
                    _enemies.erase(_enemies.begin() + i--);
                    _bullets.erase(_bullets.begin() + j--);

                    // Check for win condition
                    if (_enemies.empty()) {
                        std::cout << "You win!" << std::endl;
                    }
                }
            }
        }

        // Move all the enemies sideways
        bool shift;
        for (auto& enemy : _enemies) {
            enemy.setPosition(enemy.getPosition().x + (static_cast<float>(_enemyDirection) * enemySpeed * elapsed.asSeconds()),
                              enemy.getPosition().y);

            if (enemy.getPosition().x <= margin || enemy.getPosition().x + enemy.getGlobalBounds().width > _win.getView().getSize().x - margin) {
                shift = true;
            }
        }

        // If they moved over the margin, shift them down
        if (shift) {

            // Reverse the direction
            _enemyDirection *= -1;

            // Shift them down by the enemy height
            for (auto& enemy : _enemies) {
                enemy.setPosition(enemy.getPosition().x, enemy.getPosition().y + enemy.getGlobalBounds().height);

                // Check for lose condition
                if (enemy.getPosition().y + enemy.getGlobalBounds().height > _win.getView().getSize().y && !_lost) {
                    std::cout << "You lose!" << std::endl;
                    _lost = true;
                }
            }
        }
    }

    // Draws everything after the game logic has been updated
    void render() {
        _win.clear();
        _win.draw(_player);

        for (auto& enemy : _enemies) {
            _win.draw(enemy);
        }

        for (auto& bullet : _bullets) {
            _win.draw(bullet);
        }

        _win.display();
    }

public:
    Application() :
            _win(sf::VideoMode(winWidth, winHeight), "Bad Space Invaders", sf::Style::Default | sf::Style::Resize) {

        // Load the window icon from the header file
        _windowIcon.loadFromMemory(enemy_png, enemy_png_len);
        _win.setIcon(_windowIcon.getSize().x, _windowIcon.getSize().y, _windowIcon.getPixelsPtr());

        // Prevents screen tearing
        _win.setVerticalSyncEnabled(true);

        // Load the enemy and player textures from the header file
        _enemyTexture.loadFromImage(_windowIcon);
        _playerTexture.loadFromMemory(player_png, player_png_len);

        _player.setTexture(_playerTexture);
        _player.setScale(0.25, 0.25);
        _player.setPosition(0, _win.getView().getSize().y - _player.getGlobalBounds().height);

        // Generate all the enemies
        _enemies.reserve(numEnemies);
        for (int i = 0; i < 40; ++i) {
            // Constructs the object in the vector instead of copying it, which is pretty neat
            _enemies.emplace_back();

            sf::Sprite& enemy = _enemies.back();
            enemy.setTexture(_enemyTexture);
            enemy.setScale(0.5, 0.5);

            // Integer division works to our advantage here
            int row = i / 10;

            // I know the casts are ugly; that's considered best practice in C++ because casts can cause problems if
            // you're not careful.
            enemy.setPosition(static_cast<float>(static_cast<float>(i % 10) * enemy.getGlobalBounds().width) + static_cast<float>((i % 10) * padding) + margin,
                              static_cast<float>(row) * enemy.getGlobalBounds().height + static_cast<float>(row * padding) + margin);
        }

    }

    /**
     * Initialize the game
     */
    int run() {
        // If we don't do this, the first call to getElapsedTime might return a bigger value than we expect, which could
        // cause a stutter at the start.
        _clock.restart();
        _fireCooldown.restart();

        while (_win.isOpen()) {
            update();
            render();

            if (_quit) {
                _win.close();
            }
        }

        return 0;
    }
};

int main() {
    Application spaceInvaders{};

    return spaceInvaders.run();
}
