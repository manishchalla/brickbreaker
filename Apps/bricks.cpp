#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <string>
#include "GLUT/glut.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const float M_PI = 3.1415926f;
const int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;
const int BRICK_ROWS = 5, BRICK_COLS = 10;
const int BRICK_WIDTH = 58, BRICK_HEIGHT = 20;
const int BRICK_SPACING = 2;
const int PADDLE_WIDTH = 100, PADDLE_HEIGHT = 20;
const int BALL_RADIUS = 10;
const int FIREBALL_DURATION = 5; // Duration of fireball state in seconds
const float BALL_SPEED_L1 = 0.4f; // Ball speed for level 1
const float BALL_SPEED_L2 = 0.5f; // Ball speed for level 2
const float BALL_SPEED_L3 = 0.6f; // Ball speed for level 3
const float PADDLE_SPEED = 0.4f; // Paddle Speed
const float BULLET_SPEED = 0.5f; // Speed for bullets
const int SHRINK_DURATION = 5; // Duration of the shrink state in seconds
const float SHRINK_FACTOR = 0.5f; // Paddle Shrink
int bricksBroken = 0; // Track the number of bricks broken
int lives = 1; // Initialize lives
bool resetAfterLifeLost = false; // Flag to reset the ball after losing a life


GLuint brickTexture1, brickTexture2, brickTexture3;
GLuint fireballTexture;

enum GameState { MENU, AIM, GAME, GAME_OVER, WIN, LEVEL_SELECT, LEVEL_COMPLETE, EXIT, INSTRUCTIONS, POWER_UPS };

GameState currentGameState = MENU;
int menuSelection = 0;
float aimAngle = M_PI / 6; // Initial aim angle
const float minAimAngle = M_PI / 6; // 30 degrees
const float maxAimAngle = 5 * M_PI / 6; // 150 degrees

enum PowerUpType { FIREBALL = 1, GUN, FLIP, SHRINK };

struct Brick {
    float x, y, width, height;
    int type;
    int hitsRemaining;
    bool isVisible;
    bool hasPowerUp;
    PowerUpType powerUpType; // Type power-up
    bool isBomb;
    Brick(float x, float y, float w, float h, int t, bool powerUp = false, PowerUpType pType = FIREBALL, bool bomb = false)
        : x(x), y(y), width(w), height(h), type(t), isVisible(true), hasPowerUp(powerUp), powerUpType(pType), isBomb(bomb) {
        switch (type) {
        case 1: hitsRemaining = 1; break; // 1hit
        case 2: hitsRemaining = 2; break; // 2hit
        case 3: hitsRemaining = 3; break; // 3hit
        }
    }
};

struct PowerUp {
    float x, y; // Position power-up
    float dx, dy; // Velocity power-up
    bool active; // State power-up
    float radius; // Radius power-up
    PowerUpType type; // powerup type
    PowerUp(float _x, float _y, float _dx, float _dy, PowerUpType _type) : x(_x), y(_y), dx(_dx), dy(_dy), active(true), radius(10.0f), type(_type) {}
};

struct Bullet {
    float x, y, dy;
    bool active;
    Bullet(float _x, float _y) : x(_x), y(_y), dy(-BULLET_SPEED), active(true) {}
};

struct Ball {
    float x, y, dx, dy, radius;
    bool idle;
    bool isFireball;
    bool hasGun;
    int gunAmmo;
    time_t powerUpStartTime;
    Ball() : x(SCREEN_WIDTH / 2), y(SCREEN_HEIGHT - 50), dx(0.4f), dy(-0.4f), radius(BALL_RADIUS), idle(true), isFireball(false), hasGun(false), gunAmmo(0), powerUpStartTime(0) {
        normalizeVelocity();
    }

    void normalizeVelocity() {
        float length = sqrt(dx * dx + dy * dy);
        dx = (dx / length) * BALL_SPEED_L2;
        dy = (dy / length) * BALL_SPEED_L2;
    }

    void setSpeed(float speed) {
        float length = sqrt(dx * dx + dy * dy);
        dx = (dx / length) * speed;
        dy = (dy / length) * speed;
    }

    void applyPowerUp(float multiplier) {
        dx *= multiplier;
        dy *= multiplier;
    }

    void resetPowerUp() {
        isFireball = false;
        hasGun = false;
        gunAmmo = 0;
        normalizeVelocity();
    }
};

struct Paddle {
    float x, y, width, height;
    bool moveLeft, moveRight, controlsFlipped, isShrunk;
    time_t shrinkStartTime;
    Paddle() : x(SCREEN_WIDTH / 2 - PADDLE_WIDTH / 2), y(SCREEN_HEIGHT - 30), width(PADDLE_WIDTH), height(PADDLE_HEIGHT), moveLeft(false), moveRight(false), controlsFlipped(false), isShrunk(false), shrinkStartTime(0) {}
};

std::vector<Brick> bricks;
std::vector<PowerUp> powerUps; //To store power-ups
std::vector<Bullet> bullets; // To store bullets
Ball ball;
Paddle paddle;
int score = 0; // Initialize score variable
int level = 1; // Current game level

GLuint bombTexture;

void loadBombTexture() {
    int width, height, channels;
    unsigned char* data = stbi_load("Image/bomb.png", &width, &height, &channels, 0);
    if (data) {
        glGenTextures(1, &bombTexture);
        glBindTexture(GL_TEXTURE_2D, bombTexture);
        if (channels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else if (channels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else {
        std::cerr << "Failed to load bomb texture" << std::endl;
    }
}


void setupBricks() {
    float startX = (SCREEN_WIDTH - (BRICK_COLS * (BRICK_WIDTH + BRICK_SPACING))) / 2.0f;
    float startY = 50;
    int bombCount = 0;
    int maxBombs;

    int totalBricks = BRICK_ROWS * BRICK_COLS;
    int fireballCount, gunCount, flipCount, shrinkCount;

    switch (level) {
    case 1:
        fireballCount = totalBricks * 0.20; // 20% Fireball power-ups
        gunCount = 0;
        flipCount = 0;
        shrinkCount = 0;
        maxBombs = 8 + rand() % 2; // 8-9 bombs
        break;
    case 2:
        fireballCount = totalBricks * 0.15; // 15% Fireball power-ups
        shrinkCount = totalBricks * 0.15; // 15% Shrink power-ups
        gunCount = totalBricks * 0.10; // 10% Gun power-ups
        flipCount = 0;
        maxBombs = 3 + rand() % 2; // 3-4 bombs
        break;
    case 3:
        fireballCount = totalBricks * 0.10; // 10% Fireball power-ups
        shrinkCount = totalBricks * 0.10; // 10% Shrink power-ups
        gunCount = totalBricks * 0.10; // 10% Gun power-ups
        flipCount = totalBricks * 0.10; // 10% Flip power-ups
        maxBombs = 2 + rand() % 2; // 2-3 bombs
        break;
    default:
        fireballCount = 0;
        gunCount = 0;
        flipCount = 0;
        shrinkCount = 0;
        maxBombs = 0;
    }

    std::vector<int> brickIndices(totalBricks);
    for (int i = 0; i < totalBricks; ++i) {
        brickIndices[i] = i;
    }
    std::random_shuffle(brickIndices.begin(), brickIndices.end());

    auto assignPowerUps = [&](int& count, PowerUpType type) {
        while (count > 0) {
            int index = brickIndices.back();
            brickIndices.pop_back();
            int row = index / BRICK_COLS;
            int col = index % BRICK_COLS;
            bricks[row * BRICK_COLS + col].hasPowerUp = true;
            bricks[row * BRICK_COLS + col].powerUpType = type;
            --count;
        }
        };

    bricks.reserve(totalBricks);
    for (int i = 0; i < BRICK_ROWS; ++i) {
        for (int j = 0; j < BRICK_COLS; ++j) {
            int type;
            bool isBomb = false;

            switch (level) {
            case 1:
                type = (rand() % 100 < 25) ? 2 : 1; // 25% 2-hit bricks, rest 1-hit bricks
                break;
            case 2:
            {
                int randVal = rand() % 100;
                if (randVal < 30) {
                    type = 2; // 30% 2-hit bricks
                }
                else if (randVal < 40) {
                    type = 3; // 10% 3-hit bricks
                }
                else {
                    type = 1; // rest 1-hit bricks
                }
            }
            break;
            case 3:
            {
                int randVal = rand() % 100;
                if (randVal < 30) {
                    type = 3; // 30% 3-hit bricks
                }
                else if (randVal < 70) {
                    type = 2; // 40% 2-hit bricks
                }
                else {
                    type = 1; // rest 1-hit bricks
                }
            }
            break;
            default:
                type = 1;
            }

            bricks.emplace_back(startX + j * (BRICK_WIDTH + BRICK_SPACING), startY + i * (BRICK_HEIGHT + BRICK_SPACING), BRICK_WIDTH, BRICK_HEIGHT, type, false, FIREBALL, false);
        }
    }

    // Place bombs ensuring they are not adjacent
    std::vector<int> availableIndices;
    for (int i = 0; i < totalBricks; ++i) {
        availableIndices.push_back(i);
    }
    std::random_shuffle(availableIndices.begin(), availableIndices.end());

    while (bombCount < maxBombs && !availableIndices.empty()) {
        int index = availableIndices.back();
        availableIndices.pop_back();
        int row = index / BRICK_COLS;
        int col = index % BRICK_COLS;

        // Check adjacent bricks
        bool canPlaceBomb = true;
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                int newRow = row + dr;
                int newCol = col + dc;
                if (newRow >= 0 && newRow < BRICK_ROWS && newCol >= 0 && newCol < BRICK_COLS) {
                    int adjacentIndex = newRow * BRICK_COLS + newCol;
                    if (bricks[adjacentIndex].isBomb) {
                        canPlaceBomb = false;
                        break;
                    }
                }
            }
            if (!canPlaceBomb) break;
        }

        if (canPlaceBomb) {
            bricks[index].isBomb = true;
            bombCount++;
        }
    }

    assignPowerUps(fireballCount, FIREBALL);
    assignPowerUps(gunCount, GUN);
    assignPowerUps(flipCount, FLIP);
    assignPowerUps(shrinkCount, SHRINK); // Assign the SHRINK power-up

    std::cout << "Total bombs placed: " << bombCount << std::endl; // Debug print for bombs count
}



void loadBrickTextures() {
    int width, height, channels;
    unsigned char* data;

    // Load stage1.png
    data = stbi_load("Image/stage1.png", &width, &height, &channels, 0);
    if (data) {
        glGenTextures(1, &brickTexture1);
        glBindTexture(GL_TEXTURE_2D, brickTexture1);
        if (channels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else if (channels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else {
        std::cerr << "Failed to load stage1 texture" << std::endl;
    }

    // Load stage2.png
    data = stbi_load("Image/stage2.png", &width, &height, &channels, 0);
    if (data) {
        glGenTextures(1, &brickTexture2);
        glBindTexture(GL_TEXTURE_2D, brickTexture2);
        if (channels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else if (channels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else {
        std::cerr << "Failed to load stage2 texture" << std::endl;
    }

    // Load stage3.jpg
    data = stbi_load("Image/stage3.jpg", &width, &height, &channels, 0);
    if (data) {
        glGenTextures(1, &brickTexture3);
        glBindTexture(GL_TEXTURE_2D, brickTexture3);
        if (channels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else if (channels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else {
        std::cerr << "Failed to load stage3 texture" << std::endl;
    }
}

void drawBrick(const Brick& brick) {
    if (!brick.isVisible) return;

    if (brick.isBomb) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, bombTexture);
        glColor3f(1.0f, 1.0f, 1.0f); // Reset color to white for texture

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(brick.x, brick.y);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(brick.x + brick.width, brick.y);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(brick.x + brick.width, brick.y + brick.height);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(brick.x, brick.y + brick.height);
        glEnd();

        glDisable(GL_TEXTURE_2D);
    }
    else {
        GLuint textureID;
        switch (brick.hitsRemaining) {
        case 1: textureID = brickTexture1; break;
        case 2: textureID = brickTexture2; break;
        case 3: textureID = brickTexture3; break;
        default: return; // No texture for this brick
        }

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor3f(1.0f, 1.0f, 1.0f); // Reset color to white for texture

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(brick.x, brick.y);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(brick.x + brick.width, brick.y);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(brick.x + brick.width, brick.y + brick.height);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(brick.x, brick.y + brick.height);
        glEnd();

        glDisable(GL_TEXTURE_2D);
    }
}

void destroySurroundingBricks(int bombIndex) {
    int row = bombIndex / BRICK_COLS;
    int col = bombIndex % BRICK_COLS;

    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue;
            int newRow = row + i;
            int newCol = col + j;

            if (newRow >= 0 && newRow < BRICK_ROWS && newCol >= 0 && newCol < BRICK_COLS) {
                int index = newRow * BRICK_COLS + newCol;
                if (bricks[index].isVisible) {
                    bricks[index].isVisible = false;
                    score += 10;
                }
            }
        }
    }
}

void checkCollisions() {
    // Check ball and brick collisions
    for (int i = 0; i < bricks.size(); ++i) {
        auto& brick = bricks[i];
        if (brick.isVisible && ball.x + ball.radius > brick.x && ball.x - ball.radius < brick.x + brick.width &&
            ball.y + ball.radius > brick.y && ball.y - ball.radius < brick.y + brick.height) {

            float brickCenterX = brick.x + brick.width / 2.0f;
            float brickCenterY = brick.y + brick.height / 2.0f;

            if (brick.isBomb) {
                destroySurroundingBricks(i);
                brick.isVisible = false;
                score += 10;
                bricksBroken++;
            }
            else if (ball.isFireball) {
                brick.isVisible = false;
                score += 10; // Score for destroying brick
                bricksBroken++;
                if (brick.hasPowerUp) {
                    powerUps.emplace_back(brick.x + brick.width / 2, brick.y + brick.height / 2, 0.0f, 0.05f, brick.powerUpType); // Adjust the dy value to 0.05f for slower falling speed
                }
            }
            else {
                brick.hitsRemaining--;
                if (brick.hitsRemaining <= 0) {
                    brick.isVisible = false;
                    score += 10; // Score for destroying brick
                    bricksBroken++;
                    if (brick.hasPowerUp) {
                        powerUps.emplace_back(brick.x + brick.width / 2, brick.y + brick.height / 2, 0.0f, 0.05f, brick.powerUpType); // Adjust the dy value to 0.05f for slower falling speed
                    }
                }

                // Reflect ball based on hit position
                float hitX = (ball.x - brickCenterX) / brick.width;
                float hitY = (ball.y - brickCenterY) / brick.height;

                if (fabs(hitX) > fabs(hitY)) {
                    ball.dx = -ball.dx; // Horizontal collision
                }
                else {
                    ball.dy = -ball.dy; // Vertical collision
                }

                ball.normalizeVelocity(); // Ensure ball doesn't change speed after collision
            }

            // Increase ball speed based on level and number of bricks broken
            if (level == 2 && bricksBroken % 20 == 0) {
                ball.setSpeed(ball.dx * 1.1f);
            }
            else if (level == 3 && bricksBroken % 15 == 0) {
                ball.setSpeed(ball.dx * 1.1f);
            }

            break;
        }
    }

    // Check bullet and brick collisions
    for (auto& bullet : bullets) {
        if (!bullet.active) continue;
        for (auto& brick : bricks) {
            if (brick.isVisible && bullet.x > brick.x && bullet.x < brick.x + brick.width &&
                bullet.y > brick.y && bullet.y < brick.y + brick.height) {
                brick.isVisible = false;
                bullet.active = false;
                score += 50; // Score for destroying brick with Gun
                bricksBroken++;
                break;
            }
        }
    }

    // Check ball and paddle collisions
    if (ball.y + ball.radius >= paddle.y && ball.y - ball.radius <= paddle.y + paddle.height &&
        ball.x + ball.radius >= paddle.x && ball.x - ball.radius <= paddle.x + paddle.width) {
        ball.dy = -fabs(ball.dy); // Always reflect the ball upward

        // Adjust the ball's horizontal direction based on where it hits the paddle
        float hitPos = (ball.x - paddle.x) / paddle.width;
        ball.dx = (hitPos - 0.5f) * 2 * BALL_SPEED_L1;

        ball.normalizeVelocity(); // Keep the ball speed constant after collision
    }

    // Wall collisions
    if (ball.x - ball.radius < 0 || ball.x + ball.radius > SCREEN_WIDTH) {
        ball.dx = -ball.dx; // Bounce off left and right walls
        ball.normalizeVelocity(); // Keep the ball speed constant after collision
    }
    if (ball.y - ball.radius < 0) {
        ball.dy = -ball.dy; // Bounce off top wall
        ball.normalizeVelocity(); // Keep the ball speed constant after collision
    }
    if (ball.y + ball.radius > SCREEN_HEIGHT) {
        // Ball goes out of bounds

        if (lives > 0) {
            resetAfterLifeLost = true;
            ball.idle = true;
            ball.x = SCREEN_WIDTH / 2; // Reset ball position
            ball.y = SCREEN_HEIGHT - 50;
            ball.dx = 0.4f; // Reset ball speed
            ball.dy = -0.4f;
            paddle.x = (SCREEN_WIDTH - PADDLE_WIDTH) / 2; // Reset paddle position
            currentGameState = AIM; // Set game state to AIM to show aiming line
        }
        else {
            currentGameState = GAME_OVER;
        }
        lives--;
    }
}

GLuint bulletTexture;

void loadBulletTexture() {
    int width, height, channels;
    unsigned char* data = stbi_load("Image/bullet1.png", &width, &height, &channels, 0);
    if (data) {
        glGenTextures(1, &bulletTexture);
        glBindTexture(GL_TEXTURE_2D, bulletTexture);
        if (channels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else if (channels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else {
        std::cerr << "Failed to load bullet texture" << std::endl;
    }
}

void loadFireballTexture() {
    int width, height, channels;
    unsigned char* data = stbi_load("Image/fireball.png", &width, &height, &channels, 0);
    if (data) {
        glGenTextures(1, &fireballTexture);
        glBindTexture(GL_TEXTURE_2D, fireballTexture);
        if (channels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else if (channels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else {
        std::cerr << "Failed to load fireball texture" << std::endl;
    }
}

void drawBall() {
    if (ball.isFireball) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, fireballTexture);
    }
    else if (ball.hasGun) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, bulletTexture);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f); // White color for regular ball
    }

    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= 360; i += 30) {
        float degInRad = i * M_PI / 180;
        float x = cos(degInRad) * ball.radius + ball.x;
        float y = sin(degInRad) * ball.radius + ball.y;
        if (ball.isFireball || ball.hasGun) {
            glTexCoord2f((cos(degInRad) + 1.0f) / 2.0f, (sin(degInRad) + 1.0f) / 2.0f);
        }
        glVertex2f(x, y);
    }
    glEnd();

    if (ball.isFireball || ball.hasGun) {
        glDisable(GL_TEXTURE_2D);
    }
}


void drawPaddle() {
    glBegin(GL_QUADS);
    glVertex2f(paddle.x, paddle.y);
    glVertex2f(paddle.x + paddle.width, paddle.y);
    glVertex2f(paddle.x + paddle.width, paddle.y + paddle.height);
    glVertex2f(paddle.x, paddle.y + paddle.height);
    glEnd();
}

void drawBullets() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bulletTexture);

    for (const auto& bullet : bullets) {
        if (bullet.active) {
            glBegin(GL_TRIANGLE_FAN);
            glTexCoord2f(0.5f, 0.5f); glVertex2f(bullet.x, bullet.y);
            for (int i = 0; i <= 360; i += 30) {
                float degInRad = i * M_PI / 180;
                glTexCoord2f((cos(degInRad) + 1.0f) / 2.0f, (sin(degInRad) + 1.0f) / 2.0f);
                glVertex2f(cos(degInRad) * 5 + bullet.x, sin(degInRad) * 5 + bullet.y);
            }
            glEnd();
        }
    }

    glDisable(GL_TEXTURE_2D);
}

void drawPowerUps() {
    for (const auto& powerUp : powerUps) {
        if (powerUp.active) {
            if (powerUp.type == FIREBALL) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, fireballTexture);
            }
            else if (powerUp.type == GUN) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, bulletTexture);
            }
            else if (powerUp.type == FLIP) {
                glDisable(GL_TEXTURE_2D);
                glColor3f(0.0f, 0.0f, 1.0f); // Blue color for flip power-up
            }
            else if (powerUp.type == SHRINK) {
                glDisable(GL_TEXTURE_2D);
                glColor3f(1.0f, 0.0f, 1.0f); // Purple color for shrink power-up
            }

            glBegin(GL_TRIANGLE_FAN);
            glTexCoord2f(0.5f, 0.5f); glVertex2f(powerUp.x, powerUp.y);
            for (int j = 0; j <= 360; j += 30) {
                float degInRad = j * M_PI / 180;
                glTexCoord2f((cos(degInRad) + 1.0f) / 2.0f, (sin(degInRad) + 1.0f) / 2.0f);
                glVertex2f(cos(degInRad) * powerUp.radius + powerUp.x, sin(degInRad) * powerUp.radius + powerUp.y);
            }
            glEnd();

            if (powerUp.type == FIREBALL || powerUp.type == GUN) {
                glDisable(GL_TEXTURE_2D);
            }
            else {
                glColor3f(1.0f, 1.0f, 1.0f); // Reset color to white after drawing colored power-ups
            }
        }
    }
}

bool checkPowerUpCollision(PowerUp& powerUp) {
    if (powerUp.active) {
        // Check collision with paddle
        if (powerUp.y + powerUp.radius >= paddle.y && powerUp.x >= paddle.x && powerUp.x <= paddle.x + paddle.width) {
            // Apply power-up effect
            if (powerUp.type == FIREBALL) { // Fireball power-up
                ball.isFireball = true;
                ball.hasGun = false; // Deactivate Gun power-up
                ball.gunAmmo = 0;
                ball.powerUpStartTime = time(nullptr);
            }
            else if (powerUp.type == GUN) { // Gun power-up
                ball.hasGun = true;
                ball.isFireball = false; // Deactivate Fireball power-up
                ball.gunAmmo = 3;
            }
            else if (powerUp.type == FLIP) { // Flip power-up
                paddle.controlsFlipped = !paddle.controlsFlipped;
            }
            else if (powerUp.type == SHRINK) { // Shrink power-up
                if (!paddle.isShrunk) {
                    paddle.width *= SHRINK_FACTOR;
                    paddle.isShrunk = true;
                    paddle.shrinkStartTime = time(nullptr);
                }
            }
            powerUp.active = false; // Deactivate power-up
            return true; // Collision detected
        }
    }
    return false; // No collision
}

void updateBall() {
    if (ball.isFireball && difftime(time(nullptr), ball.powerUpStartTime) >= FIREBALL_DURATION) {
        ball.isFireball = false;
        ball.resetPowerUp();
    }

    if (!ball.idle) {
        ball.x += ball.dx;
        ball.y += ball.dy;
        checkCollisions();
    }

    if (ball.y > paddle.y + paddle.height) {
        // Ball goes out of bounds
        lives--;
        if (lives > 0) {
            resetAfterLifeLost = true;
            ball.idle = true;
            ball.x = SCREEN_WIDTH / 2; // Reset ball position
            ball.y = SCREEN_HEIGHT - 50;
            ball.dx = 0.4f; // Reset ball speed
            ball.dy = -0.4f;
            paddle.x = (SCREEN_WIDTH - PADDLE_WIDTH) / 2; // Reset paddle position
            paddle.moveLeft = false; // Reset paddle movement flags
            paddle.moveRight = false; // Reset paddle movement flags
            currentGameState = AIM; // Set game state to AIM to show aiming line
        }
        else {
            currentGameState = GAME_OVER;
        }
    }
}

void updateFireball() {
    // Check if the fireball state has expired
    if (ball.isFireball && difftime(time(nullptr), ball.powerUpStartTime) >= FIREBALL_DURATION) {
        ball.isFireball = false;
        ball.resetPowerUp();
    }
}

void shootGun() {
    if (ball.hasGun && ball.gunAmmo > 0) {
        bullets.emplace_back(paddle.x + paddle.width / 2, paddle.y);
        ball.gunAmmo--;
        if (ball.gunAmmo == 0) {
            ball.hasGun = false; // Deactivate Gun if ammo is depleted
        }
    }
}

void updateBullets() {
    for (auto& bullet : bullets) {
        if (bullet.active) {
            bullet.y += bullet.dy;
            if (bullet.y < 0) {
                bullet.active = false;
            }
        }
    }
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) { return !b.active; }), bullets.end());
}

void updatePaddle() {
    if (paddle.isShrunk && difftime(time(nullptr), paddle.shrinkStartTime) >= SHRINK_DURATION) {
        paddle.width = PADDLE_WIDTH;
        paddle.isShrunk = false;
    }

    if (paddle.controlsFlipped) {
        if (paddle.moveLeft && paddle.x + paddle.width < SCREEN_WIDTH) {
            paddle.x += PADDLE_SPEED;
        }
        if (paddle.moveRight && paddle.x > 0) {
            paddle.x -= PADDLE_SPEED;
        }
    }
    else {
        if (paddle.moveLeft && paddle.x > 0) {
            paddle.x -= PADDLE_SPEED;
        }
        if (paddle.moveRight && paddle.x + paddle.width < SCREEN_WIDTH) {
            paddle.x += PADDLE_SPEED;
        }
    }
}

void initOpenGL() {
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0, 1.0, -1.0);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 1.0);
}

void drawScore() {
    glColor3f(1.0f, 1.0f, 1.0f); // Set text color to white
    glRasterPos2f(10, SCREEN_HEIGHT - 30); // Set the position of the text

    std::string scoreString = "Score: " + std::to_string(score); // Convert the score to string
    for (char ch : scoreString) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the score string
    }
}

void updatePowerUps() {
    for (auto& powerUp : powerUps) {
        if (powerUp.active) {
            powerUp.x += powerUp.dx;
            powerUp.y += powerUp.dy;
            // Check collision with paddle
            if (checkPowerUpCollision(powerUp)) {
                // Power-up collision handled inside checkPowerUpCollision
            }
            // Check if power-up goes out of bounds
            if (powerUp.y > SCREEN_HEIGHT) {
                powerUp.active = false;
            }
        }
    }
}

void drawAmmoCount() {
    if (ball.hasGun) {
        glColor3f(1.0f, 1.0f, 1.0f); // Set text color to white
        glRasterPos2f(SCREEN_WIDTH - 100, SCREEN_HEIGHT - 30); // Set the position of the text

        std::string ammoString = "Ammo: " + std::to_string(ball.gunAmmo); // Convert the ammo count to string
        for (char ch : ammoString) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the ammo string
        }
    }
}

const std::string options[] = {
    "",
    "Start Game",
    "Instructions",
    "Power-Ups"
};

void renderText(const std::string& text, float x, float y, float scale, void* font) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

    for (char c : text) {
        glutBitmapCharacter(font, c);
    }

    glPopMatrix();
}

void drawMenu() {
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the menu title with a larger font and color
    glColor3f(1.0f, 0.0f, 0.0f);  // Set the color to red
    glRasterPos2f(SCREEN_WIDTH / 2 - 75, SCREEN_HEIGHT / 2 - 150);  // Adjust the position as needed
    std::string menuText = "Game Menu";
    for (char ch : menuText) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch);  // Use a larger font
    }

    int offsetY = 40;  // Reduce the space between options
    for (int i = 0; i < 4; ++i) {  // Updated loop to iterate over 4 options
        glRasterPos2f(SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 50 + i * offsetY); // Adjust the vertical position

        //std::cout << "Option: " << options[i] << ", Position: " << (SCREEN_HEIGHT / 2 - 50 + i * offsetY) << ", Selected: " << (i == menuSelection) << std::endl;

        if (i == menuSelection) {
            glColor3f(0.0f, 1.0f, 0.0f); // Highlight selected option in green
        }
        else {
            glColor3f(1.0f, 1.0f, 1.0f); // White color for non-selected options
        }

        for (char c : options[i]) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
    }

    // Flush the rendering pipeline to ensure all commands are processed
    glFlush();
}
int scrollOffset = 0;

void drawInstructions() {
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set color for the text
    glColor3f(1.0f, 1.0f, 1.0f); // White color for text

    // Draw the instructions title
    glRasterPos2f(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 250 + scrollOffset);
    std::string instructionsTitle = "Instructions";
    for (char ch : instructionsTitle) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch);
    }

    // Instructions for the game with symbols
    std::vector<std::string> instructions = {
        "Use arrow keys to move the paddle:",
        "  <- and -> to move left and right.",
        "Press SPACE to start the ball.",
        "Press ENTER to select an option.",
        "Avoid letting the ball fall off the screen.",
        "Destroy all bricks to complete the level.",
        "Collect power-ups for special abilities."
    };

    // Draw the instructions text with symbols
    int offsetY = 80;
    for (const auto& line : instructions) {
        glRasterPos2f(SCREEN_WIDTH / 2 - 250, SCREEN_HEIGHT / 2 - 250 + offsetY + scrollOffset);
        for (char ch : line) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
        }
        offsetY += 30;
    }

    // Details for each level
    std::vector<std::string> levelDetails = {
        "Level 1: 25% 2-hit bricks, 75% 1-hit bricks",
        "Level 2: 30% 2-hit bricks, 10% 3-hit bricks, 60% 1-hit bricks",
        "Level 3: 30% 3-hit bricks, 40% 2-hit bricks, 30% 1-hit bricks"
    };

    // Draw the level details
    offsetY += 20; // Add extra space before level details
    for (const auto& line : levelDetails) {
        glRasterPos2f(SCREEN_WIDTH / 2 - 250, SCREEN_HEIGHT / 2 - 250 + offsetY + scrollOffset);
        for (char ch : line) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
        }
        offsetY += 30;
    }

    // Draw the control keys using symbols
    offsetY += 20; // Add extra space before control keys
    glRasterPos2f(SCREEN_WIDTH / 2 - 250, SCREEN_HEIGHT / 2 - 250 + offsetY + scrollOffset);
    std::string controlKeys = "Control Keys:";
    for (char ch : controlKeys) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
    }

    offsetY += 30;
    glRasterPos2f(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 250 + offsetY + scrollOffset);
    std::string leftArrow = "<- Move Left";
    for (char ch : leftArrow) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
    }

    offsetY += 30;
    glRasterPos2f(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 250 + offsetY + scrollOffset);
    std::string rightArrow = "-> Move Right";
    for (char ch : rightArrow) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
    }

    offsetY += 30;
    glRasterPos2f(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 250 + offsetY + scrollOffset);
    std::string spaceKey = "[SPACE] Start Ball";
    for (char ch : spaceKey) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
    }

    offsetY += 30;
    glRasterPos2f(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 250 + offsetY + scrollOffset);
    std::string enterKey = "[ENTER] Select Option";
    for (char ch : enterKey) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
    }

    // Draw back button
    offsetY += 50; // Add extra space before the back button
    glRasterPos2f(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 250 + offsetY + scrollOffset);
    std::string backButton = "Press ESC to go back";
    for (char ch : backButton) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
    }

    // Flush the rendering pipeline
    glFlush();
}

void drawPowerUpsScreen() {
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set color for the text
    glColor3f(1.0f, 1.0f, 1.0f); // White color for text

    // Draw the power-ups title
    glRasterPos2f(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 200);
    std::string powerUpsTitle = "Power-Ups";
    for (char ch : powerUpsTitle) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch);
    }

    // Power-ups descriptions
    std::vector<std::string> powerUpsDescriptions = {
        "Fireball: Burns through all bricks in its path. (Level 1, 2, 3)",
        "Gun: Allows you to shoot bullets to destroy bricks. (Level 2, 3)",
        "Flip: Temporarily reverses paddle controls. (Level 3)",
        "Shrink: Temporarily shrinks the paddle size. (Level 2, 3)"
    };

    // Power-up symbols colors
    std::vector<std::vector<float>> powerUpColors = {
        {0.0f, 0.0f, 1.0f},   // Blue for Flip
        {1.0f, 0.0f, 1.0f}    // Purple for Shrink
    };

    // Draw the power-ups descriptions
    int offsetY = 50;
    for (int i = 0; i < powerUpsDescriptions.size(); ++i) {
        // Draw the power-up color circle or image
        if (i < 2) { // Use images for Fireball and Gun
            GLuint textureID = (i == 0) ? fireballTexture : bulletTexture;
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(SCREEN_WIDTH / 2 - 230, SCREEN_HEIGHT / 2 - 120 + offsetY - 10);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(SCREEN_WIDTH / 2 - 210, SCREEN_HEIGHT / 2 - 120 + offsetY - 10);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(SCREEN_WIDTH / 2 - 210, SCREEN_HEIGHT / 2 - 120 + offsetY + 10);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(SCREEN_WIDTH / 2 - 230, SCREEN_HEIGHT / 2 - 120 + offsetY + 10);
            glEnd();
            glDisable(GL_TEXTURE_2D);
        }
        else { // Use colors for Flip and Shrink
            glColor3f(powerUpColors[i - 2][0], powerUpColors[i - 2][1], powerUpColors[i - 2][2]);
            glBegin(GL_TRIANGLE_FAN);
            for (int j = 0; j <= 360; j += 30) {
                float degInRad = j * M_PI / 180;
                glVertex2f(cos(degInRad) * 10 + SCREEN_WIDTH / 2 - 220, sin(degInRad) * 10 + SCREEN_HEIGHT / 2 - 120 + offsetY);
            }
            glEnd();
        }

        // Draw the power-up description text
        glColor3f(1.0f, 1.0f, 1.0f); // White color for text
        glRasterPos2f(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 120 + offsetY);
        for (char ch : powerUpsDescriptions[i]) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
        }
        offsetY += 30;
    }

    // Brick descriptions
    std::vector<std::string> brickDescriptions = {
        "1 Hit",
        "2 Hits",
        "3 Hits"
    };

    // Brick textures
    std::vector<GLuint> brickTextures = { brickTexture1, brickTexture2, brickTexture3 };

    offsetY = 50;
    for (int i = 0; i < brickDescriptions.size(); ++i) {
        // Draw the brick texture rectangle
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, brickTextures[i]);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + offsetY - 10);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(SCREEN_WIDTH / 2 - 130, SCREEN_HEIGHT / 2 + offsetY - 10);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(SCREEN_WIDTH / 2 - 130, SCREEN_HEIGHT / 2 + offsetY + 10);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + offsetY + 10);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        // Draw the brick description text
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(SCREEN_WIDTH / 2 - 110, SCREEN_HEIGHT / 2 + offsetY);
        for (char ch : brickDescriptions[i]) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
        }
        offsetY += 30; // Move to the next line
    }

    // Draw back button
    glRasterPos2f(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 150);
    std::string backButton = "Press ESC to go back";
    for (char ch : backButton) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, ch);
    }

    // Flush the rendering pipeline
    glFlush();
}

void drawLevelSelect() {
    glColor3f(1.0f, 1.0f, 1.0f); // White color for text

    // Centering text position for the level selection message
    glRasterPos2f(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50);
    std::string levelSelectText = "Select Level:";
    for (char ch : levelSelectText) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the text
    }

    // List of levels
    std::string levels[] = {
        "1. Level 1",
        "2. Level 2",
        "3. Level 3",
        "Press ESC to go back"
    };

    int offsetY = 30;
    for (const std::string& line : levels) {
        glRasterPos2f(SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + offsetY);
        for (char ch : line) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the text
        }
        offsetY += 30; // Move to the next line
    }
}

void drawGameOver() {
    glColor3f(1.0f, 0.0f, 0.0f); // Red color for text
    glRasterPos2f(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2); // Centering text position
    std::string gameOverText = "GAME OVER! Press ENTER to restart or ESC to exit";
    for (char ch : gameOverText) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the text
    }

    // Display current score
    std::string scoreText = "Score: " + std::to_string(score);
    glRasterPos2f(SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 30); // Adjust position as needed
    for (char ch : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the score text
    }
}


void drawWin() {
    glColor3f(1.0f, 1.0f, 1.0f); // White color for text
    glRasterPos2f(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50); // Centering text position
    std::string winText = "Congratulations! You win!";
    for (char ch : winText) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the text
    }

    // Display current score
    std::string scoreText = "Score: " + std::to_string(score);
    glRasterPos2f(SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 20); // Adjust position as needed
    for (char ch : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the score text
    }

    std::string restartText = "Press ENTER to restart or ESC to exit";
    glRasterPos2f(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 20); // Adjust position as needed
    for (char ch : restartText) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the text
    }
}

void drawLevelComplete() {
    glColor3f(1.0f, 1.0f, 1.0f); // White color for text
    glRasterPos2f(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50); // Centering text position
    std::string levelCompleteText = "Congratulations! Level Complete!";
    for (char ch : levelCompleteText) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the text
    }

    // Display current score
    std::string scoreText = "Score: " + std::to_string(score);
    glRasterPos2f(SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 20); // Adjust position as needed
    for (char ch : scoreText) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the score text
    }

    std::string nextLevelText = "Press ENTER to start the next level or ESC to exit";
    glRasterPos2f(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 20); // Adjust position as needed
    for (char ch : nextLevelText) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the text
    }
}


void drawAimingLine() {
    // Draw the bricks
    for (const auto& brick : bricks) drawBrick(brick);

    // Draw the aiming line
    glColor3f(1.0f, 1.0f, 1.0f); // White color for the aiming line
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0xAAAA); // Dotted line pattern

    glBegin(GL_LINES);
    glVertex2f(ball.x, ball.y);
    glVertex2f(ball.x + 100 * cos(aimAngle), ball.y - 100 * sin(aimAngle)); // Adjust to point the line upwards
    glEnd();

    glDisable(GL_LINE_STIPPLE);

    // Draw the aiming instruction
    glRasterPos2f(SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 50); // Position the text
    std::string aimText = "Select direction and hit SPACE";
    for (char ch : aimText) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the text
    }
}

void resetGame() {
    score = 0;
    lives = 1; // Reset lives to 1
    bricks.clear();
    powerUps.clear();
    bullets.clear();
    setupBricks();
    ball = Ball();
    paddle = Paddle();
    aimAngle = M_PI / 6; // Reset aim angle
    bricksBroken = 0; // Reset the count of bricks broken

    // Set initial movement for the ball
    ball.dx = 0.4f;
    ball.dy = -0.4f;
    ball.idle = true; // Ensure the ball is idle and waiting for the player to start
}



void nextLevel() {
    bricks.clear();
    powerUps.clear();
    bullets.clear();
    setupBricks();
    ball = Ball();
    paddle = Paddle();
    aimAngle = M_PI / 6; // Reset aim angle
    bricksBroken = 0; // Reset the count of bricks broken

    // Set initial movement for the ball
    ball.dx = 0.4f; // You can adjust the speed as necessary
    ball.dy = -0.4f; // Negative to move upwards
    ball.idle = true; // Ensure the ball isn't marked as idle
}

void drawLives() {
    glColor3f(1.0f, 1.0f, 1.0f); // Set text color to white
    glRasterPos2f(SCREEN_WIDTH - 150, 30); // Set the position of the text
    std::string livesString = "Lives: " + std::to_string(lives); // Convert the lives count to string
    for (char ch : livesString) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, ch); // Draw each character of the lives string
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_ENTER:
            if (currentGameState == MENU) {
                if (menuSelection == 0) {
                    currentGameState = LEVEL_SELECT;
                }
                else if (menuSelection == 1) {
                    currentGameState = INSTRUCTIONS;
                }
                else if (menuSelection == 2) {
                    currentGameState = POWER_UPS;
                }
            }
            else if (currentGameState == LEVEL_COMPLETE) {
                level++;
                if (level > 3) {
                    currentGameState = WIN;
                }
                else {
                    nextLevel();
                    currentGameState = AIM;
                }
            }
            else if (currentGameState == GAME_OVER || currentGameState == WIN) {
                currentGameState = MENU;
            }
            break;
        case GLFW_KEY_ESCAPE:
            if (currentGameState == GAME_OVER || currentGameState == WIN || currentGameState == LEVEL_COMPLETE) {
                currentGameState = EXIT;
            }
            else if (currentGameState == INSTRUCTIONS || currentGameState == POWER_UPS || currentGameState == LEVEL_SELECT) {
                currentGameState = MENU;
            }
            break;
        case GLFW_KEY_LEFT:
            if (currentGameState == GAME && !ball.idle) {
                paddle.moveLeft = true;
            }
            else if (currentGameState == AIM) {
                aimAngle += 0.1f;
                if (aimAngle > maxAimAngle) aimAngle = maxAimAngle;
            }
            break;
        case GLFW_KEY_RIGHT:
            if (currentGameState == GAME && !ball.idle) {
                paddle.moveRight = true;
            }
            else if (currentGameState == AIM) {
                aimAngle -= 0.1f;
                if (aimAngle < minAimAngle) aimAngle = minAimAngle;
            }
            break;
        case GLFW_KEY_UP:
            if (currentGameState == MENU) {
                menuSelection = (menuSelection > 0) ? menuSelection - 1 : 3;
            }
            else if (currentGameState == INSTRUCTIONS) {
                scrollOffset += 30;
            }
            break;
        case GLFW_KEY_DOWN:
            if (currentGameState == MENU) {
                menuSelection = (menuSelection < 3) ? menuSelection + 1 : 0;
            }
            else if (currentGameState == INSTRUCTIONS) {
                scrollOffset -= 30;
            }
            break;
        case GLFW_KEY_1:
            if (currentGameState == LEVEL_SELECT) {
                level = 1;
                ball.setSpeed(BALL_SPEED_L1);
                resetGame();
                currentGameState = AIM;
            }
            break;
        case GLFW_KEY_2:
            if (currentGameState == LEVEL_SELECT) {
                level = 2;
                ball.setSpeed(BALL_SPEED_L2);
                resetGame();
                currentGameState = AIM;
            }
            break;
        case GLFW_KEY_3:
            if (currentGameState == LEVEL_SELECT) {
                level = 3;
                ball.setSpeed(BALL_SPEED_L3);
                resetGame();
                currentGameState = AIM;
            }
            break;
        case GLFW_KEY_SPACE:
            if (currentGameState == AIM || (currentGameState == GAME && ball.idle)) {
                ball.dx = ball.dx * cos(aimAngle);
                ball.dy = ball.dy * sin(aimAngle);
                ball.idle = false;
                paddle.moveLeft = false;  // Reset paddle movements
                paddle.moveRight = false; // Reset paddle movements
                currentGameState = GAME;
            }
            else if (currentGameState == GAME) {
                shootGun();
            }
            break;
        }
    }
    else if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_LEFT && currentGameState == GAME) {
            paddle.moveLeft = false;
        }
        else if (key == GLFW_KEY_RIGHT && currentGameState == GAME) {
            paddle.moveRight = false;
        }
    }
}

int main(void) {
    GLFWwindow* window;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Brick Breaker Game", NULL, NULL);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create GLFW window\n";
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    initOpenGL();
    loadBombTexture();
    loadBrickTextures();
    loadFireballTexture(); // Load fireball texture
    loadBulletTexture(); // Load bullet texture
    setupBricks();

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        switch (currentGameState) {
        case MENU:
            drawMenu();
            break;
        case LEVEL_SELECT:
            drawLevelSelect();
            break;
        case AIM:
            drawAimingLine();
            drawBall();
            drawPaddle();
            break;
        case GAME:
            updatePaddle();
            drawBall();
            drawPaddle();
            for (const auto& brick : bricks) drawBrick(brick);
            drawPowerUps();
            drawBullets();
            drawScore();
            drawAmmoCount();
            drawLives(); // Call the function here

            if (!ball.idle) {
                updateBall();
            }

            updatePowerUps();
            updateBullets();
            updateFireball();

            if (std::all_of(bricks.begin(), bricks.end(), [](const Brick& b) { return !b.isVisible; })) {
                currentGameState = LEVEL_COMPLETE;
            }
            break;
        case GAME_OVER:
            drawGameOver();
            break;
        case WIN:
            drawWin();
            break;
        case LEVEL_COMPLETE:
            drawLevelComplete();
            break;
        case EXIT:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case INSTRUCTIONS:
            drawInstructions();
            break;
        case POWER_UPS:
            drawPowerUpsScreen();
            break;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}