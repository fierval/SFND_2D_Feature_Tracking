#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

template <typename T = int>
class CsvLogger {

	typedef std::unordered_map<std::string, std::vector<T>> Result;

public:

	CsvLogger<T>(std::string _header, std::string _fn) : fn(_fn)
	{
		header += _header;
		header += ",1,2,3,4,5,6,7,8,9,10";
	}

	void add_result(std::string key, T res) {
		// we may be collecting some data over an over again.
		// if so, do nothing
		if (result_map.count(key) > 0 && result_map[key].size() >= 10) {
			return;
		}
		else if(result_map.count(key) == 0) {
			result_map.insert(std::make_pair(key, vector<T>));
		}

		result_map[key].push_back(res);
	}

	void add_result(std::string& detector, std::string& descriptor, T res) {
		std::string key = detector + "/" + descriptor;
		add_result(key, res);
	}

	void dump() {
		std::ofstream out(fn, std::ofstream::out | std::ofstream::trunc);
		
		out << header << std::endl;

		std::for_each(result_map.begin(), result_map.end(), [&out](std::pair<std::string, std::vector<T>> kv) {
			
			out << kv.first;
			std::copy(kv.second.begin(), kv.second.end() - 1, std::ostream_iterator<T>(out, ","));
			out << *(kv.second.end() - 1) << std::endl;

			});
		out.flush();
	}

private:

	std::string fn, header;
	Result result_map;
};