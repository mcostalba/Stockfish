/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2012 Marco Costalba, Joona Kiiski, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined(PLATFORM_H_INCLUDED)
#define PLATFORM_H_INCLUDED

#if defined(_MSC_VER)

// Disable some silly and noisy warning from MSVC compiler
#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4146) // Unary minus operator applied to unsigned type
#pragma warning(disable: 4800) // Forcing value to bool 'true' or 'false'
#pragma warning(disable: 4996) // Function _ftime() may be unsafe

// MSVC does not support <inttypes.h>
typedef   signed __int8    int8_t;
typedef unsigned __int8   uint8_t;
typedef   signed __int16  int16_t;
typedef unsigned __int16 uint16_t;
typedef   signed __int32  int32_t;
typedef unsigned __int32 uint32_t;
typedef   signed __int64  int64_t;
typedef unsigned __int64 uint64_t;

#else
#  include <inttypes.h>
#endif

#if !defined(_WIN32) && !defined(_WIN64) // Linux - Unix

#  include <sys/time.h>
typedef timeval sys_time_t;

inline void system_time(sys_time_t* t) { gettimeofday(t, NULL); }
inline int64_t time_to_msec(const sys_time_t& t) { return t.tv_sec * 1000LL + t.tv_usec / 1000; }

#else // Windows and MinGW

#  include <sys/timeb.h>
typedef _timeb sys_time_t;

inline void system_time(sys_time_t* t) { _ftime(t); }
inline int64_t time_to_msec(const sys_time_t& t) { return t.time * 1000LL + t.millitm; }

#endif

#endif // !defined(PLATFORM_H_INCLUDED)
