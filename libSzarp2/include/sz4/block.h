/* 
  SZARP: SCADA software 
  

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
#ifndef __SZ4_BLOCK_H__
#define __SZ4_BLOCK_H__

#include <algorithm>
#include <iterator>
#include <list>

#include "sz4/defs.h"
#include "sz4/time.h"
#include "sz4/block_cache.h"

namespace sz4 {

template<class pair_type> pair_type make_value_time_pair(const typename pair_type::value_type& v, const typename pair_type::time_type& t) {
	pair_type pair;
	pair.value = v;
	pair.time = t;
	return pair;
}

template<class time_type, class pair_type> bool cmp_time(const time_type& t, const pair_type& p) {
	return t < p.time;
}

template<class I, class T> I search_entry_for_time(I begin, I end, const T& t) {
	typedef typename I::value_type pair_type;

	return std::upper_bound(begin, end, t, cmp_time<T, pair_type>);
}

class block_cache;
class generic_block {
protected:
	block_cache* m_cache;
	std::vector<generic_block*> m_refferring_blocks;
	std::vector<generic_block*> m_refferred_blocks;

	std::list<generic_block*>::iterator m_block_location;
public:
	generic_block(block_cache* cache);
	bool ok_to_delete() const;
	std::list<generic_block*>::iterator& location();
	bool has_refferring_blocks() const;
	void add_refferring_block(generic_block* block);
	void remove_refferring_block(generic_block* block);

	void add_refferred_block(generic_block* block);
	void remove_refferred_block(generic_block* block);

	virtual size_t block_size() const = 0;

	void remove_from_cache();
	virtual ~generic_block();
};

template<class T> struct empty_merge {
	void operator()(T& t, T& t2, T& t3) const {}
};

template<
	class value_time_type,
	class value_cmp = std::equal_to<typename value_time_type::value_type>,
	class value_merge = empty_merge<typename value_time_type::value_type>
> class value_time_block : public generic_block {
public:
	typedef typename value_time_type::value_type value_type;
	typedef typename value_time_type::time_type time_type;

	typedef std::vector<value_time_type> value_time_vector;

	value_time_block(const time_type& time,
		block_cache* cache)
		:
		generic_block(cache),
		m_start_time(time)
	{}

	const time_type& start_time() const { return m_start_time; }
	const time_type end_time() const { return m_data[m_data.size() - 1].time; }

	typename value_time_vector::iterator search_entry_for_time(const time_type& time) {
		return sz4::search_entry_for_time(m_data.begin(), m_data.end(), time);
	}

	typename value_time_vector::const_iterator search_entry_for_time(const time_type& time) const {
		return sz4::search_entry_for_time(m_data.begin(), m_data.end(), time);
	}

	value_time_vector& data() {
		return m_data;
	}

	const value_time_vector& data() const {
		return m_data;
	}

	size_t block_size() const {
		return m_data.size() * (sizeof(value_type) + sizeof(time_type));
	}

	void block_data_updated(size_t previous_size) {
		m_cache->block_size_changed(this, previous_size);
	}

	template<class search_op> typename value_time_vector::const_iterator search_data_right_t(const time_type& start, const time_type& end, const search_op &condition) {
		typename value_time_vector::const_iterator i = search_entry_for_time(start);
		while (i != m_data.end()) {
			if (condition(i->value))
				return i;

			if (i->time >= end)
				break;
			std::advance(i, 1);
		}
		return m_data.end();
	}

	time_type search_result_right(const time_type& start, typename value_time_vector::const_iterator i) {
		if (i == m_data.end())
			return invalid_time_value<time_type>::value;

		if (i == m_data.begin())
			return std::max(start, m_start_time);
		else  {
			std::advance(i, -1);
			return std::max(i->time, start);
		}
	}

	time_type search_result_left(const time_type& start, typename value_time_vector::const_iterator i) {
		if (i != m_data.end())
			return std::min(time_just_before<time_type>::get(i->time), start);
		else 
			return invalid_time_value<time_type>::value;
	}


	template<class search_op> typename value_time_vector::const_iterator search_data_left_t(const time_type& start, const time_type& end, const search_op &condition) {
		if (m_data.size() == 0)
			return m_data.end();

		typename value_time_vector::const_iterator i = search_entry_for_time(start);
		if (i == m_data.end())
			std::advance(i, -1);

		while (true) {
			if (i->time <= end)
				break;

			if (condition(i->value))
				return i;

			if (i == m_data.begin())
				break;

			std::advance(i, -1);
		}
		return m_data.end();
	}


	void append_entry(const value_type& value, const time_type& time) {
		cache_block_size_updater(m_cache, this);
		if (!m_data.size())
			m_data.push_back(make_value_time_pair<value_time_type>(value, time));
		else {
			value_time_type& last_value_time = *m_data.rbegin();
			if (last_value_time.value == value) {
				last_value_time.time = time;
			} else {
				m_data.push_back(make_value_time_pair<value_time_type>(value, time));
			}
		}
	}

	void append_entries(typename value_time_vector::iterator begin, typename value_time_vector::iterator end) {
		if (begin == end)
			return;

		cache_block_size_updater(m_cache, this);
		append_entry(begin->value, begin->time);
		m_data.insert(m_data.end(), begin + 1, end);
	}


	typename value_time_vector::iterator insert_entry(typename value_time_vector::iterator i, const value_type& value, const time_type& time) {
		cache_block_size_updater(m_cache, this);
		return m_data.insert(i, make_value_time_pair<value_time_type>(value, time));
	}

	void set_data(value_time_vector& data) {
		cache_block_size_updater(m_cache, this);
		m_data.swap(data);
	}

	void maybe_merge_3_block_entries(typename value_time_vector::iterator i) {
		cache_block_size_updater(m_cache, this);
		if (i != m_data.begin()
				&& (i + 1) != m_data.end()
				&& value_cmp()((i - 1)->value, i->value)
				&& value_cmp()(i->value, (i + 1)->value))
		{
			(i - 1)->time = (i + 1)->time;
			value_merge()((i - 1)->value, i->value, (i + 1)->value);
			m_data.erase(i, i + 2);
		}
	}
	virtual ~value_time_block() {}

protected:
	value_time_vector m_data;
	time_type m_start_time;
};

template<class value_type, class time_type> class concrete_block : public value_time_block<value_time_pair<value_type, time_type> > {
public:
	typedef value_time_block<value_time_pair<value_type, time_type> > block_type;

	concrete_block(const time_type& start_time,
			block_cache* cache)
		:
			block_type(start_time, cache)
	{}
		
	void get_weighted_sum(const time_type& start_time, const time_type &end_time, weighted_sum<value_type, time_type>& r) {
		if (start_time >= end_time)
			return;

		time_type prev_time(start_time);
		bool done = false;
		typename block_type::value_time_vector::const_iterator i = this->search_entry_for_time(start_time);

		while (!done && i != this->m_data.end()) {
			typename time_difference<time_type>::type time_diff;
			if (i->time >= end_time) {
				time_diff = end_time - prev_time;
				done = true;
			} else {
				time_diff = i->time - prev_time;
			}
			prev_time = i->time;

			if (!value_is_no_data(i->value)) {
				r.add(i->value, time_diff);
			} else {
				r.no_data_weight() += time_diff;
			}

			std::advance(i, 1);
		}

		r.add_refferred_block(this);
	}

	virtual size_t block_size() const {
		return block_type::block_size();
	}
		
	time_type search_data_right(const time_type& start, const time_type& end, const search_condition &condition) {
		typename block_type::value_time_vector::const_iterator i = this->search_data_right_t(start, end, condition);
		return this->search_result_right(start, i);
	}

	time_type search_data_left(const time_type& start, const time_type& end, const search_condition &condition) {
		typename block_type::value_time_vector::const_iterator i = this->search_data_left_t(start, end, condition);
		return this->search_result_left(start, i);
	}

};

}

#endif
