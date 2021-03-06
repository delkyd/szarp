
#include <iostream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "commands.h"
#include "szcache.h"

/*** Static methods (Command Handler) ***/

std::vector<std::string> CommandHandler::tokenize (std::string& msg_received)
{
	/* check terminate string CRLF */
	auto len = msg_received.length();
	if (len < 2 || msg_received[len-2] != '\r' || msg_received[len-1] != '\n') {
		throw ParseError("no proper EOL sign, line not ended with CRLF");
	}
	msg_received.resize(len-2);

	/* tokenize received line */
	std::vector<std::string> tokens;
	boost::split(tokens, msg_received, boost::is_any_of("\t "),
			boost::algorithm::token_compress_on);

	if (0 == tokens.size()) {
		throw ParseError("too few tokens in line");
		// probably won't happen as "" gives one empty token
	}

	return tokens;
}


/*** Command implementations (CommandHandler derivatives) ***/

/** "GET" Command **/
GetCommand::GetCommand (void)
	: m_ready(false), m_start_time(0), m_end_time(0), m_param_path() { }

void GetCommand::load_args (const std::vector<std::string>& args)
{
	m_ready = false;

	if (args.size() != 3)
		throw ArgumentError("(GET) bad number of arguments");

	/* load arguments 1 and 2 (start_time, end_time) */
	try {
		m_start_time = boost::lexical_cast<time_t>(args[0]);
		m_end_time = boost::lexical_cast<time_t>(args[1]);
	}
	catch (const boost::bad_lexical_cast &) {
		throw ArgumentError("(GET) cannot cast argument 1 or 2 to time_t");
	}

	if (m_start_time > m_end_time)
		throw ArgumentError("(GET) bad argument 1 and 2, start > end");

	/* load argument 3 (param_path) */
	if (args[2].length() == 0)
		throw ArgumentError("(GET) bad argument 3, parameter path is zero length");

	m_param_path = args[2];

	/* arguments successfully loaded */
	m_ready = true;
}

std::vector<unsigned char> GetCommand::exec (void)
{
	if (! m_ready)
		throw ArgumentError("cannot exec(), arguments not loaded");

	/* get size of data and last available probe */
	auto ret = _szcache.getSizeAndLast(m_start_time, m_end_time, m_param_path);
	auto size = ret.first;
	auto last_time = ret.second;

	/* construct reply header */
	std::string reply;
	reply += std::to_string(last_time);
	reply += std::string(" ");
	reply += std::to_string(size);
	reply += std::string("\n");

	/* convert to vector (portability) */
	std::vector<unsigned char> reply_vec(reply.begin(), reply.end());

	if (size > 0) {
		/* get values of probes */
		std::vector<int16_t> vals;

		while (m_start_time < m_end_time) {
			m_start_time =
				_szcache.writeData(vals, m_start_time, m_end_time, m_param_path);
		}

		/* split into lsw/msw and insert to vector */
		for (auto s : vals) {
			unsigned char lsw = s & 0xFF;
			unsigned char msw = (s >> 8) & 0xFF;
			reply_vec.push_back(lsw);
			reply_vec.push_back(msw);
		}
	}

	return reply_vec;
}

/** "SEARCH" Command **/
SearchCommand::SearchCommand (void)
	: m_ready(false), m_start_time(0), m_end_time(0), m_direction(0),
		m_param_path() { }

void SearchCommand::load_args (const std::vector<std::string>& args)
{
	m_ready = false;

	if (args.size() != 4)
		throw ArgumentError("(SEARCH) bad number of arguments");

	/* load arguments 1 and 2 (start_time, end_time) */
	try {
		m_start_time = boost::lexical_cast<time_t>(args[0]);
		m_end_time = boost::lexical_cast<time_t>(args[1]);
	}
	catch (const boost::bad_lexical_cast &) {
		throw ArgumentError("(SEARCH) cannot cast argument 1 or 2 to time_t");
	}

	/* load argument 3 (direction) */
	try {
		m_direction = boost::lexical_cast<int>(args[2]);
	}
	catch (const boost::bad_lexical_cast &) {
		throw ArgumentError("(SEARCH) cannot cast argument 1 or 2 to time_t");
	}

	if (m_direction < -1 || m_direction > 1)
		throw ArgumentError("(SEARCH) bad argument 3, direction not in {-1, 0, 1}.");

	/* load argument 4 (param_path) */
	if (args[3].length() == 0)
		throw ArgumentError("(SEARCH) bad argument 4, parameter path is zero length");

	m_param_path = args[3];

	/* arguments successfully loaded */
	m_ready = true;
}

std::vector<unsigned char> SearchCommand::exec (void)
{
	if (! m_ready)
		throw ArgumentError("cannot exec(), arguments not loaded");

	/* search probes */
	SzCache::SzSearchResult res;

	/* timestamps are more important than direction (fix its value if needed) */
	if (m_direction != 0) {
		if (m_start_time > m_end_time)
			m_direction = -1;
		else
			m_direction = 1;
	}

	switch (m_direction) {
		case -1:
			res = _szcache.searchLeft(m_start_time, m_end_time, m_param_path);
			break;
		case  0:
			res = _szcache.searchInPlace(m_start_time, m_param_path);
			break;
		case  1:
			res = _szcache.searchRight(m_start_time, m_end_time, m_param_path);
			break;
		default:
			throw ArgumentError("(SEARCH) exec(): bad direction value");
	}

	/* construct reply message */
	std::string reply;
	reply += std::to_string(std::get<0>(res));
	reply += std::string(" ");
	reply += std::to_string(std::get<1>(res));
	reply += std::string(" ");
	reply += std::to_string(std::get<2>(res));
	reply += std::string("\n");

	/* convert to vector (portability) */
	std::vector<unsigned char> reply_vec(reply.begin(), reply.end());

	return reply_vec;
}

/** "RANGE" Command **/
void RangeCommand::load_args (const std::vector<std::string>& args)
{
	if (args.size() != 0)
		throw ArgumentError("(RANGE) bad number of arguments");
}

std::vector<unsigned char> RangeCommand::exec (void)
{
	/* get available range */
	auto range = _szcache.availableRange();

	/* construct reply message */
	std::string reply;
	reply += std::to_string(range.first);
	reply += std::string(" ");
	reply += std::to_string(range.second);
	reply += std::string("\n");

	/* convert to vector (portability) */
	std::vector<unsigned char> reply_vec(reply.begin(), reply.end());

	return reply_vec;
}
