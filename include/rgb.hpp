#pragma once

#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

struct RGBA {

		struct Raw {
			unsigned char R = 0x00;
			unsigned char G = 0x00;
			unsigned char B = 0x00;
			unsigned char A = 0xff;
		};

		static Raw FG;
		static Raw BG;
		static Raw BL;
		static Raw NO;
		static Raw WHITE;
		static Raw BLACK;
		static Raw RED;
		static Raw GREEN;
		static Raw BLUE;
		static Raw TRANSPARENT;

		unsigned char R = 0x00;
		unsigned char G = 0x00;
		unsigned char B = 0x00;
		unsigned char A = 0xff;

		RGBA() noexcept;
		RGBA(int gdTrueColorPixel, bool inverted, double alpha = 1.0);
		RGBA(const std::string& hex, bool forced = false);
		RGBA(unsigned char R, unsigned char G, unsigned char B, unsigned char A) :
			R(R), G(G), B(B), A(A) {};
		explicit RGBA(Raw rgba) : R(rgba.R), G(rgba.G), B(rgba.B), A(rgba.A) {};

		bool operator ==(const RGBA& other);
		bool operator ==(const RGBA& other) const;
		bool operator !=(const RGBA& other);
		bool operator !=(const RGBA& other) const;

		RGBA& operator =(const RGBA& other);

		static const std::string hex_normalizer(const std::string& hex);
		static bool check_color(const std::string& hex);
		double alpha();

		std::vector<unsigned char> RGB565() const;

		unsigned char GD_alpha();
};
