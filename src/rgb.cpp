#include <gd.h>
#include "rgb.hpp"

RGBA::Raw RGBA::FG({ .R = 0x00, .G = 0x00, .B = 0x00, .A = 0xff });
RGBA::Raw RGBA::BG({ .R = 0xff, .G = 0xff, .B = 0xff, .A = 0xff });
RGBA::Raw RGBA::BL({ .R = 0xff, .G = 0xff, .B = 0xff, .A = 0x00 });
RGBA::Raw RGBA::NO({ .R = 0x00, .G = 0x00, .B = 0x00, .A = 0x00 });
RGBA::Raw RGBA::BLACK({ .R = 0x00, .G = 0x00, .B = 0x00, .A = 0xff });
RGBA::Raw RGBA::WHITE({ .R = 0xff, .G = 0xff, .B = 0xff, .A = 0xff });
RGBA::Raw RGBA::RED({ .R = 0xff, .G = 0x00, .B = 0x00, .A = 0xff });
RGBA::Raw RGBA::GREEN({ .R = 0x00, .G = 0xff, .B = 0x00, .A = 0xff });
RGBA::Raw RGBA::BLUE({ .R = 0x00, .G = 0x00, .B = 0xff, .A = 0xff });
RGBA::Raw RGBA::TRANSPARENT({ .R = 0x00, .G = 0x00, .B = 0x00, .A = 0x00 });

RGBA::RGBA() noexcept {
	this -> R = 0;
	this -> G = 0;
	this -> B = 0;
	this -> A = 0xff;
}

bool RGBA::check_color(const std::string& hex) {

	std::string s(hex);

	if ( s.front() == '#' )
		s.erase(0, 1);

	if ( s.size() < 3 || s.size() == 7 || s.size() > 8 )
		return false;

	for ( char ch : hex )
		if ( !std::isdigit(ch) && ( ch != 'a' && ch != 'b' && ch != 'c' && ch != 'd' && ch != 'e' && ch != 'f' ))
			return false;

	return true;
}

const std::string RGBA::hex_normalizer(const std::string& hex) {

	std::string in(hex);

	while ( !in.empty() && in.at(0) == '#' )
		in.erase(0, 1);

	std::string out(in);

	if ( in.size() == 3 )
		out = in.at(0) + in.at(0) + in.at(1) + in.at(1) + in.at(2) + in.at(2);
	else if ( in.size() == 4 )
		out = in.at(0) + in.at(0) + in.at(1) + in.at(1) + in.at(2) + in.at(2) + in.at(3) + in.at(3);
	else if ( in.size() == 5 )
		out = in.at(0) + in.at(0) + in.at(1) + in.at(1) + in.at(2) + in.at(2) + in.at(3) + in.at(4);

	if ( out.empty())
		throw std::runtime_error("hex color string is empty");
	if ( out.size() != 6 && out.size() != 8 )
		throw std::runtime_error("hex color string length not acceptable: " + hex);

	for ( auto &c : out )
		c = std::tolower(c);

	if ( std::string::npos != out.find_first_not_of("0123456789abcdef"))
		throw std::runtime_error("hex color contains letters outside of hex range (0123456789abcdef): " + hex);

	return out;
}

RGBA::RGBA(int gdTrueColorPixel, bool inverted, double alpha) {

	this -> R = inverted ? ( 255 - gdTrueColorGetRed(gdTrueColorPixel)) : gdTrueColorGetRed(gdTrueColorPixel);
	this -> G = inverted ? ( 255 - gdTrueColorGetGreen(gdTrueColorPixel)) : gdTrueColorGetGreen(gdTrueColorPixel);
	this -> B = inverted ? ( 255 - gdTrueColorGetBlue(gdTrueColorPixel)) : gdTrueColorGetBlue(gdTrueColorPixel);

	unsigned char gdAlpha = gdTrueColorGetAlpha(gdTrueColorPixel);
	gdAlpha = gdAlpha == 127 ? 0 : ( 255 - ( 2 * gdAlpha ));

	if ( alpha < 1.0 && alpha >= 0 ) {
		double _a = (( 255 * 0.01 ) * ( alpha * 100));
		if ( _a < 0 ) _a = 0;
		else if ( _a > 255 ) _a = 255;
		_a = 255 - _a;
		unsigned char a = (unsigned char)_a;
		if ( gdAlpha - a < 0 ) gdAlpha = 0;
		else gdAlpha-=a;
	}

	this -> A = gdAlpha;
}

RGBA::RGBA(const std::string& hex, bool forced) {

	std::string c;

	try {
		c = RGBA::hex_normalizer(hex);
	} catch(std::runtime_error& e) {
		if ( !forced )
			throw std::runtime_error(e.what());
		else c = "000000ff";
	}

	char *e;
	unsigned long l = strtoul(c.c_str(), &e, 16);

	if ( e != NULL && *e != '\0' )
		throw std::runtime_error("hex color string contains un-allowed letters (allowed: 0123456789abcdef): " + hex);

	if ( c.size() == 8 ) {
		this -> R = ( l >> 24 ) & 0xff;
		this -> G = ( l >> 16 ) & 0xff;
		this -> B = ( l >> 8 ) & 0xff;
		this -> A = ( l >> 0 ) & 0xff;
	} else {
		this -> R = ( l >> 16 ) & 0xff;
		this -> G = (l >> 8 ) & 0xff;
		this -> B = l & 0xff;
		this -> A = 0xff;
	}
}

bool RGBA::operator ==(const RGBA& other) {

	return this -> R == other.R && this -> G == other.G &&
		this -> B == other.B && this -> A == other.A;
}

bool RGBA::operator ==(const RGBA& other) const {

	return this -> R == other.R && this -> G == other.G &&
		this -> B == other.B && this -> A == other.A;

}

bool RGBA::operator !=(const RGBA& other) {

	return !(*this == other);
}

bool RGBA::operator !=(const RGBA& other) const {

	return !(*this == other);
}

RGBA& RGBA::operator =(const RGBA& other) {

	this -> R = other.R;
	this -> G = other.G;
	this -> B = other.B;
	this -> A = other.A;
	return *this;
}

double RGBA::alpha() {
	return this -> A == 0 ? 0 : ( this -> A / 0xff );
}

std::vector<unsigned char> RGBA::RGB565() const {

	return {
		(unsigned char)(( this -> R & 0xf8 ) | (( this -> G & 0xe0 ) >> 5 )),
		(unsigned char)((( this -> G & 0x1c ) << 3 ) | (( this -> B & 0xf8 ) >> 3 ))
		};
}

unsigned char RGBA::GD_alpha() {
	unsigned char a = this -> A * 0.5;
	if ( a > 127 ) a = 127;

	return 127 - a;
}
