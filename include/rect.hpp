#pragma once

#include <vector>

struct RECT {

	struct POINT { int x, y; };

		POINT min, max;

		constexpr int& minx()	{ return this -> min.x; }
		constexpr int& miny()	{ return this -> min.y; }
		constexpr int& maxx()	{ return this -> max.x; }
		constexpr int& maxy()	{ return this -> max.y; }
		constexpr int width()	{ return this -> max.x - this -> min.x; }
		constexpr int height()	{ return this -> max.y - this -> min.y; }

		constexpr void minx(int value)	{ this -> min.x = value; }
		constexpr void miny(int value)	{ this -> min.y = value; }
		constexpr void maxx(int value)	{ this -> max.x = value; }
		constexpr void maxy(int value)	{ this -> max.y = value; }

		RECT& operator =(const std::vector<int>& vec);
		RECT& operator =(const std::vector<POINT>& vec);
		RECT& operator =(const RECT& other);

		RECT() : min(POINT { .x = 0, .y = 0 }), max(POINT { .x = 0, .y = 0 }) {}
		RECT(int maxx, int maxy) : min(POINT { .x = 0, .y = 0 }), max(POINT { .x = maxx, .y = maxy }) {}
		RECT(POINT max) : min(POINT { .x = 0, .y = 0 }), max(max) {}
		RECT(int minx, int miny, int maxx, int maxy) : min(POINT { .x = minx, .y = miny }), max(POINT { .x = maxx, .y = maxy }) {}
		RECT(POINT min, POINT max) : min(min), max(max) {}
		RECT(const RECT& other) : min(other.min), max(other.max) {}
};
