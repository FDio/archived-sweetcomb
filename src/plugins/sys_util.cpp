#include "sys_util.h"

namespace utils {

prefix::prefix()
{
}

prefix::prefix(const prefix &p)
{
    m_address = p.address();
    m_prefix_len = p.prefix_length();
}

prefix::prefix(std::string p)
{
    size_t found = p.find_last_of('/');

    if (found == std::string::npos) //not found
        throw std::runtime_error("missing '/' in " + p);

    m_address = boost::asio::ip::address::from_string(p.substr(0, found));
    m_prefix_len =  std::stoi(p.substr(found+1, p.length()));
}

prefix prefix::make_prefix(std::string str)
{
    prefix tmp(str);
    return prefix(tmp);
}

unsigned short prefix::prefix_length() const
{
    return m_prefix_len;
}

boost::asio::ip::address prefix::address() const
{
    return m_address;
}

std::string prefix::to_string() const
{
    ostringstream os;
    os << m_address << "/" << m_prefix_len;
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const prefix& p)
{
    os << p.to_string();

    return os;
}

bool prefix::empty() const
{
    return to_string().empty();
}

}
