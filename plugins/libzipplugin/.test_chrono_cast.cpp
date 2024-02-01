/*
SPDX-FileCopyrightText: 2024 Hannah von Reth <vonreth@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include <chrono>

int main()
{
    std::chrono::clock_cast<std::chrono::system_clock>(std::chrono::system_clock::now());
    return 0;
}
