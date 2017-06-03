#ifndef CODERSOFTHECARIBBEAN_H
#define CODERSOFTHECARIBBEAN_H

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

using namespace std;
using namespace chrono;

constexpr int MAP_WIDTH{ 23 };
constexpr int MAP_HEIGHT{ 21 };
constexpr int COOLDOWN_CANNON{ 2 };
constexpr int COOLDOWN_MINE{ 5 };
constexpr int INITIAL_SHIP_HEALTH{ 100 };
constexpr int MAX_SHIP_HEALTH{ 100 };
constexpr int MIN_SHIPS{ 1 };
constexpr int MIN_RUM_BARRELS{ 10 };
constexpr int MAX_RUM_BARRELS{ 26 };
constexpr int MIN_RUM_BARREL_VALUE{ 10 };
constexpr int MAX_RUM_BARREL_VALUE{ 20 };
constexpr int REWARD_RUM_BARREL_VALUE{ 30 };
constexpr int MINE_VISIBILITY_RANGE{ 5 };
constexpr int FIRE_DISTANCE_MAX{ 10 };
constexpr int LOW_DAMAGE{ 25 };
constexpr int HIGH_DAMAGE{ 50 };
constexpr int MINE_DAMAGE{ 25 };
constexpr int NEAR_MINE_DAMAGE{ 10 };

#define LEAGUE_LEVEL 3

#if LEAGUE_LEVEL == 0 // 1 ship / no mines / speed 1
constexpr int MAX_SHIPS{ 1 };
constexpr int CANNONS_ENABLED{ false };
constexpr int MINES_ENABLED{ false };
constexpr int MIN_MINES{ 0 };
constexpr int MAX_MINES{ 0 };
constexpr int MAX_SHIP_SPEED{ 1 };
#elif LEAGUE_LEVEL == 1 // add mines
constexpr int MAX_SHIPS{ 1 };
constexpr int CANNONS_ENABLED{ true };
constexpr int MINES_ENABLED{ true };
constexpr int MIN_MINES{ 5 };
constexpr int MAX_MINES{ 10 };
constexpr int MAX_SHIP_SPEED{ 1 };
#elif LEAGUE_LEVEL == 2 // 3 ships max
constexpr int MAX_SHIPS{ 3 };
constexpr int CANNONS_ENABLED{ true };
constexpr int MINES_ENABLED{ true };
constexpr int MIN_MINES{ 5 };
constexpr int MAX_MINES{ 10 };
constexpr int MAX_SHIP_SPEED{ 1 };
#else // increase max speed
constexpr int MAX_SHIPS{ 3 };
constexpr int CANNONS_ENABLED{ true };
constexpr int MINES_ENABLED{ true };
constexpr int MIN_MINES{ 5 };
constexpr int MAX_MINES{ 10 };
constexpr int MAX_SHIP_SPEED{ 2 };
#endif

constexpr double PI = 3.14159265358979323846;
constexpr int FIRST_TURN_TIMEOUT{ 10000005 }; // in milliseconds
constexpr int TURN_TIMEOUT{ 11000055 }; // in milliseconds
constexpr int TURN_LIMIT{ 200 };

constexpr int DIRECTIONS_EVEN[6][2] = { { 1, 0 }, { 0, -1 }, { -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, 1 } };
constexpr int DIRECTIONS_ODD[6][2] = { { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, 0 }, { 0, 1 }, { 1, 1 } };
constexpr int CUBE_DIRECTIONS[6][3] = { { 1, -1, 0 }, { +1, 0, -1 }, { 0, +1, -1 }, { -1, +1, 0 }, { -1, 0, +1 }, { 0, -1, +1 } };
class CubeCoordinate {
public:
	int m_x, m_y, m_z;

	CubeCoordinate(int x, int y, int z) : m_x(x), m_y(y), m_z(z) {};

	inline CubeCoordinate neighbor(int orientation) const {
		int nx = m_x + CUBE_DIRECTIONS[orientation][0];
		int ny = m_y + CUBE_DIRECTIONS[orientation][1];
		int nz = m_z + CUBE_DIRECTIONS[orientation][2];

		return CubeCoordinate(nx, ny, nz);
	}
	inline int distanceTo(const CubeCoordinate& dst) const {
		return (abs(m_x - dst.m_x) + abs(m_y - dst.m_y) + abs(m_z - dst.m_z)) / 2;
	}
};
class Coord {
public:
	int m_x, m_y;

	Coord(int x, int y) : m_x(x), m_y(y) {};
	Coord(const Coord& other) : m_x(other.m_x), m_y(other.m_y) {};

	double angle(const Coord& targetPosition) const {
		double dy = (targetPosition.m_y - m_y) * sqrt(3) / 2;
		double dx = targetPosition.m_x - m_x + ((m_y - targetPosition.m_y) & 1) * 0.5;
		double angle = -atan2(dy, dx) * 3 / PI;
		if (angle < 0) {
			angle += 6;
		} else if (angle >= 6) {
			angle -= 6;
		}
		return angle;
	}

	CubeCoordinate toCubeCoordinate() const {
		int xp = m_x - (m_y - (m_y & 1)) / 2;
		int zp = m_y;
		int yp = -(xp + zp);
		return CubeCoordinate(xp, yp, zp);
	}

	Coord neighbor(int orientation) const {
		int newY, newX;
		if (m_y % 2 == 1) {
			newY = m_y + DIRECTIONS_ODD[orientation][1];
			newX = m_x + DIRECTIONS_ODD[orientation][0];
		} else {
			newY = m_y + DIRECTIONS_EVEN[orientation][1];
			newX = m_x + DIRECTIONS_EVEN[orientation][0];
		}

		return Coord(newX, newY);
	}

	bool isInsideMap() const {
		return m_x >= 0 && m_x < MAP_WIDTH && m_y >= 0 && m_y < MAP_HEIGHT;
	}

	int distanceTo(Coord dst) const {
		return toCubeCoordinate().distanceTo(dst.toCubeCoordinate());
	}

	bool operator==(const Coord& o) const {
		return (m_x == o.m_x) && (m_y == o.m_y);
	}
};
enum class ENTITY_TYPE {
	EMPTY = 0,
	SHIP = 1,
	BARREL = 2,
	MINE = 3,
	CANNONBALL = 4
};
class Entity {
private:
	inline string typeName() const {
		if (m_type == ENTITY_TYPE::EMPTY) {
			return "EMPTY";
		} else if (m_type == ENTITY_TYPE::SHIP) {
			return "SHIP";
		} else if (m_type == ENTITY_TYPE::BARREL) {
			return "BARREL";
		} else if (m_type == ENTITY_TYPE::MINE) {
			return "MINE";
		} else if (m_type == ENTITY_TYPE::CANNONBALL) {
			return "CANNONBALL";
		} else {
			return "ERROR";
		}
	}
public:
	ENTITY_TYPE m_type;
	size_t m_id;
	Coord m_position;

	//Entity() {}; // default constructor for resize (generate factories)
	Entity(ENTITY_TYPE type, size_t id, int x, int y) : m_type(type), m_id(id), m_position(x, y) {};

	inline string toBotString(int arg1, int arg2, int arg3, int arg4) const {
		return to_string(m_id) + " " + typeName() + " " + to_string(m_position.m_x) + " " + to_string(m_position.m_y) + " " + to_string(arg1) + " " + to_string(arg2) + " " + to_string(arg3) + " " + to_string(arg4) + "\n";
	}
};
class Damage {
public:
	Coord m_position;
	int m_health;
	bool m_hit;

	Damage(Coord position, int health, bool hit) : m_position(position), m_health(health), m_hit(hit) {};
};
 class Cannonball : public Entity {
 public:
	int m_ownerEntityId;
	int m_srcX;
	int m_srcY;
	int m_initialRemainingTurns;
	int m_remainingTurns;

	Cannonball(size_t id, int x, int y, int ownerEntityId, int srcX, int srcY, int remainingTurns) :
		Entity(ENTITY_TYPE::CANNONBALL, id, x, y),
		m_ownerEntityId(ownerEntityId),
		m_srcX(srcX),
		m_srcY(srcY),
		m_initialRemainingTurns(remainingTurns),
		m_remainingTurns(remainingTurns) 
	{};

	inline string toBotString(int bot) const {
		return Entity::toBotString(m_ownerEntityId, m_remainingTurns, 0, 0);
	}
	inline void print() const {
		printf("%2d x,y:%2d,%2d, it:%d, t:%d", m_id, m_position.m_x, m_position.m_y, m_initialRemainingTurns, m_remainingTurns);
	}
};
class RumBarrel : public Entity {
public:
	int m_health;

	RumBarrel(size_t id, int x, int y, int health) : Entity(ENTITY_TYPE::BARREL, id, x, y), m_health(health) {};

	inline string toBotString(int bot) const {
		return Entity::toBotString(m_health, 0, 0, 0);
	}
	inline void print() const {
		printf("%2d x,y:%2d,%2d, h:%d", m_id, m_position.m_x, m_position.m_y, m_health);
	}
};
enum class ACTION_TYPE {
	EMPTY = 0,
	FASTER = 1,
	SLOWER = 2,
	PORT = 3,
	STARBOARD = 4,
	FIRE = 5,
	MINE = 6
};
class Ship : public Entity {
public:
	Bot* m_owner;
	int m_orientation;
	int m_newOrientation;
	int m_speed;
	int m_health;
	int m_initialHealth;
	int m_mineCooldown;
	int m_cannonCooldown;
	Coord m_newPosition;
	Coord m_newBowCoordinate;
	Coord m_newSternCoordinate;

	Ship(size_t id, Bot* owner, int x, int y, int orientation) : 
		Entity(ENTITY_TYPE::SHIP, id, x, y),
		m_owner(owner),
		m_orientation(orientation),
		m_newOrientation(orientation),
		m_speed(0),
		m_health(INITIAL_SHIP_HEALTH), 
		m_initialHealth(INITIAL_SHIP_HEALTH),
		m_mineCooldown(0),
		m_cannonCooldown(0),
		m_newPosition(m_position),
		m_newBowCoordinate(bow()),
		m_newSternCoordinate(stern())
	{};

	inline string toBotString(int bot) const {
		return Entity::toBotString(m_orientation, m_speed, m_health, m_owner->m_id == bot ? 1 : 0);
	}
	inline void print() const {
		printf("%2d o:%d, x,y:%2d,%2d, o,s,h:%d,%d,%d", m_id, m_owner->m_id, m_position.m_x, m_position.m_y, m_orientation, m_speed, m_health);
	}

	inline Coord stern() const { // back part 
		return m_position.neighbor((m_orientation + 3) % 6);
	}
	inline Coord bow() const { // forward part 
		return m_position.neighbor(m_orientation);
	}
	inline Coord newStern() const {
		return m_position.neighbor((m_newOrientation + 3) % 6);
	}
	inline Coord newBow() const {
		return m_position.neighbor(m_newOrientation);
	}

	bool at(Coord coord) const {
		return (stern() == coord || bow() == coord || m_position == coord);
	}
	bool newBowIntersect(const Ship& other) const {
		return (m_newBowCoordinate == other.m_newBowCoordinate || m_newBowCoordinate == other.m_newPosition || m_newBowCoordinate == other.m_newSternCoordinate);
	}
	bool newBowIntersect(const vector<Ship>& ships) const {
		for (Ship other : ships) {
			if (*this != other && newBowIntersect(other)) {
				return true;
			}
		}
		return false;
	}
	bool newPositionsIntersect(const Ship& other) const {
		bool sternCollision = (m_newSternCoordinate == other.m_newBowCoordinate || m_newSternCoordinate == other.m_newPosition || m_newSternCoordinate == other.m_newSternCoordinate);
		bool centerCollision = (m_newPosition == other.m_newBowCoordinate || m_newPosition == other.m_newPosition || m_newPosition == other.m_newSternCoordinate);
		return newBowIntersect(other) || sternCollision || centerCollision;
	}
	bool newPositionsIntersect(const vector<Ship>& ships) const {
		for (const Ship& other : ships) {
			if (*this != other && newPositionsIntersect(other)) {
				return true;
			}
		}
		return false;
	}

	void damage(int health) {
		m_health -= health;
		if (m_health <= 0) {
			m_health = 0;
		}
	}
	void heal(int health) {
		m_health += health;
		if (m_health > MAX_SHIP_HEALTH) {
			m_health = MAX_SHIP_HEALTH;
		}
	}

	bool operator==(const Ship& o) const {
		return (o.m_id == m_id);
	}
	bool operator!=(const Ship& o) const {
		return (o.m_id != m_id);
	}
};
class Mine : public Entity {
public:
	Mine(size_t id, int x, int y) : Entity(ENTITY_TYPE::MINE, id, x, y) {
	}

	inline string toBotString(int bot) const {
		return Entity::toBotString(0, 0, 0, 0);
	}
	inline void print() const {
		printf("%2d x,y:%2d,%2d", m_id, m_position.m_x, m_position.m_y);
	}

	bool explode(vector<Ship>& ships, bool force) {
		bool damage = false;
		Ship* victim = nullptr;

		for (Ship& ship : ships) {
			if (m_position == ship.bow() || m_position == ship.stern() || m_position == ship.m_position) {
				damage = true;
				ship.damage(MINE_DAMAGE);
				victim = &ship;
			}
		}

		if (force || victim != nullptr) {
			if (victim == nullptr) {
				damage = true;
			}
			for (Ship& ship : ships) {
				if (victim == nullptr || ship.m_id != victim->m_id) {
					Coord impactPosition(-1, -1);
					if (ship.stern().distanceTo(m_position) <= 1) {
						impactPosition = ship.stern();
					}
					if (ship.bow().distanceTo(m_position) <= 1) {
						impactPosition = ship.bow();
					}
					if (ship.m_position.distanceTo(m_position) <= 1) {
						impactPosition = ship.m_position;
					}

					if (!(impactPosition == Coord(-1, -1))) {
						ship.damage(NEAR_MINE_DAMAGE);
						damage = true;
					}
				}
			}
		}

		return damage;
	}
};
class Action {
public:
	ACTION_TYPE m_type;
	Ship* m_ship;
	Coord m_target;

	Action(ACTION_TYPE type, Ship* ship) : m_type(type), m_ship(ship), m_target(ship->m_position) {}; // WAIT, FASTER, SLOWER, PORT, STARBOARD, MINE
	Action(ACTION_TYPE type, Ship* ship, int x, int y) : m_type(type), m_ship(ship), m_target(Coord(x, y)) {}; // FIRE

	friend ostream& operator<<(ostream& os, const Action& a) {
		if (a.m_type == ACTION_TYPE::FIRE) {
			os << "FIRE s:" << a.m_ship->m_id << " to:" << setw(2) << (int)a.m_target.m_x << "," << setw(2) << (int)a.m_target.m_y;
		} else if (a.m_type == ACTION_TYPE::FASTER) {
			os << "FASTER s:" << a.m_ship->m_id;
		} else if (a.m_type == ACTION_TYPE::SLOWER) {
			os << "SLOWER s:" << a.m_ship->m_id;
		} else if (a.m_type == ACTION_TYPE::PORT) {
			os << "PORT s:" << a.m_ship->m_id;
		} else if (a.m_type == ACTION_TYPE::STARBOARD) {
			os << "STARBOARD s:" << a.m_ship->m_id;
		} else if (a.m_type == ACTION_TYPE::MINE) {
			os << "MINE s:" << a.m_ship->m_id;
		} else {
			os << "EMPTY s:" << a.m_ship->m_id;
		}
		return os;
	}
};
class CodersOfTheCaribbean : public Game {
private:
	size_t id;

	// game state
	size_t mineCount;
	vector<Cannonball> m_cannonballs;
	vector<Mine> m_mines;
	vector<RumBarrel> m_barrels;
	vector<Ship> m_ships;

	//vector<Damage> m_damage;
	//vector<Coord> m_cannonBallExplosions;

	inline bool shipsAlive(size_t owner) {
		return find_if(m_ships.begin(), m_ships.end(), [&](const Ship& s) {
			return s.m_owner->m_id == owner;
		}) != m_ships.end();
	}
	inline bool gameIsOver() {
		for (const Bot& b : m_bots) {
			if (!shipsAlive(b.m_id)) {
				return true;
			}
		}
		return m_barrels.size() == 0 && LEAGUE_LEVEL == 0;
	}
	inline int evaluateWinner() {
		int health[2] = { 0, 0 };
		for (const Ship& ship : m_ships) {
			health[ship.m_owner->m_id] = ship.m_health;
		}
		if (health[0] == health[1]) {
			return -1;
		} else if (health[0]>health[1]) {
			return 0;
		} else {
			return 1;
		}
	}

	void deserializeActions(const vector<string>& botActions, vector<Action>& actions);
	ACTION_TYPE CodersOfTheCaribbean::moveTo(const Ship& ship, int x, int y);
	void output(int turn);

	void moveCannonballs(vector<Coord>& cannonBallExplosions);
	void decrementRum();
	void updateInitialRum();
	bool cellIsFreeOfBarrels(const Coord& target);
	bool cellIsFreeOfMines(const Coord& target);
	bool cellIsFreeOfShips(const Coord& target);
	void applyActions(const vector<Action>& actions);
	void checkBarrelCollisions(Ship& ship);
	void checkMineCollisions();
	void checkCollisions();
	void moveShips();
	void rotateShips();
	void explodeShips(vector<Coord>& cannonBallExplosions);
	void explodeMines(vector<Coord>& cannonBallExplosions);
	void explodeBarrels(vector<Coord>& cannonBallExplosions);
	void updateGame(const vector<string>& botActions);

	string serializeInitBotInput(int bot) const;
	string serializeBotInput(int turn, int bot) const;

public:
	CodersOfTheCaribbean(const vector<string>& botNames, long long seed = system_clock::now().time_since_epoch().count(), size_t mineCount = 0, size_t barrelCount = 0, size_t shipsPerPlayer = 0);

	int run();

	~CodersOfTheCaribbean();
};






#endif
