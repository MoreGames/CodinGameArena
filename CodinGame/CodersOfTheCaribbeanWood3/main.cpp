//#pragma GCC optimize("-O3")
//#pragma GCC optimize("inline")
//#pragma GCC optimize("omit-frame-pointer")
//#pragma GCC optimize("unroll-loops")

#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <vector>
#include <chrono>
#include <cstring>

using namespace std;
using namespace std::chrono;

//#define PROFILE
//#define DEBUG
//#define DEBUGFACTORYDETAILS
//#define DEBUGINPUT
//#define DEBUGDIST
//#define DEBUGSIMENEMY
//#define DEBUGSIMMY
//#define DEBUGBESTBOMBTARGETS
//#define DEBUGOPTBOMBS
//#define DEBUGACTION
//#define STATS

high_resolution_clock::time_point start;
#define NOW high_resolution_clock::now()
#define TIME duration_cast<duration<double>>(NOW - start).count()


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

#define LEAGUE_LEVEL 0

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
constexpr int FIRST_TURN_TIMEOUT{ 1005 }; // in milliseconds
constexpr int TURN_TIMEOUT{ 55 }; // in milliseconds
constexpr int TURN_LIMIT{ 200 };

class Random {
private:
	long long m_seed;
	inline void setSeed(long long seed) {
		m_seed = (seed ^ 0x5DEECE66DL) & ((1LL << 48) - 1);
	}
	inline unsigned int next(unsigned int bits) {
		m_seed = (m_seed * 0x5DEECE66DL + 0xBL) & ((1LL << 48) - 1);
		return (unsigned int)(m_seed >> (48 - bits));
	}
public:
	Random(long long seed) {
		setSeed(seed);
	}

	long long getSeed() {
		return m_seed;
	}
	unsigned int nextInt() {
		return next(32);
	}
	unsigned int nextInt(unsigned int n) {
		if ((n & (n - 1)) == 0) // i.e., n is a power of 2
			return (unsigned int)((n * (long long)next(31)) >> 31);
		unsigned int bits, val;
		do {
			bits = next(31);
			val = bits % n;
		} while (bits - val + (n - 1) < 0);
		return val;
	}
	unsigned long long nextLong() {
		// it's okay that the bottom word remains signed.
		return ((unsigned long long)(next(32)) << 32) + next(32);
	}
};

constexpr int DIRECTIONS_EVEN[6][2] = { { 1, 0 },{ 0, -1 },{ -1, -1 },{ -1, 0 },{ -1, 1 },{ 0, 1 } };
constexpr int DIRECTIONS_ODD[6][2] = { { 1, 0 },{ 1, -1 },{ 0, -1 },{ -1, 0 },{ 0, 1 },{ 1, 1 } };
constexpr int CUBE_DIRECTIONS[6][3] = { { 1, -1, 0 },{ +1, 0, -1 },{ 0, +1, -1 },{ -1, +1, 0 },{ -1, 0, +1 },{ 0, -1, +1 } };
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

	inline string toPlayerString(int arg1, int arg2, int arg3, int arg4) const {
		return to_string(m_id) + " " + typeName() + " " + to_string(arg1) + " " + to_string(arg2) + " " + to_string(arg3) + " " + to_string(arg4) + "\n";
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
	int m_owner;
	int m_ownerEntityId;
	int m_srcX;
	int m_srcY;
	int m_initialRemainingTurns;
	int m_remainingTurns;

	Cannonball(size_t id, int owner, int x, int y, int ownerEntityId, int srcX, int srcY, int remainingTurns) :
		Entity(ENTITY_TYPE::CANNONBALL, id, x, y),
		m_owner(owner),
		m_ownerEntityId(ownerEntityId),
		m_srcX(srcX),
		m_srcY(srcY),
		m_initialRemainingTurns(remainingTurns),
		m_remainingTurns(remainingTurns)
	{};

	string toPlayerString(int playerIdx) {
		return Entity::toPlayerString(m_ownerEntityId, m_remainingTurns, 0, 0);
	}
};
class RumBarrel : public Entity {
public:
	int m_health;

	RumBarrel(size_t id, int x, int y, int health) : Entity(ENTITY_TYPE::BARREL, id, x, y), m_health(health) {};
};

class GameState {
public:
	vector<RumBarrel> m_barrels;
	vector<Coord> m_ships;
};

int main() {
	Random random = Random(123456789);

	unsigned short turn = 0;
	// game loop
	while (1) {
		GameState base;
		int myShipCount; // the number of remaining ships
		cin >> myShipCount; cin.ignore();
		int entityCount; // the number of entities (e.g. ships, mines or cannonballs)
		cin >> entityCount; cin.ignore();
		for (int i = 0; i < entityCount; i++) {
			int entityId;
			string entityType;
			int x;
			int y;
			int arg1;
			int arg2;
			int arg3;
			int arg4;
			cin >> entityId >> entityType >> x >> y >> arg1 >> arg2 >> arg3 >> arg4; cin.ignore();

			if (entityType.compare("SHIP") == 0 && arg4 == 1) {
				base.m_ships.push_back(Coord(x, y));
				//fprintf(stderr, "MYSHIP %d: X=%d, Y=%d, DIR=%d, SPD=%d, RUM=%d, OWN=%d\n", i, x, y, arg1, arg2, arg3, arg4);
			} else if (entityType.compare("SHIP") == 0 && arg4 == 0) {
				//fprintf(stderr, "ENEMYSHIP %d: X=%d, Y=%d, DIR=%d, SPD=%d, RUM=%d, OWN=%d\n", i, x, y, arg1, arg2, arg3, arg4);
			} else if (entityType.compare("BARREL") == 0) {
				base.m_barrels.push_back(RumBarrel(entityId, x, y, arg1));
				//fprintf(stderr, "BARREL %d: X=%d, Y=%d, STOCK=%d\n", i, x, y, arg1);
			} else if (entityType.compare("MINE") == 0) {
				//fprintf(stderr, "MINE %d: X=%d, Y=%d\n", i, x, y);
			} else if (entityType.compare("CANNONBALL") == 0) {
				//fprintf(stderr, "CANNONBALL %d: X=%d, Y=%d, FROMID=%d, IMPACT=%d\n", i, x, y, arg1, arg2);
			}
		}

		//for (const RumBarrel& barrel : base.m_barrels) {
		//	fprintf(stderr, "BARREL %d: X=%d, Y=%d, STOCK=%d\n", barrel.m_id, barrel.m_position.m_x, barrel.m_position.m_y, barrel.m_health);
		//}

		for (int i = 0; i < myShipCount; i++) {
			const Coord& ship = base.m_ships[i];
			sort(base.m_barrels.begin(), base.m_barrels.end(), [&ship](const RumBarrel& a, const RumBarrel& b) -> bool {
				if (a.m_position.distanceTo(ship) == b.m_position.distanceTo(ship)) {
					return a.m_health > b.m_health;
				} else {
					return a.m_position.distanceTo(ship) < b.m_position.distanceTo(ship);
				}
			});

			// Write an action using cout. DON'T FORGET THE "<< endl"
			// To debug: cerr << "Debug messages..." << endl;
			if (base.m_barrels.size() > 0) {
				cout << "MOVE " << base.m_barrels[0].m_position.m_x << " " << base.m_barrels[0].m_position.m_y << endl;
			} else {
				int x = 1 + random.nextInt(MAP_WIDTH - 2);
				int y = 1 + random.nextInt(MAP_HEIGHT / 2);
				cout << "MOVE " << x << " " << y << endl;
			}

			//cout << "MOVE 11 10" << endl; // Any valid action, such as "WAIT" or "MOVE x y"
		}
	}
}
