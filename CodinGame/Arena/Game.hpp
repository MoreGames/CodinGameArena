#ifndef GAME_H
#define GAME_H

#include <vector>

#include "Networking.hpp"
#include "Random.hpp"

class Bot {
public:
	unsigned int m_id; // 0 or 1
	string m_name;

	Bot(int id, const string& name) : m_id(id), m_name(name) {
	};
};

class Game {
protected:
	// other
	Networking m_networking;
	Random m_random;
	vector<Bot> m_bots;
	unsigned long long m_seed;

	virtual string serializeInitBotInput(int bot) const = 0;
	virtual string serializeBotInput(int turn, int bot) const = 0;

public:
	Game(const vector<string>& botNames, long long seed = 0) : m_random(seed) {
		// init seed, random
		if (seed == 0) {
			seed = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
			m_seed = Random(seed).nextLong();
		} else {
			Random(seed).nextLong();
			m_seed = seed;
		}
		m_random = Random(m_seed);

		// init bots
		for (size_t i = 0; i < botNames.size(); i++) {
			const string& b = botNames[i];
			m_bots.push_back(Bot(i, b));
		}

		// start and connect bots
		try {
			for (const Bot& b : m_bots) {
				m_networking.startAndConnectBot(b.m_name);
			}
		}
		catch (...) {
			cerr << "One or more of your bot launch command strings failed.  Please check for correctness and try again." << endl;
			exit(1);
		}
	}

	virtual int run() = 0;

	~Game() {
		for (size_t i = 0; i < m_bots.size(); i++) {
			m_networking.killBot(i);
		}
	}
};



#endif
