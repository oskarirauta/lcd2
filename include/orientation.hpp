#pragma once

class ORIENTATION {

	private:
		unsigned int _value = 0;

		ORIENTATION(unsigned int value) : _value(value) {};

	public:

		inline unsigned int value() { return this -> _value < 4 ? this -> _value : 0 ; }
		inline unsigned int angle() {
			return this -> _value == 0 ? 0 :
				( this -> _value == 1 ? 90 :
				( this -> _value == 2 ? 180 :
				( this -> _value == 3 ? 270 : 0 )));
		}
		inline bool isUpsideDown() { return this -> value() == 1 || this -> value() == 3; }
		inline bool isFlipped() { return this -> value() == 2 || this -> value() == 3; }
		inline bool isPortrait() { return this -> value() == 1 || this -> value() == 3; }
		inline bool isRotated0() { return this -> value() == 0; }
		inline bool isRotated90() { return this -> value() == 1; }
		inline bool isRotated180() { return this -> value() == 2; }
		inline bool isRotated270() { return this -> value() == 3; }

		static inline ORIENTATION rotate() { return ORIENTATION(0); }
		static inline ORIENTATION rotate0() { return ORIENTATION(0); }
		static inline ORIENTATION rotate90() { return ORIENTATION(1); }
		static inline ORIENTATION rotate180() { return ORIENTATION(2); }
		static inline ORIENTATION rotate270() { return ORIENTATION(3); }
		static inline ORIENTATION landscape() { return ORIENTATION(0); }
		static inline ORIENTATION portrait() { return ORIENTATION(1); }
		static inline ORIENTATION landscapeFlipped() { return ORIENTATION(2); }
		static inline ORIENTATION portraitFlipped() { return ORIENTATION(3); }

		explicit ORIENTATION() : _value(0) {};
};
