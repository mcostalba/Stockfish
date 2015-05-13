/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad

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

#include <algorithm>
#include <cassert>
#include <ostream>

#include "misc.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "syzygy/tbprobe.h"

using std::string;

UCI::OptionsMap Options; // Global object

namespace UCI {

/// 'On change' actions, triggered by an option's value change
void on_clear_hash(const Option&) { Search::reset(); }
void on_hash_size(const Option& o) { TT.resize(o); }
void on_logger(const Option& o) { start_logger(o); }
void on_threads(const Option&) { Threads.read_uci_options(); }
void on_tb_path(const Option& o) { Tablebases::init(o); }


/// Our case insensitive less() function as required by UCI protocol
bool CaseInsensitiveLess::operator() (const string& s1, const string& s2) const {

  return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(),
         [](char c1, char c2) { return tolower(c1) < tolower(c2); });
}


/// init() initializes the UCI options to their hard-coded default values

void init(OptionsMap& o) {

  const int MaxHashMB = Is64Bit ? 1024 * 1024 : 2048;

  o["Write Debug Log"]       << Option(false, on_logger);
  o["Contempt"]              << Option(0, -100, 100);
  o["Min Split Depth"]       << Option(5, 0, 12, on_threads);
  o["Threads"]               << Option(1, 1, MAX_THREADS, on_threads);
  o["Hash"]                  << Option(16, 1, MaxHashMB, on_hash_size);
  o["Clear Hash"]            << Option(on_clear_hash);
  o["Ponder"]                << Option(true);
  o["MultiPV"]               << Option(1, 1, 500);
  o["Skill Level"]           << Option(20, 0, 20);
  o["Move Overhead"]         << Option(30, 0, 5000);
  o["Minimum Thinking Time"] << Option(20, 0, 5000);
  o["Slow Mover"]            << Option(80, 10, 1000);
  o["nodestime"]             << Option(0, 0, 10000);
  o["UCI_Chess960"]          << Option(false);
  o["SyzygyPath"]            << Option("<empty>", on_tb_path);
  o["SyzygyProbeDepth"]      << Option(1, 1, 100);
  o["Syzygy50MoveRule"]      << Option(true);
  o["SyzygyProbeLimit"]      << Option(6, 0, 6);
}


/// operator<<() is used to print all the options default values in chronological
/// insertion order (the idx field) and in the format defined by the UCI protocol.

std::ostream& operator<<(std::ostream& os, const OptionsMap& om) {

  for (size_t idx = 0; idx < om.size(); ++idx)
      for (const auto& it : om)
          if (it.second.idx == idx)
          {
              const Option& o = it.second;
              os << "\noption name " << it.first << " type " << o.type;

              if (o.type != "button")
                  os << " default " << o.defaultValue;

              if (o.type == "spin")
                  os << " min " << o.min << " max " << o.max;

              break;
          }

  return os;
}


/// Option class constructors and conversion operators

Option::Option(const char* v, OnChange f) : type("string"), min(0), max(0), on_change(f)
{ defaultValue = currentValue = v; }

Option::Option(bool v, OnChange f) : type("check"), min(0), max(0), on_change(f)
{ defaultValue = currentValue = (v ? "true" : "false"); }

Option::Option(OnChange f) : type("button"), min(0), max(0), on_change(f)
{}

Option::Option(int v, int minv, int maxv, OnChange f) : type("spin"), min(minv), max(maxv), on_change(f)
{ defaultValue = currentValue = std::to_string(v); }

Option::operator int() const {
  assert(type == "check" || type == "spin");
  return (type == "spin" ? stoi(currentValue) : currentValue == "true");
}

Option::operator std::string() const {
  assert(type == "string");
  return currentValue;
}


/// operator<<() inits options and assigns idx in the correct printing order

void Option::operator<<(const Option& o) {

  static size_t insert_order = 0;

  *this = o;
  idx = insert_order++;
}


/// operator=() updates currentValue and triggers on_change() action. It's up to
/// the GUI to check for option's limits, but we could receive the new value from
/// the user by console window, so let's check the bounds anyway.

Option& Option::operator=(const string& v) {

  assert(!type.empty());

  if (   (type != "button" && v.empty())
      || (type == "check" && v != "true" && v != "false")
      || (type == "spin" && (stoi(v) < min || stoi(v) > max)))
      return *this;

  if (type != "button")
      currentValue = v;

  if (on_change)
      on_change(*this);

  return *this;
}

} // namespace UCI



/// Tuning Framework. Fully separated from SF code, appended here to avoid
/// adding a *.cpp file and to modify Makefile.

#include <iostream>
#include <sstream>

typedef std::map<std::string, int, UCI::CaseInsensitiveLess> ResultsMap;
ResultsMap TuneResults;

string Tune::next(string& names, bool pop) {

  string name;

  do {
      string token = names.substr(0, names.find(','));

      if (pop)
          names.erase(0, token.size() + 1);

      std::stringstream ws(token);
      name += (ws >> token, token); // Remove trailing whitespace

  } while (  std::count(name.begin(), name.end(), '(')
           - std::count(name.begin(), name.end(), ')'));

  return name;
}

static void on_tune(const UCI::Option&) { Tune::read_options(); }

static void make_option(const string& n, int v, const SetRange& r) {

  // Do not generate option when there is nothing to tune (ie. min = max)
  if (r(v).first == r(v).second)
      return;

  if (TuneResults.count(n))
      v = TuneResults[n];

  Options[n] << UCI::Option(v, r(v).first, r(v).second, on_tune);

  // Print formatted parameters, ready to be copy-pasted in fishtest
  std::cout << n << ","
            << v << ","
            << r(v).first << "," << r(v).second << ","
            << (r(v).second - r(v).first) / 20.0 << ","
            << "0.0020"
            << std::endl;
}

template<> void Tune::Entry<int>::init_option() { make_option(name, value, range); }

template<> void Tune::Entry<int>::read_option() {
  if (Options.count(name))
      value = Options[name];
}

template<> void Tune::Entry<Value>::init_option() { make_option(name, value, range); }

template<> void Tune::Entry<Value>::read_option() {
  if (Options.count(name))
      value = Value(int(Options[name]));
}

template<> void Tune::Entry<Score>::init_option() {
  make_option("m" + name, mg_value(value), range);
  make_option("e" + name, eg_value(value), range);
}

template<> void Tune::Entry<Score>::read_option() {
  if (Options.count("m" + name) || Options.count("e" + name))
      value = make_score(Options["m" + name], Options["e" + name]);
}

// Instead of a variable here we have a PostUpdate function: just call it
template<> void Tune::Entry<Tune::PostUpdate>::init_option() {}
template<> void Tune::Entry<Tune::PostUpdate>::read_option() { value(); }


void Tune::read_results() {
// To update values use:
//
//  cat tune_result.txt | sed 's/^param: \([^,]*\), best: \([^,]*\).*/  TuneResults["\1"] = int(\2);/'
//
// And paste output here below:
//
    TuneResults["mLinear_0"] = int(1799.20);
    TuneResults["eLinear_0"] = int(1931.15);
    TuneResults["mLinear_1"] = int(-166.21);
    TuneResults["eLinear_1"] = int(-164.45);
    TuneResults["mLinear_2"] = int(-1023.07);
    TuneResults["eLinear_2"] = int(-1192.30);
    TuneResults["mLinear_3"] = int(-196.66);
    TuneResults["eLinear_3"] = int(-178.29);
    TuneResults["mLinear_4"] = int(226.05);
    TuneResults["eLinear_4"] = int(238.32);
    TuneResults["mLinear_5"] = int(-155.99);
    TuneResults["eLinear_5"] = int(-157.66);
    TuneResults["mQuadraticOurs_1_0"] = int(38.86);
    TuneResults["eQuadraticOurs_1_0"] = int(36.54);
    TuneResults["mQuadraticOurs_1_1"] = int(2.00);
    TuneResults["eQuadraticOurs_1_1"] = int(2.16);
    TuneResults["mQuadraticOurs_2_0"] = int(34.82);
    TuneResults["eQuadraticOurs_2_0"] = int(34.64);
    TuneResults["mQuadraticOurs_2_1"] = int(279.23);
    TuneResults["eQuadraticOurs_2_1"] = int(272.88);
    TuneResults["mQuadraticOurs_2_2"] = int(-4.20);
    TuneResults["eQuadraticOurs_2_2"] = int(-4.07);
    TuneResults["mQuadraticOurs_3_1"] = int(119.82);
    TuneResults["eQuadraticOurs_3_1"] = int(104.71);
    TuneResults["mQuadraticOurs_3_2"] = int(4.06);
    TuneResults["eQuadraticOurs_3_2"] = int(3.99);
    TuneResults["mQuadraticOurs_4_0"] = int(-25.77);
    TuneResults["eQuadraticOurs_4_0"] = int(-27.09);
    TuneResults["mQuadraticOurs_4_1"] = int(-1.95);
    TuneResults["eQuadraticOurs_4_1"] = int(-1.96);
    TuneResults["mQuadraticOurs_4_2"] = int(47.63);
    TuneResults["eQuadraticOurs_4_2"] = int(48.50);
    TuneResults["mQuadraticOurs_4_3"] = int(99.24);
    TuneResults["eQuadraticOurs_4_3"] = int(98.15);
    TuneResults["mQuadraticOurs_4_4"] = int(-146.73);
    TuneResults["eQuadraticOurs_4_4"] = int(-149.42);
    TuneResults["mQuadraticOurs_5_0"] = int(-171.85);
    TuneResults["eQuadraticOurs_5_0"] = int(-178.04);
    TuneResults["mQuadraticOurs_5_1"] = int(23.77);
    TuneResults["eQuadraticOurs_5_1"] = int(24.62);
    TuneResults["mQuadraticOurs_5_2"] = int(115.37);
    TuneResults["eQuadraticOurs_5_2"] = int(125.38);
    TuneResults["mQuadraticOurs_5_3"] = int(140.51);
    TuneResults["eQuadraticOurs_5_3"] = int(151.03);
    TuneResults["mQuadraticOurs_5_4"] = int(-140.95);
    TuneResults["eQuadraticOurs_5_4"] = int(-147.83);
    TuneResults["mQuadraticTheirs_1_0"] = int(38.22);
    TuneResults["eQuadraticTheirs_1_0"] = int(35.80);
    TuneResults["mQuadraticTheirs_2_0"] = int(10.49);
    TuneResults["eQuadraticTheirs_2_0"] = int(9.73);
    TuneResults["mQuadraticTheirs_2_1"] = int(62.33);
    TuneResults["eQuadraticTheirs_2_1"] = int(66.53);
    TuneResults["mQuadraticTheirs_3_0"] = int(57.32);
    TuneResults["eQuadraticTheirs_3_0"] = int(55.53);
    TuneResults["mQuadraticTheirs_3_1"] = int(62.29);
    TuneResults["eQuadraticTheirs_3_1"] = int(64.30);
    TuneResults["mQuadraticTheirs_3_2"] = int(41.09);
    TuneResults["eQuadraticTheirs_3_2"] = int(37.18);
    TuneResults["mQuadraticTheirs_4_0"] = int(51.85);
    TuneResults["eQuadraticTheirs_4_0"] = int(49.95);
    TuneResults["mQuadraticTheirs_4_1"] = int(41.41);
    TuneResults["eQuadraticTheirs_4_1"] = int(44.83);
    TuneResults["mQuadraticTheirs_4_2"] = int(21.47);
    TuneResults["eQuadraticTheirs_4_2"] = int(23.23);
    TuneResults["mQuadraticTheirs_4_3"] = int(-20.23);
    TuneResults["eQuadraticTheirs_4_3"] = int(-21.52);
    TuneResults["mQuadraticTheirs_5_0"] = int(96.06);
    TuneResults["eQuadraticTheirs_5_0"] = int(98.94);
    TuneResults["mQuadraticTheirs_5_1"] = int(101.70);
    TuneResults["eQuadraticTheirs_5_1"] = int(105.23);
    TuneResults["mQuadraticTheirs_5_2"] = int(-41.29);
    TuneResults["eQuadraticTheirs_5_2"] = int(-43.75);
    TuneResults["mQuadraticTheirs_5_3"] = int(153.60);
    TuneResults["eQuadraticTheirs_5_3"] = int(153.60);
    TuneResults["mQuadraticTheirs_5_4"] = int(263.06);
    TuneResults["eQuadraticTheirs_5_4"] = int(260.44);
}
