#ifndef __SZ4_LUA_PARAM_ENTRY_H__
#define __SZ4_LUA_PARAM_ENTRY_H__
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

#include <tr1/tuple>
#include <set>

#if 0
#include "szarp_base_common/lua_types.h"
#endif
#include "sz4/definable_param_cache.h"
#include "sz4/buffered_param_entry.h"
#include "sz4/fixed_stack_top.h"
#include "sz4/lua_first_last_time.h"
#include "sz4/lua_average_calc_algo.h"

namespace sz4 {

template<class types> class lua_caluclate : public lua_average_calc_algo {
	typename types::base* m_base;
	TParam* m_param;
	bool m_param_ok;
public:
	lua_caluclate(typename types::base* base, TParam* param) : m_base(base), m_param(param), m_param_ok(false) {
		m_param_ok = m_base->get_lua_interpreter().prepare_param(param);
	}

	void initialize() {}

	void do_calculate_value(second_time_t time, SZARP_PROBE_TYPE probe_type, double &result, bool& fixed) {
		if (!m_param_ok) {
			result = nan("");
			fixed = false;
			return;
		}

		fixed_stack_top stack_top(m_base->fixed_stack());
		result = m_base->get_lua_interpreter().calculate_value(sz4::nanosecond_time_t(time), probe_type, 0);
		fixed &= stack_top.top();
	}

	~lua_caluclate() {
		if (m_param_ok) {
			m_base->get_lua_interpreter().pop_prepared_param();
		}
	}
};

template<class value_type, class time_type, class types> class lua_param_entry_in_buffer : public buffered_param_entry_in_buffer<value_type, time_type, types, lua_caluclate> {
public:
	lua_param_entry_in_buffer(typename types::base* _base, TParam* param, const boost::filesystem::wpath& path) : buffered_param_entry_in_buffer<value_type, time_type, types, lua_caluclate>(_base, param, path) {}

	void get_first_time(std::list<generic_param_entry*>& referred_params, time_type &t) {
		this->m_base->get_heartbeat_first_time(this->m_param, t);
		lua_adjust_first_time(this->m_param, t);
	}

	void get_last_time(const std::list<generic_param_entry*>& referred_params, time_type &t) {
		this->m_base->get_heartbeat_last_time(this->m_param, t);
		lua_adjust_last_time(this->m_param, t);
	}
};

}

#endif
