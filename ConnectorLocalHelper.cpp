#include "ConnectorLocalHelper.h"
#include <vector>
#include <algorithm>

using namespace boost::interprocess;

static std::vector<char> g_random_chars = { '0','1','2','3','4','5','6','7','8','9',
'A','B','C','D','E','F','G','H','I','J',
'K','L','M','N','O','P','Q','R','S','T',
'U','V','W','X','Y','Z','a','b','c','d',
'e','f','g','h','i','j','k','l','m','n',
'o','p','q','r','s','t','u','v','w','x',
'y','z' };


RandomGenerator::RandomGenerator()
	: m_random_engine(std::random_device{}()),
	m_dist(0, static_cast<int>(g_random_chars.size()) - 1)
{
}


std::string RandomGenerator::GenerateRandomString(size_t length) const
{
	std::string str(length, 0);
	std::generate_n(str.begin(), length, [this]() { return g_random_chars[m_dist(m_random_engine)]; });
	return str;
}