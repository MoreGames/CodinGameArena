#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include <algorithm>
#include <vector>

using namespace std;
using namespace std::chrono;

//#define DEBUG

//////////////////////////////////

constexpr int FACTORYSTORAGE = 15;
constexpr int TROOPSTORAGE = 2000;
constexpr int BOMBSTORAGE = 4;
constexpr int FACTORY_TROOP_STORAGE = 200;
constexpr int ACTIONSTORAGE = 200;

constexpr int DAMAGE_DURATION = 5;

//////////////////////////////////

unsigned char factoryDist[FACTORYSTORAGE][FACTORYSTORAGE] = { 0 };
unsigned char factoryMaxDist[FACTORYSTORAGE] = { 0 };
float factoryProdToDist[FACTORYSTORAGE][FACTORYSTORAGE] = { 0 };

int turn = 0;
int myScore = 0;
int myUsedBombs = 0;
int myAvailableBombs = 2;
int hisScore = 0;
int hisUsedBombs = 0;
int hisAvailableBombs = 2;

class Factory;
class Troop;
class Bomb;

//////////////////////////////////

class Entity {
public:
	char m_id;
	char m_owner;

	Entity() : m_id(-1), m_owner(0) {};
	Entity(char id, char owner) : m_id(id), m_owner(owner) {};
};

//////////////////////////////////

class Factory : public Entity {
public:
	unsigned short m_cyborgs;
	unsigned char m_production;
	unsigned char m_turns; // 0 producing, otherwise turns until producing again
	short m_cyborgsAvailable[21] = { 0 }; // neg enemy, pos own, not correct for neutral Factories

	Factory() : Entity(), m_production(0), m_turns(0) {};
	Factory(char id, char owner, unsigned short cyborgs, unsigned char production, unsigned char turns) : Entity(id, owner), m_cyborgs(cyborgs), m_production(production), m_turns(turns) {};
	void update();

	unsigned char getCurrentProduction() {
		return (!m_turns) ? m_production : 0;
	}

	friend std::ostream& operator<<(std::ostream& os, const Factory& f) {
		os << std::setw(2) << (int)f.m_id << " o:" << std::setw(2) << (int)f.m_owner << " c:" << std::setw(3) << (int)f.m_cyborgs << " p:" << (int)f.m_production << " t:" << (int)f.m_turns;
		os << " " << (int)f.m_cyborgsAvailable[0];
		for (size_t i = 1; i < factoryMaxDist[f.m_id] + (size_t)1; i++) {
			os << "|" << (int)f.m_cyborgsAvailable[i];
		}
		return os;
	}
};

Factory factories[FACTORYSTORAGE];
size_t factoryCount = 0;

// from i -> j means element [i][j]
unsigned char factoryMinDist[FACTORYSTORAGE][FACTORYSTORAGE] = { 0 };
Factory* factoryMinDistNextStep[FACTORYSTORAGE][FACTORYSTORAGE] = { 0 };

void FloydWarshallWithPathReconstruction() {
	for (size_t i = 0; i < factoryCount; i++) {
		for (size_t j = 0; j < factoryCount; j++) {
			factoryMinDist[i][j] = factoryDist[i][j];
			factoryMinDistNextStep[i][j] = &factories[j];
		}
	}

	for (size_t k = 0; k < factoryCount; k++) {
		for (size_t i = 0; i < factoryCount; i++) {
			for (size_t j = 0; j < factoryCount; j++) {
				if (factoryMinDist[i][j] > factoryMinDist[i][k] + factoryMinDist[k][j]) {
					factoryMinDist[i][j] = factoryMinDist[i][k] + factoryMinDist[k][j];
					factoryMinDistNextStep[i][j] = factoryMinDistNextStep[i][k];
				}
			}
		}
	}
}

//////////////////////////////////

class MovingEntity {
public:
	char m_id;
	char m_owner;
	Factory* m_source;
	Factory* m_target;
	unsigned char m_remainingDistance;

	MovingEntity() : m_id(-1), m_owner(0), m_source(nullptr), m_target(nullptr), m_remainingDistance(0) {};
	MovingEntity(char id, char owner, Factory* source, Factory* target, unsigned char dist) : m_id(id), m_owner(owner), m_source(source), m_target(target), m_remainingDistance(dist) {};

	void move() {
		m_remainingDistance--;
	}
};

//////////////////////////////////

class Troop : public MovingEntity {
public:
	unsigned short m_cyborgs;

	Troop() : MovingEntity(), m_cyborgs(0) {};
	Troop(char id, char owner, Factory* source, Factory* target, unsigned char dist, unsigned short cyborgs) : MovingEntity(id, owner, source, target, dist), m_cyborgs(cyborgs) {};

	friend std::ostream& operator<<(std::ostream& os, const Troop& t) {
		os << std::setw(2) << (int)t.m_id << " o:" << std::setw(2) << (int)t.m_owner << " c:" << std::setw(3) << (int)t.m_cyborgs << " " << std::setw(2) << (int)t.m_source->m_id << "->" << std::setw(2) << (int)t.m_target->m_id << ":" << std::setw(2) << (int)t.m_remainingDistance;
		return os;
	}
};

Troop troops[TROOPSTORAGE];
size_t troopCount = 0;

//////////////////////////////////

class Bomb : public MovingEntity {
public:
	bool active;
	Factory* m_targets[FACTORYSTORAGE];
	size_t m_targetCount;


	Bomb() : MovingEntity(), m_targetCount(0) {};
	Bomb(char id, char owner, Factory* source, Factory* target, unsigned char dist) : MovingEntity(id, owner, source, target, dist), m_targetCount(0) {
		if (owner == -1) {
			for (size_t i = 0; i < factoryCount; i++) {
				if (factories[i].m_owner == 0 || factories[i].m_owner == 1) {
					m_targets[m_targetCount++] = &factories[i];
				}
			}
		}
	};
	void update() {
		active = true;
		if (m_owner == -1) {
			m_remainingDistance += 1;

			Factory* temp[FACTORYSTORAGE] = { 0 };
			size_t tempCount = 0;
			for (size_t i = 0; i < m_targetCount; i++) {
				if (factoryDist[m_source->m_id][m_targets[i]->m_id] > m_remainingDistance) {
					temp[tempCount++] = m_targets[i];
				}
			}

			m_targetCount = tempCount;
			for (size_t i = 0; i < m_targetCount; i++) {
				m_targets[i] = temp[i];
			}
		} else {
			m_remainingDistance--;
		}
	}

	void explode() {
		unsigned int damage = (std::min)((int)m_target->m_cyborgs, (std::max)(10, (int)m_target->m_cyborgs / 2));
		m_target->m_cyborgs -= damage;
		m_target->m_turns = DAMAGE_DURATION;
	}

	friend std::ostream& operator<<(std::ostream& os, const Bomb& b) {
		os << std::setw(2) << (int)b.m_id << " o:" << std::setw(2) << (int)b.m_owner << " " << std::setw(2) << (int)b.m_source->m_id << "->";
		if (b.m_owner == -1) {
			cerr << " ?";
		} else {
			cerr << std::setw(2) << (int)b.m_target->m_id;
		}
		cerr << ":" << std::setw(2) << (int)b.m_remainingDistance;
		if (b.m_owner == -1) {
			cerr << " " << (int)b.m_targets[0]->m_id;;
			for (size_t i = 1; i < b.m_targetCount; i++) {
				cerr << "|" << (int)b.m_targets[i]->m_id;
			}
		}
		return os;
	}
};

Bomb bombs[BOMBSTORAGE];
size_t bombCount = 0;

// TODO
// bei Bomben könnte man sich ausrechnen welche möglichen Factories wann getroffen werden
// unter der Voraussetzung dass der Gegner nur eigene Factories ansteuert
// 
// Unter der Annahme, dass sich in den einzelnen Factories nur die gerade produzierten und ev. von anderen Factories eingeflogenen Troops (ca. deren Produktion)
// befinden, d.h. eher wenig Cyborgs befinden), wird die durchschnittliche Effektivität einer Bombe wahrscheinlich bei 1-3 liegen. Von daher wäre es vielleicht besser
// auf neutrale Factories die Bomben zu schicken (falls dort mehr Cyborgs sind)
//class Bomb1 : public MovingEntity {
//public:
//	Factory* possibleTargets[FACTORYSTORAGE]; // only relevant for enemy bombs, dont update enemy bombs
//	size_t possibleTargetCount;
//
//	Bomb1() : MovingEntity() {};
//	Bomb1(char id, char owner, Factory* source, Factory* target, unsigned char dist) : MovingEntity(id, owner, source, target, dist) {
//		for (size_t i = 0; i < myFactoryCount; i++) {
//			Factory* f = myFactories[i];
//			possibleTargets[possibleTargetCount++] = f;
//		}
//	};
//	void update() {
//		m_remainingDistance--;
//	}
//
//	void explode() {
//		unsigned int damage = (std::min)((int)m_target->m_cyborgs, (std::max)(10, (int)m_target->m_cyborgs / 2));
//		m_target->m_cyborgs -= damage;
//		m_target->m_turns = DAMAGE_DURATION;
//	}
//	//vector<Factory*> getNextPossibleTargets() {
//
//	//}
//
//	friend std::ostream& operator<<(std::ostream& os, const Bomb1& b) {
//		os << std::setw(2) << (int)b.m_id << " o:" << std::setw(2) << (int)b.m_owner << " " << std::setw(2) << (int)b.m_source << "->" << std::setw(2) << (int)b.m_target << ":" << std::setw(2) << (int)b.m_remainingDistance;
//		return os;
//	}
//};

//////////////////////////////////

class Action {
public:
	// WAIT ... source==nullptr
	// INC ... target==nullptr
	// BOMB ... cyborgs==0
	// MOVE ... all fields filled
	Factory* m_source;
	Factory* m_target;
	unsigned short m_cyborgs;

	Action() : m_source(nullptr), m_target(nullptr), m_cyborgs(0) {};
	Action(Factory* source, Factory* target, unsigned short cyborgs) : m_source(source), m_target(target), m_cyborgs(cyborgs) {};

	string output() {
		if (m_source == nullptr) {
			return "WAIT";
		} else if (m_target == nullptr) {
			return "INC " + std::to_string(m_source->m_id);
		} else if (m_cyborgs == 0) {
			return "BOMB " + std::to_string(m_source->m_id) + " " + std::to_string(m_target->m_id);
		} else {
			return "MOVE " + std::to_string(m_source->m_id) + " " + std::to_string(m_target->m_id) + " " + std::to_string(m_cyborgs);
		}
	}

	friend std::ostream& operator<<(std::ostream& os, const Action& a) {
		if (a.m_source == nullptr) {
			os << "s: - t: - c: -";
		} else if (a.m_target == nullptr) {
			os << "s:" << std::setw(2) << (int)a.m_source->m_id << " t: - c: -";
		} else if (a.m_cyborgs == 0) {
			os << "s:" << std::setw(2) << (int)a.m_source->m_id << " t:" << std::setw(2) << (int)a.m_target->m_id << " c: -";
		} else {
			os << "s:" << std::setw(2) << (int)a.m_source->m_id << " t:" << std::setw(2) << (int)a.m_target->m_id << " c:" << std::setw(2) << (int)a.m_cyborgs;
		}
		return os;
	}
};

// group actions with same source and target
string output(vector<Action>& actions) {
	sort(actions.begin(), actions.end(), [](const Action& a, const Action& b) {
		if (a.m_source->m_id == b.m_source->m_id) {
			if (a.m_target->m_id == b.m_target->m_id) {
				return a.m_cyborgs > b.m_cyborgs;
			} else {
				return a.m_target->m_id > b.m_target->m_id;
			}
		} else {
			return a.m_source->m_id > b.m_source->m_id;
		}
	});

#ifdef DEBUG
	cerr << "Actions:" << endl;
	for (size_t i = 0; i < actions.size(); i++) {
		cerr << actions[i] << endl;
	}
#endif

	vector<Action> temp(0);
	temp.push_back(actions[0]);
	for (size_t i = 1; i < actions.size(); i++) {
		if (actions[i].m_source->m_id == actions[i - 1].m_source->m_id && actions[i].m_target->m_id == actions[i - 1].m_target->m_id) {
			temp[temp.size() - 1].m_cyborgs += actions[i].m_cyborgs;
		} else {
			temp.push_back(actions[i]);
		}
	}

#ifdef DEBUG
	cerr << "Action groups:" << endl;
	for (size_t i = 0; i < temp.size(); i++) {
		cerr << temp[i] << endl;
	}
#endif

	string ret = "";
	for (size_t i = 0; i < temp.size() - 1; i++) {
		ret += temp[i].output() + ";";
	}
	ret += temp[temp.size() - 1].output();

	return ret;
}

//////////////////////////////////

//class Solution {
//public:
//	Action actions[DEPTH][ACTIONSTORAGE];
//	size_t actionsSize[DEPTH];
//
//	Solution() {
//		for (size_t i = 0; i < DEPTH; i++) {
//			actionsSize[i] = 1;
//			if (myFactoryCount != 1) {
//
//			} else {
//				int type = fastRandInt(2);
//				if (type) { // 1
//					Factory* target = enemyFactories[fastRandInt(enemyFactoryCount)];
//					int cyborgs = fastRandInt(myFactories[0]->m_cyborgs) + 1;
//					actions[i][0] = Action(myFactories[0], target, cyborgs);
//				} else { // 0
//					actions[i][0] = Action();
//				}
//			}
//		}
//	}
//};

//////////////////////////////////

Factory* myFactories[FACTORYSTORAGE];
size_t myFactoryCount = 0;
Factory* enemyFactories[FACTORYSTORAGE];
size_t enemyFactoryCount = 0;

Troop* myTroops[TROOPSTORAGE];
size_t myTroopCount = 0;
Troop* enemyTroops[TROOPSTORAGE];
size_t enemyTroopCount = 0;

Bomb* myBombs[2];
size_t myBombCount = 0;
Bomb* enemyBombs[2];
size_t enemyBombCount = 0;

//////////////////////////////////

// TODO test and check
void Factory::update() {
	short help[4][21] = { 0 }; // own, enemy, prod, owner

	for (size_t i = 0; i < troopCount; i++) {
		if (troops[i].m_target->m_id == m_id) {
			if (troops[i].m_owner == 1) {
				help[0][troops[i].m_remainingDistance] += troops[i].m_cyborgs;
			} else {
				help[1][troops[i].m_remainingDistance] -= troops[i].m_cyborgs;
			}
		}
	}

	if (m_owner != 1) {
		help[2][0] = -m_cyborgs;
	} else {
		help[2][0] = m_cyborgs;
	}
	m_cyborgsAvailable[0] = help[2][0];
	char owner = m_owner;
	help[3][0] = owner;
	for (size_t i = 1; i < 21; i++) {
		// battle incoming troops
		short t = help[0][i] + help[1][i];

		if (owner == 0) {
			if (t < 0) {
				m_cyborgsAvailable[i] = m_cyborgsAvailable[i - 1] - t;
			} else {
				m_cyborgsAvailable[i] = m_cyborgsAvailable[i - 1] + t;
			}

			// set owner
			if (m_cyborgsAvailable[i] > 0 && m_cyborgsAvailable[i - 1] <= 0) {
				if (t < 0) {
					owner = -1;
					m_cyborgsAvailable[i] = -m_cyborgsAvailable[i];
				} else {
					owner = 1;
				}
			}
		} else {
			if (i > m_turns) {
				help[2][i] = owner*m_production;
			}

			m_cyborgsAvailable[i] = m_cyborgsAvailable[i - 1] + t + help[2][i];

			if ((m_cyborgsAvailable[i] > 0 && m_cyborgsAvailable[i - 1] <= 0) || (m_cyborgsAvailable[i] < 0 && m_cyborgsAvailable[i - 1] >= 0)) {
				if (m_cyborgsAvailable[i] < 0) {
					owner = -1;
				} else {
					owner = 1;
				}
			}
		}

		help[3][i] = owner;
	}

#ifdef DEBUG
	// neutral and enemy cyborgs are negative
	// neutral cyborgs are mask with a star
	cerr << "Factory: " << (int)m_id << endl;
	cerr << "  |  O|  E| P =  S" << endl;
	cerr << " 0|   |   |" << std::setw(2) << help[2][0] << " =" << std::setw(3) << help[2][0];
	if (help[3][0] == 0) {
		cerr << "*";
	}
	cerr << endl;
	for (size_t i = 1; i < factoryMaxDist[m_id] + (size_t)1; i++) {
		cerr << std::setw(2) << i << "|" << std::setw(3) << help[0][i] << "|" << std::setw(3) << help[1][i] << "|" << std::setw(2) << help[2][i] << " =" << std::setw(3) << m_cyborgsAvailable[i];
		if (help[3][i] == 0) {
			cerr << "*";
		}
		cerr << endl;
	}
#endif
}

//////////////////////////////////

void reset() {
	for (size_t i = 0; i < troopCount; i++) {
		troops[i] = Troop();
	}
	troopCount = 0;

	for (size_t i = 0; i < myFactoryCount; i++) {
		myFactories[i] = nullptr;
	}
	myFactoryCount = 0;
	for (size_t i = 0; i < enemyFactoryCount; i++) {
		enemyFactories[i] = nullptr;
	}
	enemyFactoryCount = 0;

	for (size_t i = 0; i < myTroopCount; i++) {
		myTroops[i] = nullptr;
	}
	myTroopCount = 0;
	for (size_t i = 0; i < enemyTroopCount; i++) {
		enemyTroops[i] = nullptr;
	}
	enemyTroopCount = 0;

	for (size_t i = 0; i < myBombCount; i++) {
		myBombs[i] = nullptr;
	}
	myBombCount = 0;
	for (size_t i = 0; i < enemyBombCount; i++) {
		enemyBombs[i] = nullptr;
	}
	enemyBombCount = 0;
}

short alreadyEnoughTroopsUnderway(Factory* factory, const vector<Action>& actions) {

	unsigned char d = factoryMaxDist[factory->m_id];
	short cyborgs = factory->m_cyborgsAvailable[d];

	for (const auto& a : actions) {
		if (a.m_target->m_id == factory->m_id) {
			cyborgs += a.m_cyborgs;
		}
	}

	return cyborgs;
}

vector<Action> heuristicWood2_1() {
	vector<Action> actions;

	// take the factory with the highest cyborg count
	Factory* bestFactory = nullptr;
	for (size_t i = 0; i < myFactoryCount; i++) {
		if (bestFactory == nullptr || bestFactory->m_cyborgs < myFactories[i]->m_cyborgs) {
			bestFactory = myFactories[i];
		}
	}

	// check the Troops towards each Factory (own pos, enemy neg)
	short troopsTowardsFactory[FACTORYSTORAGE] = { 0 };
	for (size_t i = 0; i < troopCount; i++) {
		troopsTowardsFactory[troops[i].m_target->m_id] += troops[i].m_owner * troops[i].m_cyborgs;
	}

#ifdef DEBUG
	cerr << "troopsTowardsFactory:" << endl;
	for (size_t i = 0; i < factoryCount; i++) {
		cerr << troopsTowardsFactory[i] << " | ";
	}
	cerr << endl;
#endif

	// take the max ProdToDist value which is conquerable (including production over time)
	Factory* bestTarget = bestFactory;
	unsigned short bestCyborgs = 0;
	for (size_t i = 0; i < enemyFactoryCount; i++) {
		Factory* factory = enemyFactories[i];
		unsigned char target = factory->m_id;
		unsigned char dist = factoryDist[bestFactory->m_id][target];
		short cyborgs = factory->m_cyborgs - troopsTowardsFactory[target];
		if (factory->m_owner != 0) {
			cyborgs += factory->m_production*dist;
		}
		if (cyborgs >= 0 && cyborgs < bestFactory->m_cyborgs) { // can be captured
			if (factoryProdToDist[bestFactory->m_id][bestTarget->m_id] < factoryProdToDist[bestFactory->m_id][target]) { // better value
				bestCyborgs = (unsigned short)cyborgs + 1;
				bestTarget = factory;
			}
		}
	}

	if (bestTarget->m_id != bestFactory->m_id) { // target found
		actions.push_back(Action(bestFactory, bestTarget, bestCyborgs));
	} else { // nothing found
		actions.push_back(Action());
	}

	return actions;
}

//////////////////////////////////

int main() {
	for (size_t i = 0; i < FACTORYSTORAGE; i++) {
		for (size_t j = 0; j < FACTORYSTORAGE; j++) {
			factoryProdToDist[i][j] = -1;
		}
	}

	cin >> factoryCount; cin.ignore();
	int linkCount;
	cin >> linkCount; cin.ignore();
	for (int i = 0; i < linkCount; i++) {
		int f1, f2, d;
		cin >> f1 >> f2 >> d; cin.ignore();
		factoryDist[f1][f2] = factoryDist[f2][f1] = d;
	}

#ifdef DEBUG
	cerr << "factoryDist:" << endl;
	cerr << "  ";
	for (size_t i = 0; i < factoryCount; i++) {
		cerr << "|" << std::setw(2) << i;
	}
	cerr << "|" << endl;
	cerr << "--";
	for (size_t i = 0; i < factoryCount; i++) {
		cerr << "---";
	}
	cerr << "-" << endl;
	for (size_t i = 0; i < factoryCount; i++) {
		cerr << std::setw(2) << i;
		for (size_t j = 0; j < factoryCount; j++) {
			cerr << "|" << std::setw(2) << (int)factoryDist[i][j];
		}
		cerr << "|" << endl;

		cerr << "--";
		for (size_t j = 0; j < factoryCount; j++) {
			cerr << "---";
		}
		cerr << "-" << endl;
	}
#endif

	for (size_t i = 0; i < FACTORYSTORAGE; i++) {
		factories[i] = Factory();
	}
	for (size_t i = 0; i < TROOPSTORAGE; i++) {
		troops[i] = Troop();
	}

	while (1) {

		// reset bombs
		for (size_t i = 0; i < bombCount; i++) {
			bombs[i].active = false;
		}

		int entityCount;
		cin >> entityCount; cin.ignore();
		for (int i = 0; i < entityCount; i++) {
			int id, arg1, arg2, arg3, arg4, arg5;
			string type;
			cin >> id >> type >> arg1 >> arg2 >> arg3 >> arg4 >> arg5; cin.ignore();
			//cerr << id << " " << type << " " << arg1 << " " << arg2 << " " << arg3 << " " << arg4 << " " << arg5 << endl;
			if (type.compare("FACTORY") == 0) {
				factories[id] = Factory(id, arg1, arg2, arg3, arg4);
			} else if (type.compare("TROOP") == 0) {
				troops[troopCount++] = Troop(id, arg1, &factories[arg2], &factories[arg3], arg5, arg4);
			} else {
				bool found = false;
				size_t j = 0;
				for (; j < bombCount; j++) {
					if (id == bombs[j].m_id) {
						found = true;
						break;
					}
				}

				if (found) {
					bombs[j].update();
				} else {
					if (arg1 == -1) {
						bombs[bombCount++] = Bomb(id, arg1, &factories[arg2], nullptr, 0);
					} else {
						bombs[bombCount++] = Bomb(id, arg1, &factories[arg2], &factories[arg3], arg4);
					}

				}
			}
		}

		// update factories with infos about troops
		for (size_t i = 0; i < factoryCount; i++) {
			factories[i].update();
		}

		// remove exploded bombs
		{
			Bomb tempBombs[BOMBSTORAGE];
			size_t tempBombCount = 0;

			for (size_t i = 0; i < bombCount; i++) {
				if (bombs[i].active) {
					tempBombs[tempBombCount++] = bombs[i];
				}
			}

			bombCount = tempBombCount;
			for (size_t i = 0; i < bombCount; i++) {
				bombs[i] = tempBombs[i];
			}
			for (size_t i = bombCount; i < BOMBSTORAGE; i++) {
				bombs[i] = Bomb();
			}
		}

		if (turn == 0) {
			FloydWarshallWithPathReconstruction();

			// max dist
			for (size_t i = 0; i < factoryCount; i++) {
				factoryMaxDist[i] = factoryMinDist[i][0];
				for (size_t j = 1; j < factoryCount; j++) {
					if (factoryMaxDist[i] < factoryMinDist[i][j]) {
						factoryMaxDist[i] = factoryMinDist[i][j];
					}
				}
			}

#ifdef DEBUG
			//			cerr << "FloydWarshall:" << endl;
			//			cerr << "Next:" << endl;
			//			cerr << "  ";
			//			for (size_t i = 0; i < factoryCount; i++) {
			//				cerr << "|" << std::setw(2) << i;
			//			}
			//			cerr << "|" << endl;
			//			cerr << "--";
			//			for (size_t i = 0; i < factoryCount; i++) {
			//				cerr << "---";
			//			}
			//			cerr << "-" << endl;
			//			for (size_t i = 0; i < factoryCount; i++) {
			//				cerr << std::setw(2) << i;
			//				for (size_t j = 0; j < factoryCount; j++) {
			//					cerr << "|" << std::setw(2) << (int)factoryMinDistNextStep[i][j]->m_id;
			//				}
			//				cerr << "|" << endl;
			//
			//				cerr << "--";
			//				for (size_t j = 0; j < factoryCount; j++) {
			//					cerr << "---";
			//				}
			//				cerr << "-" << endl;
			//			}
			//			cerr << "MinDist:" << endl;
			//			cerr << "  ";
			//			for (size_t i = 0; i < factoryCount; i++) {
			//				cerr << "|" << std::setw(2) << i;
			//			}
			//			cerr << "|" << endl;
			//			cerr << "--";
			//			for (size_t i = 0; i < factoryCount; i++) {
			//				cerr << "---";
			//			}
			//			cerr << "-" << endl;
			//			for (size_t i = 0; i < factoryCount; i++) {
			//				cerr << std::setw(2) << i;
			//				for (size_t j = 0; j < factoryCount; j++) {
			//					cerr << "|" << std::setw(2) << (int)factoryMinDist[i][j];
			//				}
			//				cerr << "|" << endl;
			//
			//				cerr << "--";
			//				for (size_t j = 0; j < factoryCount; j++) {
			//					cerr << "---";
			//				}
			//				cerr << "-" << endl;
			//			}
			cerr << "factoryMaxDist:" << endl;
			for (size_t i = 0; i < factoryCount; i++) {
				cerr << std::setw(2) << i << " ";
			}
			cerr << endl;
			for (size_t i = 0; i < factoryCount; i++) {
				cerr << "---";
			}
			cerr << endl;
			for (size_t i = 0; i < factoryCount; i++) {
				cerr << std::setw(2) << (int)factoryMaxDist[i] << " ";
			}
			cerr << endl;

#endif
		}

		// update
		for (size_t i = 0; i < factoryCount; i++) {
			for (size_t j = 0; j < factoryCount; j++) {
				if (i == j) continue;
				factoryProdToDist[i][j] = (float)factories[j].m_production / (float)factoryMinDist[i][j];
			}
		}

		//#ifdef DEBUG
		//		cerr << "factoryProdToDist:" << endl;
		//		cerr << "  ";
		//		for (size_t i = 0; i < factoryCount; i++) {
		//			cerr << "|" << std::setw(5) << i;
		//		}
		//		cerr << "|" << endl;
		//		cerr << "--";
		//		for (size_t i = 0; i < factoryCount; i++) {
		//			cerr << "------";
		//		}
		//		cerr << "-" << endl;
		//		for (size_t i = 0; i < factoryCount; i++) {
		//			cerr << std::setw(2) << i;
		//			for (size_t j = 0; j < factoryCount; j++) {
		//				cerr << "|" << std::setw(5) << std::fixed << std::setprecision(2) << factoryProdToDist[i][j];
		//			}
		//			cerr << "|" << endl;
		//
		//			cerr << "--";
		//			for (size_t j = 0; j < factoryCount; j++) {
		//				cerr << "------";
		//			}
		//			cerr << "-" << endl;
		//		}
		//		cerr << "Factories:" << endl;
		//		for (size_t i = 0; i < factoryCount; i++) {
		//			cerr << factories[i] << endl;
		//		}
		//		cerr << "Troops:" << endl;
		//		for (size_t i = 0; i < troopCount; i++) {
		//			cerr << troops[i] << endl;
		//		}
		//		cerr << "Bombs:" << endl;
		//		for (size_t i = 0; i < bombCount; i++) {
		//			cerr << bombs[i] << endl;
		//		}
		//#endif

		for (size_t i = 0; i < factoryCount; i++) {
			if (factories[i].m_owner == 1) {
				myFactories[myFactoryCount++] = &factories[i];
			} else {
				enemyFactories[enemyFactoryCount++] = &factories[i];
			}
		}
		for (size_t i = 0; i < troopCount; i++) {
			if (troops[i].m_owner == 1) {
				myTroops[myTroopCount++] = &troops[i];
			} else {
				enemyTroops[enemyTroopCount++] = &troops[i];
			}
		}
		for (size_t i = 0; i < bombCount; i++) {
			if (bombs[i].m_owner == 1) {
				myBombs[myBombCount++] = &bombs[i];
			} else {
				enemyBombs[enemyBombCount++] = &bombs[i];
			}
		}

#ifdef DEBUG
		cerr << "My factories:" << endl;
		for (size_t i = 0; i < myFactoryCount; i++) {
			cerr << *(myFactories[i]) << endl;
		}
		cerr << "Enemy factories:" << endl;
		for (size_t i = 0; i < enemyFactoryCount; i++) {
			cerr << *(enemyFactories[i]) << endl;
		}
		cerr << "My troops:" << endl;
		for (size_t i = 0; i < myTroopCount; i++) {
			cerr << *(myTroops[i]) << endl;
		}
		cerr << "Enemy troops:" << endl;
		for (size_t i = 0; i < enemyTroopCount; i++) {
			cerr << *(enemyTroops[i]) << endl;
		}
		cerr << "My bombs:" << endl;
		for (size_t i = 0; i < myBombCount; i++) {
			cerr << *(myBombs[i]) << endl;
		}
		cerr << "Enemy bombs:" << endl;
		for (size_t i = 0; i < enemyBombCount; i++) {
			cerr << *(enemyBombs[i]) << endl;
		}
#endif

		vector<Action> actions = heuristicWood2_1();

#ifdef DEBUG
		cerr << "turn:" << turn << " factoryCount:" << factoryCount << endl;
#endif

		// Any valid action, such as "WAIT" or "MOVE source destination cyborgs"
		cout << output(actions) << endl;

		reset();

		turn += 1;
	}
}