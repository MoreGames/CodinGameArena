#ifndef GHOSTINTHECELL_H
#define GHOSTINTHECELL_H

//#define DEBUG

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

using namespace std;
using namespace chrono;

constexpr int MAP_WIDTH{ 16000 };
constexpr int MAP_HEIGHT{ 6500 };
constexpr int EXTRA_SPACE_BETWEEN_FACTORIES{ 300 };
constexpr int MIN_FACTORY_COUNT{ 7 };
constexpr int MAX_FACTORY_COUNT{ 15 };
constexpr int MIN_PRODUCTION_RATE{ 0 };
constexpr int MAX_PRODUCTION_RATE{ 3 };
constexpr int MIN_TOTAL_PRODUCTION_RATE{ 4 };
constexpr int PLAYER_INIT_UNITS_MIN{ 15 };
constexpr int PLAYER_INIT_UNITS_MAX{ 30 };
constexpr int BOMBS_PER_PLAYER{ 2 };
constexpr int DAMAGE_DURATION{ 5 };
constexpr int FIRST_TURN_TIMEOUT{ 1005 }; // in milliseconds
constexpr int TURN_TIMEOUT{ 55 }; // in milliseconds
constexpr int TURN_LIMIT{ 200 };

class Point {
public:
	int m_x;
	int m_y;

	Point() {}; // default constructor for resize (generate factories)
	Point(int x, int y) : m_x(x), m_y(y) {};

	inline Point operator-(const Point& a) const {
		return Point{ m_x - a.m_x, m_y - a.m_y };
	}
	inline double distance(const Point& a) const {
		int x = (a.m_x - m_x);
		int y = (a.m_y - m_y);
		return sqrt(x*x + y*y);
	}
	friend ostream& operator<<(ostream& os, const Point& r) {
		os << r.m_x << " " << r.m_y;
		return os;
	}
};
class Entity {
public:
	size_t m_id;
	Bot* m_owner;

	Entity() {}; // default constructor for resize (generate factories)
	Entity(size_t id, Bot* owner) : m_id(id), m_owner(owner) {};

	inline string toPlayerString(string type, int arg1, int arg2, int arg3, int arg4, int arg5) const {
		return to_string(m_id) + " " + type + " " + to_string(arg1) + " " + to_string(arg2) + " " + to_string(arg3) + " " + to_string(arg4) + " " + to_string(arg5) + "\n";
	}
};
class Factory : public Entity, public Point {
public:
	int m_cyborgs;
	int m_production;
	int m_turns; // 0 producing, otherwise turns until producing again
	vector<int> m_dist; // distance to every other factory

	Factory() : Entity() {}; // default constructor for resize (generate factories)
	Factory(size_t id, Bot* owner, int cyborgs, int production, int turns, const Point& pos) : Entity(id, owner), Point(pos), m_cyborgs(cyborgs), m_production(production), m_turns(turns) {};

	inline string toBotString(int bot) const {
		int ownerShip = 0;
		if (m_owner != nullptr) {
			ownerShip = (bot == m_owner->m_id) ? 1 : -1;
		}
		return Entity::toPlayerString("FACTORY", ownerShip, m_cyborgs, m_production, m_turns, 0);
	}
	inline int getCurrentProduction() {
		return (m_turns == 0) ? m_production : 0;
	}
	inline void print() const {
		printf("%2d o:%d, p:%d, c:%2d, t:%d", m_id, m_owner == nullptr ? 0 : m_owner->m_id+1, m_production, m_cyborgs, m_turns);
	}
};
class MovingEntity : public Entity {
public:
	Factory* m_source;
	Factory* m_target;
	int m_remainingDistance;

	MovingEntity(size_t id, Bot* owner, Factory* source, Factory* target) : Entity(id, owner), m_source(source), m_target(target), m_remainingDistance(source->m_dist[target->m_id]) {};

	inline void move() {
		m_remainingDistance--;
	}
};
class Troop : public MovingEntity {
public:
	int m_cyborgs;

	Troop(size_t id, Bot* owner, Factory* source, Factory* target, int cyborgs) : MovingEntity(id, owner, source, target), m_cyborgs(cyborgs) {};

	inline string toBotString(int bot) const {
		int ownerShip = 0;
		if (m_owner != nullptr) {
			ownerShip = (bot == m_owner->m_id) ? 1 : -1;
		}
		return Entity::toPlayerString("TROOP", ownerShip, m_source->m_id, m_target->m_id, m_cyborgs, m_remainingDistance);
	}
	inline void print() const {
		printf("%3d o:%d, %2d->%2d, c:%2d, t:%d", m_id, m_owner == nullptr ? 0 : m_owner->m_id + 1, m_source->m_id, m_target->m_id, m_cyborgs, m_remainingDistance);
	}
};
class Bomb : public MovingEntity {
public:

	Bomb(size_t id, Bot* owner, Factory* source, Factory* target) : MovingEntity(id, owner, source, target) {};

	inline string toBotString(int bot) const {
		if (m_owner->m_id == bot) {
			return Entity::toPlayerString("BOMB", 1, m_source->m_id, m_target->m_id, m_remainingDistance, 0);
		} else {
			return Entity::toPlayerString("BOMB", -1, m_source->m_id, -1, -1, 0);
		}
	}
	inline void explode() const {
		int damage = min(m_target->m_cyborgs, max(10, m_target->m_cyborgs / 2));
		m_target->m_cyborgs -= damage;
		m_target->m_turns = DAMAGE_DURATION;
	}
	inline void print() const {
		printf("%2d o:%d, %2d->%2d, t:%d", m_id, m_owner == nullptr ? 0 : m_owner->m_id + 1, m_source->m_id, m_target->m_id, m_remainingDistance);
	}
};
enum class ACTION_TYPE {
	MOVE = 0,
	BOMB = 1,
	INC = 2
};
class Action {
public:
	ACTION_TYPE m_type;
	Bot* m_owner;
	Factory* m_source;
	Factory* m_target;
	int m_cyborgs;

	Action(Bot* owner, Factory* source) : m_type(ACTION_TYPE::INC), m_owner(owner), m_source(source), m_target(nullptr), m_cyborgs(0) {}; // INC
	Action(Bot* owner, Factory* source, Factory* target) : m_type(ACTION_TYPE::BOMB), m_owner(owner), m_source(source), m_target(target), m_cyborgs(0) {}; // BOMB
	Action(Bot* owner, Factory* source, Factory* target, int cyborgs) : m_type(ACTION_TYPE::MOVE), m_owner(owner), m_source(source), m_target(target), m_cyborgs(cyborgs) {}; // MOVE

	friend ostream& operator<<(ostream& os, const Action& a) {
		if (a.m_type == ACTION_TYPE::MOVE) {
			os << "MOVE s:" << setw(2) << (int)a.m_source << " t:" << setw(2) << (int)a.m_target << " c:" << setw(2) << (int)a.m_cyborgs << " o:" << setw(2) << (int)a.m_owner;
		} else if (a.m_type == ACTION_TYPE::BOMB) {
			os << "BOMB s:" << setw(2) << (int)a.m_source << " t:" << setw(2) << (int)a.m_target << " o:" << setw(2) << (int)a.m_owner;
		} else if (a.m_type == ACTION_TYPE::INC) {
			os << "INC s:" << setw(2) << (int)a.m_source << " o:" << setw(2) << (int)a.m_owner;
		}
		return os;
	}
};

class GhostInTheCell : public Game {
private:
	size_t id;

	// game state
	vector<Factory> m_factories;
	vector<Troop> m_troops;
	vector<Bomb> m_bombs;
	vector<Bomb> m_newBombs;
	vector<size_t> m_remainingBombs; // remaining bombs, default 2

	vector<Action> m_bombActions;
	vector<Action> m_moveActions;
	vector<Action> m_incActions;

	inline bool invalidFactoryId(size_t id) noexcept {
		return id < 0 || id >= m_factories.size();
	}
	inline bool validFactorySpawn(const Point& p, size_t id, size_t minSpaceBetweenFactories) {
		for (size_t i = 0; i < id; i++) {
			if (m_factories[i].distance(p) < minSpaceBetweenFactories) {
				return false;
			}
		}
		return true;
	}
	inline bool botAlive(size_t owner) noexcept {
		// factory with production and/or cyborgs
		// any troop
		return find_if(m_factories.begin(), m_factories.end(), [&](const Factory& f) {
			return f.m_owner != nullptr && f.m_owner->m_id == owner && (f.m_cyborgs != 0 || f.m_production != 0); 
		}) != m_factories.end() || 
			find_if(m_troops.begin(), m_troops.end(), [&](const Troop& t) {
			return t.m_owner->m_id == owner; 
		}) != m_troops.end();
	}
	inline bool botWon(size_t bot) {
		for (size_t i = 0; i<m_bots.size(); i++) {
			if (i != bot && botAlive(i)) {
				return false;
			}
		}
		return true;
	}
	inline bool allDead() {
		for (const Bot& b : m_bots) {
			if (botAlive(b.m_id)) {
				return false;
			}
		}
		return true;
	}
	inline int cyborgCount(int bot) noexcept {
		int cyborgs{ 0 };
		for (const Factory& f : m_factories) {
			if (f.m_id == bot) {
				cyborgs = f.m_cyborgs;
			}
		}
		for (const Troop& t : m_troops) {
			if (t.m_id == bot) {
				cyborgs = t.m_cyborgs;
			}
		}
		return cyborgs;
	}
	inline bool findWithSameRoute(const MovingEntity& entity, const vector<Bomb>& bombs) {
		for (const Bomb& b : bombs) {
			if (b.m_source->m_id == entity.m_source->m_id && b.m_target->m_id == entity.m_target->m_id) {
				return true;
			}
		}
		return false;
	}

	void generateFactories();

	void botActions(const string& input, int bot);
	void deserializeActions(const string& input, int bot, vector<Action>& actions);
	void groupActions(vector<Action>& actions);
	void validateActions(const vector<Action>& actions);
	void processActions(const vector<Action>& actions);

	void output(int turn);
	void updateGame();

	string serializeInitBotInput(int bot) const;
	string serializeBotInput(int turn, int bot) const;

public:
	GhostInTheCell(const vector<string>& botNames, long long seed = system_clock::now().time_since_epoch().count());

	int run();

	~GhostInTheCell();
};






#endif
