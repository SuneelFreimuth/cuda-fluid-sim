#include "app.hpp"

#include <iostream>
#include <cmath>
#include <thread>
#include <array>
#include <cstring>

namespace {
    void divergence(
        Matrix<double>& result,
        Matrix<glm::vec2>& vecField,
        double halfrdx)
    {
        for (uint32_t y = 0; y < result.height; y++)
            for (uint32_t x = 0; x < result.width; x++)
                result.set(x, y, halfrdx * (
                    (vecField.getWrapped(x + 1, y).x - vecField.getWrapped(x - 1, y).x) +
                    (vecField.getWrapped(x, y + 1).y - vecField.getWrapped(x, y - 1).y)
                ));
    }

    template <typename T>
    T min(T a, int b) {
        return a < b ? a : b;
    }

    template <typename T>
    T max(T a, int b) {
        return a > b ? a : b;
    }

    // https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB
    glm::vec3 hslToRgb(const glm::vec3& hsl) {
        double chroma = (1 - abs(2 * hsl.z - 1)) * hsl.y;
        double h = hsl.x / 60.0;
        double x = chroma * (1 - abs(fmod(h, 2.0) - 1.0));
        double r, g, b;
        if (h < 1.0) {
            r = chroma;
            g = x;
            b = 0;
        } else if (h < 2.0) {
            r = x;
            g = chroma;
            b = 0;
        } else if (h < 3.0) {
            r = 0;
            g = chroma;
            b = x;
        } else if (h < 4.0) {
            r = 0;
            g = x;
            b = chroma;
        } else if (h < 5.0) {
            r = x;
            g = 0;
            b = chroma;
        } else if (h < 6.0) {
            r = chroma;
            g = 0;
            b = x;
        }
        double m = hsl.z - chroma / 2.0;
        return glm::vec3(r + m, g + m, b + m);
    }
}

void checkBounds(int x, int y, uint32_t width, uint32_t height) {
    if constexpr (CHECK_BOUNDS) {
        if (!(x >= 0 && x < width && y >= 0 && y < height)) {
            std::cerr << "Position (" <<
                x << ", " << y << ") is outside of width " <<
                width << " and height " << height << std::endl;
            throw std::exception();
        }
    }
}

Simulation::Simulation(uint32_t width, uint32_t height)
    : width(width),
    height(height),
    dye(width, height),
    dyeTemp(width, height),
    pressure(width, height),
    pressureTemp(width, height),
    velocity(width, height),
    velocityTemp(width, height),
    divVelocity(width, height)
{
}


void Simulation::step() {
    advectVelocity();
    diffuse();
    computePressure();
    subtractPressureGradient();
    advectDye();
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            auto d = dye.get(x, y);
            dye.set(x, y, glm::vec3(
                d.x * dyeDecay,
                d.y * dyeDecay,
                d.z * dyeDecay
            ));
        }
    }
}

void Simulation::advectVelocity() {
    auto rdx = 1.0 / dx;

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            const auto v = velocity.get(x, y);

            int backwardX = x - rdx * dt * v.x;
            int backwardY = y - rdx * dt * v.y;
            
            velocityTemp.set(x, y,
                (
                    velocity.getWrapped(backwardX, backwardY) +
                    velocity.getWrapped(backwardX + 1, backwardY) +
                    velocity.getWrapped(backwardX, backwardY + 1) +
                    velocity.getWrapped(backwardX + 1, backwardY + 1)
                ) / 4.0f
            );
        }
    }
    
    velocity.swap(velocityTemp);
}

void Simulation::diffuse() {
    const auto alpha = dx * dx / (viscosity * dt);
    const auto rbeta = 1 / (4 + alpha);
    
    for (uint32_t i = 0; i < stepsToConvergence; i++) {
        if constexpr (USE_MULTIPLE_THREADS) {
            std::array<std::thread, NUM_THREADS> threads;
            for (uint32_t j = 0; j < NUM_THREADS; j++) {
                threads[j] = std::thread([&, j](){
                    for (
                        uint32_t y = j * height / NUM_THREADS;
                        y < (j + 1) * height / NUM_THREADS;
                        y++
                    ) {
                        for (uint32_t x = 0; x < width; x++) {
                            auto v = velocity.get(x, y);
                            auto vTemp = (
                                velocity.getWrapped(x - 1, y) +
                                velocity.getWrapped(x + 1, y) +
                                velocity.getWrapped(x, y - 1) +
                                velocity.getWrapped(x, y + 1) +
                                glm::vec2(v.x * alpha, v.y * alpha)
                            );
                            velocityTemp.set(x, y,
                                glm::vec2(vTemp.x * rbeta, vTemp.y * rbeta));
                        }
                    }
                });
            }
            for (auto& t : threads)
                t.join();
        } else {
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    auto v = velocity.get(x, y);
                    auto vTemp = (
                        velocity.getWrapped(x - 1, y) +
                        velocity.getWrapped(x + 1, y) +
                        velocity.getWrapped(x, y - 1) +
                        velocity.getWrapped(x, y + 1) +
                        glm::vec2(v.x * alpha, v.y * alpha)
                    );
                    velocityTemp.set(x, y,
                        glm::vec2(vTemp.x * rbeta, vTemp.y * rbeta));
                }
            }
        }

        velocity.swap(velocityTemp);
    }
}


void Simulation::computePressure() {
    divergence(divVelocity, velocity, 0.5 / dx);

    std::memset(pressure.items, 0, width * height * sizeof(double));

    const auto alpha = -(dx * dx);
    const auto rbeta = 0.25;

    for (uint32_t i = 0; i < stepsToConvergence; i++) {
        if constexpr (USE_MULTIPLE_THREADS) {
            std::array<std::thread, NUM_THREADS> threads;
            for (uint32_t j = 0; j < NUM_THREADS; j++) {
                threads[j] = std::thread([&, j](){
                    for (
                        uint32_t y = height * j / NUM_THREADS;
                        y < height * (j + 1) / NUM_THREADS;
                        y++)
                    {
                        for (uint32_t x = 0; x < width; x++) {
                            pressureTemp.set(x, y, 
                                (
                                    pressure.getWrapped(x - 1, y) +
                                    pressure.getWrapped(x + 1, y) +
                                    pressure.getWrapped(x, y - 1) +
                                    pressure.getWrapped(x, y + 1) +
                                    divVelocity.get(x, y) * alpha
                                ) * rbeta
                            );
                        }
                    }
                });
            }
            for (auto& t : threads)
                t.join();
        } else {
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    pressureTemp.set(x, y, 
                        (
                            pressure.getWrapped(x - 1, y) +
                            pressure.getWrapped(x + 1, y) +
                            pressure.getWrapped(x, y - 1) +
                            pressure.getWrapped(x, y + 1) +
                            divVelocity.get(x, y) * alpha
                        ) * rbeta
                    );
                }
            }
        }

        pressure.swap(pressureTemp);
    }
}

void Simulation::subtractPressureGradient() {
    const auto halfrdx = 0.5 / dx;

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            auto v = velocity.get(x, y);

            const auto gradX = halfrdx * (
                pressure.getWrapped(x + 1, y) - pressure.getWrapped(x - 1, y)
            );
            const auto gradY = halfrdx * (
                pressure.getWrapped(x, y + 1) - pressure.getWrapped(x, y - 1)
            );

            v.x -= gradX;
            v.y -= gradY;
            velocity.set(x, y, v);
        }
    }
}

void Simulation::advectDye() {
    auto rdx = 1.0 / dx;

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            const auto v = velocity.get(x, y);

            int backwardX = x - rdx * dt * v.x;
            int backwardY = y - rdx * dt * v.y;
            
            dyeTemp.set(x, y,
                (
                    dye.getWrapped(backwardX, backwardY) +
                    dye.getWrapped(backwardX + 1, backwardY) +
                    dye.getWrapped(backwardX, backwardY + 1) +
                    dye.getWrapped(backwardX + 1, backwardY + 1)
                ) / 4.0f
            );
        }
    }
    
    dye.swap(dyeTemp);
}

constexpr int BRUSH_SIZE = 20;

void App::setup() {
    prevTime = millis();
    prevX = 0;
    prevY = 0;
    frameCount = 0;
}

void App::draw(piksel::Graphics& g) {
    // Draw simulation
    g.noStroke();
    g.background(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    for (uint32_t y = 0; y < sim.height; y++) {
        for (uint32_t x = 0; x < sim.width; x++) {
            if (sim.dye.get(x, y) != glm::vec3(0.0f, 0.0f, 0.0f)) {
                g.fill(glm::vec4(sim.dye.get(x, y), 1.0f));
                g.rect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
            }
        }
    }

    // Draw framerate
    auto now = millis();
    auto dt = now - prevTime;
    prevTime = now;
    g.fill(glm::vec4(1.0f));
    g.stroke(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    g.strokeWeight(1);
    g.textSize(20);
    g.text(std::to_string((uint32_t) (1000.0 / dt)), 5, 25);

    sim.step();
    frameCount += 1;
}


void App::mouseMoved(int x, int y) {
    x *= (float) SIM_WIDTH / WIDTH;
    y *= (float) SIM_HEIGHT / HEIGHT;
    auto mouseDisp = glm::vec2(x - prevX, y - prevY);
    auto velocity = mouseDisp;
    velocity /= 6;
    for (
        int y_ = max(y - BRUSH_SIZE / 2, 0);
        y_ <= min(y + BRUSH_SIZE / 2, sim.height - 1);
        y_++)
    {
        for (
            int x_ = max(x - BRUSH_SIZE / 2, 0);
            x_ <= min(x + BRUSH_SIZE / 2, sim.width - 1);
            x_++)
        {
            if (sqrt(pow(x - x_, 2) + pow(y - y_, 2)) <= BRUSH_SIZE / 2) {
                sim.velocity.set(x_, y_, sim.velocity.get(x_, y_) + mouseDisp * 5.0f);
                sim.dye.set(x_, y_, hslToRgb(glm::vec3(3 * frameCount % 360, 1.0f, 0.5f)));
            }
        }
    }
    prevX = x;
    prevY = y;
}

