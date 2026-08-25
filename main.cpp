#include <iostream>
#include <iomanip>
#include <print>
#include <random>
#include <algorithm>
#include <ranges>
#include <thread>
#include <stop_token>
#include <chrono>

enum class CELL{OPEN, CLOSED};
enum class MOVE{UP, DOWN, RIGHT};

int main()
{
    constexpr int rows = 10,
                  columns = 10,
                  states = rows * columns;

    CELL grid[rows][columns];

    for (int row = 0; row < rows; ++row)
        for (int column = 0; column < columns; ++column)
            grid[row][column] = CELL::CLOSED;

    std::random_device rd;
    std::uniform_int_distribution<int> rowDist(0, rows - 1),
                                       moveDist(0, 2);
    MOVE moves[3] = {MOVE::UP, MOVE::DOWN, MOVE::RIGHT};
    const int startingRow = rowDist(rd);

    // create maze:
    {
        int column = 0,
            row = startingRow;

        while (true)
        {
            grid[row][column] = CELL::OPEN;

            if (column == columns - 1)
                break;

            switch (moves[moveDist(rd)])
            {
                case MOVE::UP:
                {
                    if (row > 0)
                        --row;

                    break;
                }
                case MOVE::DOWN:
                {
                    if (row + 1 < rows)
                        ++row;

                    break;
                }
                case MOVE::RIGHT:
                {
                    ++column;
                    break;
                }
            }
        }
    }

    double qTable[states][3];
    for (int state = 0; state < states; ++state)
        for (int action = 0; action < 3; ++action)
            qTable[state][action] = 0;

    constexpr double learningRate = 0.1,
                     discountFactor = 0.9;
    double explorationRate = 0.25;

    std::print("\033[?25l"); // cursor off
    std::print("\033[2J"); // clear screen

    {
        std::jthread thr{[&](std::stop_token st)
        {
            std::uniform_real_distribution<double> dist(0, 1);
            int row = startingRow,
                column = 0;
            int epoch = 0;

            while (!st.stop_requested())
            {
                std::print("\033[1;1;H");
                for (int i = 0; i < rows; ++i)
                {
                    for (int i2 = 0; i2 < columns; ++i2)
                        switch (grid[i][i2])
                        {
                            case CELL::OPEN:
                                std::print("\033[38;2;247;137;33m.");
                                break;
                            case CELL::CLOSED:
                                std::print("\033[38;2;52;48;145m█");
                                break;
                        }

                    std::println();
                }
                std::print("\033[38;2;255;255;255m");
                std::print("Epoch: {}", epoch);

                std::print("\033[{};{}H", row + 1, column + 1);
                if (column == columns - 1)
                    std::print("\033[38;2;80;186;112m●");
                else
                    std::print("\033[38;2;254;192;20m●");
                std::cout << std::flush;

                if (column == columns - 1)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
                    row = startingRow;
                    column = 0;
                    explorationRate *= 0.99;
                    ++epoch;
                    continue;
                }

                auto& stateRow = qTable[row * columns + column];

                const int moveIndex = (dist(rd) < explorationRate)
                                    ? moveDist(rd)
                                    : static_cast<int>(std::ranges::max_element(stateRow)
                                                        - std::ranges::begin(stateRow));

                int newRow = row,
                    newColumn = column;
                double reward = -0.1;

                switch (moves[moveIndex])
                {
                    case MOVE::UP:
                    {
                        if ((row > 0) && (grid[row - 1][column] == CELL::OPEN))
                            newRow = row - 1;
                        else
                            reward = -1;

                        break;
                    }
                    case MOVE::DOWN:
                    {
                        if ((row < rows - 1) && (grid[row + 1][column] == CELL::OPEN))
                            newRow = row + 1;
                        else
                            reward = -1;

                        break;
                    }
                    case MOVE::RIGHT:
                        if (grid[row][column + 1] == CELL::OPEN)
                            newColumn = column + 1;
                        else
                            reward = -1;

                        break;
                }

                if (newColumn == columns - 1)
                    reward = 100;

                auto& newStateRow = qTable[newRow * columns + newColumn];

                stateRow[moveIndex] += learningRate * (reward + discountFactor * std::ranges::max(newStateRow)
                                                        - stateRow[moveIndex]);

                row = newRow;
                column = newColumn;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }};
        std::cin.get();
    }

    std::print("\033[?25h"); // restore cursor
}