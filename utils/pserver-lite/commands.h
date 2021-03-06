#ifndef __PSERVER_COMMANDS_H
#define __PSERVER_COMMANDS_H

#include "command_handler.h"

/**
 * "GET" Command
 *
 * Syntax:
 *  GET start_time end_time param_path\r\n
 *
 *   'start_time' and 'end_time' (time_t) - inclusive time range of probes.
 *   'param_path' (string) - a path to parameter (relative to cache dir).
 *
 * Response:
 *  last_time size\n
 *  data
 *
 *   'last_time' (time_t) - time of the last available probe for parameter.
 *   'size' (int) - size (in bytes) of data section.
 *   'data' (bin) - sequential raw values of a fetched parameter as
 *                  little-endian two byte numbers.
 */
class GetCommand : public CommandHandler
{
public:
	static const std::string get_tag (void) { return "GET"; };

	GetCommand (void);
	~GetCommand (void) { };

	void load_args (const std::vector<std::string>& args) override;
	std::vector<unsigned char> exec (void) override;

private:
	bool m_ready;
	time_t m_start_time;
	time_t m_end_time;
	std::string m_param_path;
};

/**
 * "SEARCH" Command
 *
 * Syntax:
 *  SEARCH start_time end_time direction param_path\r\n
 *
 *   'start_time' and 'end_time' (time_t) - inclusive time range for search,
 *      -1 means searching from the beginning or to the ending of the
 *      available range.
 *   'direction' (int) - search direction, -1 for left (start_time -> end_time),
 *                       0 for in-place, 1 for right (end_time -> start_time).
 *   'param_path' (string) - a path to parameter (relative to cache dir).
 *
 * Response:
 *   found_time first_time last_time\n
 *
 *  'found_time' (time_t) - a time of data found or -1 on failure.
 *  'first_time' and 'last_time' - range of available data or -1 in no data is
 *     available.
 */
class SearchCommand : public CommandHandler
{
public:
	static const std::string get_tag (void) { return "SEARCH"; };

	SearchCommand (void);
	~SearchCommand (void) { };

	void load_args (const std::vector<std::string>& args) override;
	std::vector<unsigned char> exec (void) override;

private:
	bool m_ready;
	time_t m_start_time;
	time_t m_end_time;
	int m_direction;
	std::string m_param_path;
};

/**
 * "RANGE" Command
 *
 * Syntax:
 *  RANGE\r\n
 *
 * Response:
 *  first_time last_time\n
 *
 *  'first_time' and 'last_time' (time_t) - approximate time range of available
 *     probes (for all parameters).
 */
class RangeCommand : public CommandHandler
{
public:
	static const std::string get_tag (void) { return "RANGE"; };

	RangeCommand (void) { };
	~RangeCommand (void) { };

	void load_args (const std::vector<std::string>& args) override;
	std::vector<unsigned char> exec (void) override;
};

#endif /* __PSERVER_COMMANDS_H */
