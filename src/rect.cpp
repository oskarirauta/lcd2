#include <string>
#include "throws.hpp"
#include "rect.hpp"

RECT& RECT::operator =(const std::vector<int>& vec) {

	if ( vec.size() != 4 )
		throws << "wrong amount of arguments for &RECT operator " <<
			"=(std::vector<int>&), 4 is required" << std::endl;

	this -> min.x = vec[0]; this -> min.y = vec[1];
	this -> max.x = vec[2]; this -> max.y = vec[3];;
	return *this;
}

RECT& RECT::operator =(const std::vector<RECT::POINT>& vec) {

	if ( vec.size() != 2 )
		throws << "wrong amount of arguments for &RECT operator " <<
			"=(std::vector<point>&, 2 is required" << std::endl;

	this -> min = vec[0];
	this -> max = vec[1];
	return *this;
}

RECT& RECT::operator =(const RECT& other) {

	this -> min.x = other.min.x;
	this -> min.y = other.min.y;
	this -> max.x = other.max.x;
	this -> max.y = other.max.y;
	return *this;
}
