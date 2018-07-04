#pragma once
#include "IPCLocalhost.h"

#include <string>
#include <random>
#include <functional>

#include "IPCLocalhostDefs.h"
#include "IPCLocalhostData.h"


class RandomGenerator
{
public:
    explicit RandomGenerator();
    std::string GenerateRandomString(size_t length) const;

private:
    std::default_random_engine m_random_engine;
    std::uniform_int_distribution<> m_dist;
};