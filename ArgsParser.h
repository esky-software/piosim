#pragma once

#include <algorithm>
#include <list>
#include <string>

////////////////

class ArgsParser
{
public:
	ArgsParser(int argc, char* argv[])
		: m_args(argv+1, argv+argc)
	{}

	bool Done() const
	{
		return m_args.empty();
	}

	std::string NextArg() const
	{
		if (m_args.empty())
			return {};
		return m_args.front();
	}

	// Parse helpers return false on error.  Absence of the specified args is *not* an error.
	bool ParseString(std::string& o_val, const std::list<std::string>& i_names, const std::list<std::string>& i_valid = {})
	{
		if (!Match(i_names))
			return false;
		if (m_args.empty())
			return false;
		if (!i_valid.empty() && (std::find(i_valid.begin(), i_valid.end(), m_args.front()) == i_valid.end()))
			return false;
		o_val = m_args.front();
		m_args.pop_front();
		return true;
	}

	bool ParseUint(unsigned& o_val, const std::list<std::string>& i_names)
	{
		if (!Match(i_names))
			return false;
		if (m_args.empty())
			return false;;
		o_val = std::stoul(m_args.front());
		m_args.pop_front();
		return true;
	}
	
	bool ParseBool(bool& o_val, const std::list<std::string>& i_names)
	{
		if (!Match(i_names))
			return false;
		if (m_args.empty())
			return false;;
		o_val = (std::stoul(m_args.front()) != 0);
		m_args.pop_front();
		return true;
	}

	bool ParseFlag(bool& o_val, const std::list<std::string>& i_names)
	{
		if (!Match(i_names))
			return false;
		o_val = true;
		return true;
	}

private:
	bool Match(const std::list<std::string>& i_names)
	{
		if (std::find(i_names.begin(), i_names.end(), m_args.front()) == i_names.end())
			return false;
		m_args.pop_front();
		return true;
	}

private:
	std::list<std::string> m_args;
};
