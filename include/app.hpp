#ifndef APP_HPP
#define APP_HPP

#include <array>
#include <chrono>
#include <iostream>
#include <cassert>

#include <string>
using namespace std::string_literals;

#include <piksel/baseapp.hpp>


constexpr bool CHECK_BOUNDS = true;
constexpr bool USE_MULTIPLE_THREADS = true;
constexpr int NUM_THREADS = 4;


int mod(int m, int n) {
    return m >= 0
        ? m % n
        : m % n + n;
}


template <typename T>
struct Matrix {
    T* items;
    uint32_t width, height;

    Matrix(uint32_t width, uint32_t height)
        : width{width}
        , height{height}
    {
        items = new T[width * height];
        std::memset(items, 0, sizeof(T) * width * height);
    }

    ~Matrix() {
        delete[] items;
    }

    T& get(int x, int y) {
        checkBounds(x, y, width, height);
        return items[y * width + x];
    }

    T& getWrapped(int x, int y) {
        return items[mod(y, height) * width + mod(x, width)];
    }

    void set(int x, int y, const T& val) {
        checkBounds(x, y, width, height);
        items[y * width + x] = val;
    }

    template <typename Floating>
    void operator*=(Floating n) {
        items
    }

    void swap(Matrix& m) {
        T* temp = m.items;
        m.items = items;
        items = temp;
    }
};

constexpr uint32_t WIDTH = 600;
constexpr uint32_t HEIGHT = WIDTH;
constexpr uint32_t SIM_WIDTH = 200;
constexpr uint32_t SIM_HEIGHT = SIM_WIDTH;
static_assert(WIDTH % SIM_WIDTH == 0);
static_assert(HEIGHT % SIM_HEIGHT == 0);
constexpr uint32_t CELL_SIZE = WIDTH / SIM_WIDTH;

class Simulation {
public:
    Simulation(uint32_t width, uint32_t height);
    void step();

    uint32_t width;
    uint32_t height;
    Matrix<glm::vec3> dye;
    Matrix<double> pressure;
    Matrix<glm::vec2> velocity;
    Matrix<double> divVelocity;

private:
    void advectDye();
    void advectVelocity();
    void diffuse();
    void computePressure();
    void subtractPressureGradient();

    double dx = 0.005;
    double dt = 0.01;
    double viscosity = 0.001;
    uint32_t stepsToConvergence = 40;
    double dyeDecay = 0.95;

    Matrix<glm::vec3> dyeTemp;
    Matrix<double> pressureTemp;
    Matrix<glm::vec2> velocityTemp;
};

class App : public piksel::BaseApp {
public:
    App()
        : piksel::BaseApp(WIDTH, HEIGHT),
        sim(SIM_WIDTH, SIM_HEIGHT)
    {}
    void setup();
    void draw(piksel::Graphics& g);
    void mouseMoved(int x, int y);

private:

    Simulation sim;
    int prevTime, prevX, prevY;
    uint32_t frameCount;
};

#endif
