/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#ifndef MINGW32
#include <syslog.h>
#endif

#include "liblog.h"
#include "loghandlers.h"
#include "argsmgr.h"

namespace szlog {

std::shared_ptr<Logger> logger;

}


int sz_loginit_cmdline(int level, const char * logfile, int *argc, char *argv[], SZ_LIBLOG_FACILITY)
{
	if (!szlog::logger)
		szlog::logger = std::make_shared<szlog::Logger>();

	szlog::logger->set_log_treshold(level);
	std::string pname = "";
	if (logfile) {
		pname = std::string(logfile);
		if (!pname.empty()) {
			szlog::logger->set_logger<szlog::FileLogger>(std::string(logfile));
		} else {
			szlog::logger->set_logger<szlog::JournaldLogger>();
		}
	} else {
		szlog::logger->set_logger<szlog::JournaldLogger>();
	}

	return level;
}

namespace szlog {

void init(const ArgsManager& args_mgr) {
	if (!szlog::logger)
		szlog::logger = std::make_shared<Logger>();

	auto log_type = args_mgr.get<std::string>("logger");
	if (log_type) {
		if (*log_type == "cout" || args_mgr.has("no-daemon") || args_mgr.has("no_daemon")) {
			logger->set_logger<COutLogger>();
		} else if (*log_type == "journald") {
			logger->set_logger<JournaldLogger>();
		} else if (*log_type == "file") {
			auto logfile_opt = args_mgr.get<std::string>("log_file");
			if (logfile_opt) {
				logger->set_logger<FileLogger>(*logfile_opt);
			} else {
				logger->set_logger<COutLogger>();
			}
		} else {
			logger->set_logger<COutLogger>();
		}
	}

	auto log_level = args_mgr.get<int>("log_level").get_value_or(2);

	// if debug is specified, use it
	log_level = args_mgr.get<unsigned int>("debug").get_value_or(log_level);

	logger->set_log_treshold(log_level);
}

}

int sz_loginit(int level, const char * logname, SZ_LIBLOG_FACILITY, void *) {
	if (!szlog::logger)
		szlog::logger = std::make_shared<szlog::Logger>();

	if (logname != NULL) {
		auto pname = std::string(logname);
		if (!pname.empty()) {
			szlog::logger->set_logger<szlog::FileLogger>(pname);
		} else {
			szlog::logger->set_logger<szlog::JournaldLogger>();
		}
	} else {
		szlog::logger->set_logger<szlog::JournaldLogger>();
	}

	szlog::logger->set_log_treshold(level);
}

void sz_logdone(void) {
}

void sz_log(int level, const char * msg_format, ...) {
	va_list fmt_args;
	va_start(fmt_args, msg_format);

	char str[256];
	vsnprintf(str, 255, msg_format, fmt_args);

	szlog::log() << szlog::PriorityForLevel(level) << std::string(str) << szlog::flush;

	va_end(fmt_args);
}


void vsz_log(int level, const char * msg_format, va_list fmt_args) {
	sz_log(level, msg_format, fmt_args);
}

namespace szlog {

template <>
Logger& Logger::operator<<(const szlog::priority& p) {
	std::lock_guard<std::mutex> lock(_msg_mutex);
	const auto t_id = std::this_thread::get_id();

	if (!_msgs[t_id]) _msgs[t_id] = std::make_shared<LogEntry>();
	_msgs[t_id]->_p = p;
	return *this;
}

template <>
Logger& Logger::operator<<(const szlog::str_mod& m) {
	std::lock_guard<std::mutex> lock(_msg_mutex);
	const auto t_id = std::this_thread::get_id();

	if (!_msgs[t_id]) _msgs[t_id] = std::make_shared<LogEntry>();

	switch(m) {
	case szlog::str_mod::endl:
		log_later(_msgs[t_id]);
		_msgs.erase(t_id);
		break;
	case szlog::str_mod::flush:
		log_now(_msgs[t_id]);
		_msgs.erase(t_id);
		break;
	case szlog::str_mod::block:
		break;
	}

	return *this;
}


priority PriorityForLevel(int level) {
	if (level <= 2) {
		return szlog::CRITICAL;
	} else if (level <= 4) {
		return szlog::ERROR;
	} else if (level <= 7) {
		return szlog::WARNING;
	} else if (level <= 9) {
		return szlog::INFO;
	} else {
		return szlog::DEBUG;
	}
}


// To substitute with std::put_time in (g++ >= 5)
std::string format_date(tm* localtime_t) {
	std::stringstream oss;
	std::vector<std::pair<int, char>> date_fields_with_postfixes = {{localtime_t->tm_year + 1900, '-'}, {localtime_t->tm_mon, '-'}, {localtime_t->tm_mday, ' '}, {localtime_t->tm_hour, ':'}, {localtime_t->tm_min, ':'}, {localtime_t->tm_sec, ' '}};
	for (auto c: date_fields_with_postfixes) {

		// If less than 10 we need to add the "0" in front (00:02 instead of 0:2)
		if (c.first < 10) {
			oss << "0";
		}

		oss << c.first << c.second;
	}

	return oss.str();
}

const std::string msg_priority_for_level(szlog::priority p) {
	switch (p) {
	case szlog::DEBUG:
		return "DEBUG";
	case szlog::INFO:
		return "INFO";
	case szlog::WARNING:
		return "WARN";
	case szlog::ERROR:
		return "ERROR";
	case szlog::CRITICAL:
		return "CRITICAL";
	}

	return "";
}

Logger& log() {
	if (!logger)
		logger = std::make_shared<Logger>();
	
	return *logger;
}


} // namespace szlog

void sz_log_info(int info) {}
