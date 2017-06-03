#ifndef CODE4LIFE_H
#define CODE4LIFE_H

#define DEBUG

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <chrono>
#include <array>
#include <map>
#include <random>
#include <future>

#include "Game.hpp"
#include "Collections.hpp"

using namespace std;
using namespace chrono;

constexpr int MAX_STORAGE = 10;
constexpr int MAX_TRAY = 3;
constexpr int SAMPLE_RANK_COUNT = 3;
constexpr int MAX_SCORE = 170;

#define LEAGUE_LEVEL 3

#if LEAGUE_LEVEL == 0 // wood 2
constexpr int RESOURCE_PER_TYPE_BY_LEAGUE_LEVEL{ 99 };
constexpr int INIT_DIAGNOSED_SAMPLES_BY_LEAGUE_LEVEL{ 50 };
constexpr int SCIENCE_PROJECTS_BY_LEAGUE_LEVEL{ 0 };
constexpr int SCIENCE_PROJECT_VALUE{ 30 };
#elif LEAGUE_LEVEL == 1 // wood 1
constexpr int RESOURCE_PER_TYPE_BY_LEAGUE_LEVEL{ 99 };
constexpr int INIT_DIAGNOSED_SAMPLES_BY_LEAGUE_LEVEL{ 0 };
constexpr int SCIENCE_PROJECTS_BY_LEAGUE_LEVEL{ 0 };
constexpr int SCIENCE_PROJECT_VALUE{ 30 };
#elif LEAGUE_LEVEL == 2 // bronze
constexpr int RESOURCE_PER_TYPE_BY_LEAGUE_LEVEL{ 6 };
constexpr int INIT_DIAGNOSED_SAMPLES_BY_LEAGUE_LEVEL{ 0 };
constexpr int SCIENCE_PROJECTS_BY_LEAGUE_LEVEL{ 3 };
constexpr int SCIENCE_PROJECT_VALUE{ 30 };
#else // silver
constexpr int RESOURCE_PER_TYPE_BY_LEAGUE_LEVEL{ 5 };
constexpr int INIT_DIAGNOSED_SAMPLES_BY_LEAGUE_LEVEL{ 0 };
constexpr int SCIENCE_PROJECTS_BY_LEAGUE_LEVEL{ 3 };
constexpr int SCIENCE_PROJECT_VALUE{ 50 };
#endif

constexpr int FIRST_TURN_TIMEOUT{ 10000005 }; // in milliseconds
constexpr int TURN_TIMEOUT{ 11000055 }; // in milliseconds
constexpr int TURN_LIMIT{ 200 };

enum class MODULE_TYPE {
	SAMPLES = 0, DIAGNOSIS = 1, MOLECULES = 2, LABORATORY = 3, START_POS = 4
};
class BotInfo {
private:
	inline string targetString() const {
		if (m_target == MODULE_TYPE::SAMPLES) {
			return "SAMPLES";
		} else if (m_target == MODULE_TYPE::DIAGNOSIS) {
			return "DIAGNOSIS";
		} else if (m_target == MODULE_TYPE::MOLECULES) {
			return "MOLECULES";
		} else if (m_target == MODULE_TYPE::LABORATORY) {
			return "LABORATORY";
		} else {
			return "START_POS";
		}
	}

public:
	Bot* m_bot;
	MODULE_TYPE m_target;
	int m_eta;
	int m_score;
	int m_storage[5];
	int m_expertise[5];

	BotInfo(Bot* bot) : m_bot(bot), m_target(MODULE_TYPE::START_POS), m_eta(0), m_score(0) {
		for (size_t i = 0; i < 5; i++) {
			m_storage[i] = m_expertise[i] = 0;
		}
	};

	inline string toBotString() const {
		return targetString() + " " + to_string(m_eta) + " " + to_string(m_score) + " " +
			to_string(m_storage[0]) + " " + to_string(m_storage[1]) + " " + to_string(m_storage[2]) + " " + to_string(m_storage[3]) + " " + to_string(m_storage[4]) + " " +
			to_string(m_expertise[0]) + " " + to_string(m_expertise[1]) + " " + to_string(m_expertise[2]) + " " + to_string(m_expertise[3]) + " " + to_string(m_expertise[4]) + "\n";
	}
	inline void print() const {
		printf("%d t:%10s eta:%d score:%3d sto:%d,%d,%d,%d,%d exp:%d,%d,%d,%d,%d", m_bot->m_id, targetString().c_str(), m_eta, m_score, m_storage[0], m_storage[1], m_storage[2], m_storage[3], m_storage[4], m_expertise[0], m_expertise[1], m_expertise[2], m_expertise[3], m_expertise[4]);
	}
	inline int getSumStorage() {
		int sum = 0;
		for (size_t i = 0; i < 5; i++) {
			sum += m_storage[i];
		}
		return sum;
	}
};
class Sample {
private:
	inline string expertiseString() const {
		if (m_expertise == 0) {
			return "A";
		} else if (m_expertise == 1) {
			return "B";
		} else if (m_expertise == 2) {
			return "C";
		} else if (m_expertise == 3) {
			return "D";
		} else {
			return "E";
		}
	}
public:
	int m_id;
	int m_expertise;
	int m_life;
	int m_costs[5];
	int m_rank;
	bool m_discovered;
	BotInfo* m_discoveredBy;
	BotInfo* m_carriedBy;

	// TODO bot m_discoveredBy, m_carriedBy 
	Sample(int costs[5], int life, int expertise, int rank) : m_expertise(expertise), m_life(life), m_rank(rank), m_discovered(false), m_discoveredBy(nullptr), m_carriedBy(nullptr) {
		memcpy(m_costs, costs, sizeof(m_costs));
	};

	inline string toBotString(int bot) const {
		string s{ "" };
		s += to_string(m_id) + " " + to_string(m_carriedBy == nullptr ? -1 : (m_carriedBy->m_bot->m_id == bot ? 0 : 1)) + " " + to_string(m_rank + 1) + " ";
		if (m_discovered) {
			s += expertiseString() + " " + to_string(m_life) + " " + to_string(m_costs[0]) + " " + to_string(m_costs[1]) + " " + to_string(m_costs[2]) + " " + to_string(m_costs[3]) + " " + to_string(m_costs[4]) + "\n";
		} else {
			s += "0 -1 -1 -1 -1 -1 -1\n";
		}
		return s;
	}
	inline void print() const {
		printf("%d c:%d r:%d e:%d l:%d d:%d cost:%d,%d,%d,%d,%d", m_id, m_carriedBy == nullptr ? -1 : m_carriedBy->m_bot->m_id, m_rank, m_expertise, m_life, m_discovered, m_costs[0], m_costs[1], m_costs[2], m_costs[3], m_costs[4]);
	}
};
class ScienceProject {
public:
	int m_costs[5];
	bool m_remove;

	ScienceProject(int cost[5]) : m_remove(false) {
		memcpy(m_costs, cost, sizeof(m_costs));
	};

	inline string toBotString() const {
		return to_string(m_costs[0]) + " " + to_string(m_costs[1]) + " " + to_string(m_costs[2]) + " " + to_string(m_costs[3]) + " " + to_string(m_costs[4]) + "\n";
	}
	inline void print() const {
		printf("costs:%d,%d,%d,%d,%d", m_costs[0], m_costs[1], m_costs[2], m_costs[3], m_costs[4]);
	}
};
enum class ACTION_TYPE {
	WAIT = 0,
	GOTO = 1,
	CONNECT = 2
};
class Action {
public:
	ACTION_TYPE m_type;
	MODULE_TYPE m_target;
	int m_connectionId;

	Action(ACTION_TYPE type) : m_type(type) {}; // WAIT
	Action(ACTION_TYPE type, MODULE_TYPE target) : m_type(type), m_target(target) {}; // GOTO
	Action(ACTION_TYPE type, int connectionId) : m_type(type), m_connectionId(connectionId) {}; // CONNECT
};

class Code4Life : public Game {
private:
	size_t id;

	// game state
	array<int, 5> m_molecules;
	array<vector<Sample>, 3> m_samplePool;
	vector<Sample> m_samplesInGame;
	vector<ScienceProject> m_scienceProjects;
	vector<BotInfo> m_botInfos;

	inline bool gameIsOver() {
		if (LEAGUE_LEVEL >= 3) {
			return false;
		} else {
			for (size_t i = 0; i < m_botInfos.size(); i++) {
				if (m_botInfos[i].m_score >= MAX_SCORE) {
					return true;
				}
			}
			return false;
		}
	}
	inline int evaluateWinner() {
		int score[2] = { 0, 0 };
		for (size_t i = 0; i < m_botInfos.size(); i++) {
			score[i] = m_botInfos[i].m_score;
		}
		if (score[0] == score[1]) {
			return -1;
		} else if (score[0] > score[1]) {
			return 0;
		} else {
			return 1;
		}
	}

	void deserializeActions(const vector<string>& botActions, vector<Action>& actions);
	void output(int turn);

	Sample* getSample(int id);
	Sample* getSample(int bot, int id);
	void removeSample(Sample* sample);
	int getSampleSize(int bot);

	void requestSample(int bot, int rank);
	void requestDiagnosis(int bot, int id);

	void requestMolecule(int bot, int molecule);
	bool canAfford(int bot, int cost[5]);
	void requestProduction(int bot, int id);
	bool completedProject(int bot, const ScienceProject& project);
	int getDistance(MODULE_TYPE from, MODULE_TYPE to);
	void updateGame(const vector<string>& botActions);

	string serializeInitBotInput(int bot) const;
	string serializeBotInput(int turn, int bot) const;

public:
	Code4Life(const vector<string>& botNames, long long seed = system_clock::now().time_since_epoch().count());

	int run();

	~Code4Life();
};

#endif