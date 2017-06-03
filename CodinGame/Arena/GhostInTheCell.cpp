#include "GhostInTheCell.hpp"

// private

void GhostInTheCell::generateFactories() {
	size_t factoryCount = MIN_FACTORY_COUNT + m_random.nextInt(MAX_FACTORY_COUNT - MIN_FACTORY_COUNT + 1);
	m_factories.resize(factoryCount);
	if (factoryCount % 2 == 0) { // factoryCount must be odd
		factoryCount += 1;
	}
	m_factories.resize(factoryCount);

	size_t i = 0;
	// center factory
	m_factories[i++] = Factory(id++, nullptr, 0, 0, 0, Point(MAP_WIDTH / 2, MAP_HEIGHT / 2));
	// other factories
	int factoryRadius{ m_factories.size()>10 ? 600 : 700 };
	int minSpaceBetweenFactories = 2 * (factoryRadius + EXTRA_SPACE_BETWEEN_FACTORIES);
	while (i < factoryCount - 1) {
		int x = m_random.nextInt(MAP_WIDTH / 2 - 2 * factoryRadius) + factoryRadius + EXTRA_SPACE_BETWEEN_FACTORIES;
		int y = m_random.nextInt(MAP_HEIGHT - 2 * factoryRadius) + factoryRadius + EXTRA_SPACE_BETWEEN_FACTORIES;
		Point p(x, y);
		bool valid = validFactorySpawn(p, i, minSpaceBetweenFactories);

		if (valid) {
			int prod = MIN_PRODUCTION_RATE + m_random.nextInt(MAX_PRODUCTION_RATE - MIN_PRODUCTION_RATE + 1);

			if (i == 1) {
				int units = PLAYER_INIT_UNITS_MIN + m_random.nextInt(PLAYER_INIT_UNITS_MAX - PLAYER_INIT_UNITS_MIN + 1);
				m_factories[i++] = Factory(id++, &m_bots[0], units, prod, 0, p);
				m_factories[i++] = Factory(id++, &m_bots[1], units, prod, 0, Point(MAP_WIDTH, MAP_HEIGHT) - p);
			} else {
				int units = m_random.nextInt(5 * prod + 1);
				m_factories[i++] = Factory(id++, nullptr, units, prod, 0, p);
				m_factories[i++] = Factory(id++, nullptr, units, prod, 0, Point(MAP_WIDTH, MAP_HEIGHT) - p);
			}
		}
	}

	// compute distance to all other factories
	for (Factory& factory : m_factories) {
		factory.m_dist.resize(m_factories.size());
	}
	for (size_t i = 0; i<m_factories.size(); i++) {
		for (size_t j = i + 1; j<m_factories.size(); j++) {
			double d{ m_factories[i].distance(m_factories[j]) };
			int dist{ static_cast<int>(round((d - 2 * factoryRadius) / 800.0)) };
			m_factories[i].m_dist[j] = dist;
			m_factories[j].m_dist[i] = dist;
		}
	}

	// make sure that the initial accumulated production rate for all the factories is at least MIN_TOTAL_PRODUCTION_RATE
	int totalProductionRate{ 0 };
	for (const Factory& factory : m_factories) {
		totalProductionRate += factory.m_production;
	}
	for (size_t i = 1; totalProductionRate<MIN_TOTAL_PRODUCTION_RATE && i<m_factories.size(); i++) {
		Factory& factory{ m_factories[i] };
		if (factory.m_production<3) {
			factory.m_production++;
			totalProductionRate++;
		}
	}
}

void GhostInTheCell::output(int turn) {
	printf("turn #%d\n", turn);
	for (size_t i = 0; i < m_remainingBombs.size(); i++) {
		printf("Bot #%d, remaining bombs: %d\n", i, m_remainingBombs[i]);
	}	
	printf("factories | troops | bombs:\n");
	size_t m = max(m_factories.size(), max(m_troops.size(), m_bombs.size()));
	for (size_t i = 0; i < m; i++) {
		if (i < m_factories.size()) {
			m_factories[i].print();
		} else {
			printf("%22s","");
		}
		printf(" | ");
		if (i < m_troops.size()) {
			m_troops[i].print();
		} else {
			printf("%26s","");
		}
		printf(" | ");
		if (i < m_bombs.size()) {
			m_bombs[i].print();
		} else {
			printf("%22s","");
		}
		printf("\n");
	}
}

void GhostInTheCell::updateGame() {

	// move troops and bombs
	for (Troop& troop : m_troops) {
		troop.m_remainingDistance--;
	}
	for (Bomb& bomb : m_bombs) {
		bomb.m_remainingDistance--;;
	}

	// decrease disabled countdown
	for (Factory& factory : m_factories) {
		if (factory.m_turns > 0) {
			factory.m_turns--;
		}
	}

	// Execute orders
	// - check and process bombs
	for (const Action& a : m_bombActions) {
		Bomb bomb(id++, a.m_owner, a.m_source, a.m_target);

		if (m_remainingBombs[a.m_owner->m_id] > 0 && !findWithSameRoute(bomb, m_newBombs)) {
			m_remainingBombs[a.m_owner->m_id]--;
			m_newBombs.push_back(bomb);
			m_bombs.push_back(bomb);
		}
	}
	// - check and process troops
	for (const Action& a : m_moveActions) {
		int cyborgsToMove{ min(a.m_cyborgs, a.m_source->m_cyborgs) };
		Troop troop(id++, a.m_owner, a.m_source, a.m_target, cyborgsToMove);

		if (cyborgsToMove > 0 && !findWithSameRoute(troop, m_newBombs)) {
			a.m_source->m_cyborgs -= cyborgsToMove;
			m_troops.push_back(troop);
		}
	}
	// - check and process increases
	for (const Action& a : m_incActions) {
		if (a.m_source->m_cyborgs >= 10 && a.m_source->m_production<3) {
			a.m_source->m_cyborgs -= 10;
			a.m_source->m_production++;
		}
	}
	
	// create new units
	for (Factory& factory : m_factories) {
		if (factory.m_owner != nullptr) {
			factory.m_cyborgs += factory.getCurrentProduction();
		}
	}

	// solve battles
	vector<array<int, 2>> arrivedTroops(m_factories.size(), { 0, 0 });
	for (auto it = m_troops.begin(); it != m_troops.end();) {
		Troop& troop{ *it };
		if (troop.m_remainingDistance == 0) {
			arrivedTroops[troop.m_target->m_id][troop.m_owner->m_id] += troop.m_cyborgs;
			it = m_troops.erase(it);
		} else {
			it++;
		}
	}
	for (Factory& factory : m_factories) {
		int cyborgs = min(arrivedTroops[factory.m_id][0], arrivedTroops[factory.m_id][1]);
		arrivedTroops[factory.m_id][0] -= cyborgs;
		arrivedTroops[factory.m_id][1] -= cyborgs;

		// remaining units fight on the factory
		for (Bot& bot : m_bots) {
			if (factory.m_owner != nullptr && factory.m_owner->m_id == bot.m_id) { // allied
				factory.m_cyborgs += arrivedTroops[factory.m_id][bot.m_id];
			} else { // opponent
				if (arrivedTroops[factory.m_id][bot.m_id] > factory.m_cyborgs) {
					factory.m_owner = &bot;
					factory.m_cyborgs = arrivedTroops[factory.m_id][bot.m_id] - factory.m_cyborgs;
				} else {
					factory.m_cyborgs -= arrivedTroops[factory.m_id][bot.m_id];
				}
			}
		}
	}

	// solve bombs
	for (auto it = m_bombs.begin(); it != m_bombs.end();) { // bombs move and explode
		Bomb& bomb{ *it };
		if (bomb.m_remainingDistance == 0) {
			bomb.explode();
			it = m_bombs.erase(it);
		} else {
			it++;
		}
	}

	// check end conditions
	for (const Bot& bot : m_bots) {
		if (!botAlive(bot.m_id)) {
			m_networking.killBot(bot.m_id);
		}
	}
}

void GhostInTheCell::botActions(const string& input, int bot) {
	vector<Action> actions;

	try {
		deserializeActions(input, bot, actions);
		groupActions(actions);
		validateActions(actions);
	} catch (const int ex) {
		if (ex == 2) {
			cerr << "Invalid move from AI " << bot << " name: " << m_bots[bot].m_name << endl;
		} else if (ex == 3) {
			cerr << "Unrecognised move from AI " << bot << " name: " << m_bots[bot].m_name << endl;
		}
		m_networking.killBot(bot);
	}

	processActions(actions);
}
void GhostInTheCell::deserializeActions(const string& input, int bot, vector<Action>& actions) {
	//cerr << Move << endl;
	stringstream ss(input);
	string type;
	ss >> type;
	if (type == "WAIT") {
		return;
	} else {
		size_t delim{ 0 };
		while (delim != string::npos) {
			stringstream ss(input.substr(delim, input.find_first_of(';', delim)));
			delim = input.find_first_of(';', delim);
			if (delim != string::npos) {
				++delim;
			}
			string type;
			ss >> type;
			Bot* owner = &m_bots[bot];
			if (type == "MOVE") {
				int from, to, cyborgs;
				ss >> from >> to >> cyborgs;
				actions.push_back(Action(owner, &m_factories[from], &m_factories[to], cyborgs));
			} else if (type == "BOMB") {
				int from, to;
				ss >> from >> to;
				actions.push_back(Action(owner, &m_factories[from], &m_factories[to]));
			} else if (type == "INC") {
				int from;
				ss >> from;
				actions.push_back(Action(owner, &m_factories[from]));
			} else if (type == "MSG") {
				continue;
			} else {
				throw(3);
			}
		}
	}
}
void GhostInTheCell::groupActions(vector<Action>& actions) {
	vector<Action> temp;

	// insert all bombs and inc
	for (size_t i = 0; i < actions.size(); i++) {
		if (actions[i].m_target == nullptr || actions[i].m_cyborgs == 0) {
			temp.push_back(actions[i]);
			actions.erase(actions.begin() + i);
			i = -1;
		}
	}

	// insert grouped moves
	if (!actions.empty()) {
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

		temp.push_back(actions[0]);
		for (size_t i = 1; i < actions.size(); i++) {
			if (actions[i].m_source->m_id == actions[i - 1].m_source->m_id && actions[i].m_target->m_id == actions[i - 1].m_target->m_id) {
				temp[temp.size() - 1].m_cyborgs += actions[i].m_cyborgs;
			} else {
				temp.push_back(actions[i]);
			}
		}
	}

	actions = temp;
}
void GhostInTheCell::validateActions(const vector<Action>& actions) {
	for (const Action& a : actions) {
		if (invalidFactoryId(a.m_source->m_id)) {
			cerr << "Invalid source: " << a << endl;
			throw(2);
		}
		Factory& factory{ m_factories[a.m_source->m_id] };
		if (factory.m_owner == nullptr || a.m_owner->m_id != factory.m_owner->m_id) {
			cerr << "Invalid factory owner: " << a << endl;
			throw(2);
		}
		if (a.m_type == ACTION_TYPE::MOVE || a.m_type == ACTION_TYPE::BOMB) {
			if (a.m_source->m_id == a.m_target->m_id || invalidFactoryId(a.m_target->m_id)) {
				cerr << "Invalid target: " << a << endl;
				throw(2);
			}
		}
		if (a.m_type == ACTION_TYPE::MOVE) {
			if (a.m_cyborgs < 0) {
				cerr << "Invalid cyborgs: " << a << endl;
				throw(2);
			}
		}
	}
}
void GhostInTheCell::processActions(const vector<Action>& actions) {
	for (const Action& a : actions) {
		if (a.m_type == ACTION_TYPE::BOMB) {
			m_bombActions.push_back(a);
		}
		if (a.m_type == ACTION_TYPE::MOVE) {
			m_moveActions.push_back(a);
		}
		if (a.m_type == ACTION_TYPE::INC) {
			m_incActions.push_back(a);
		}
	}
}

string GhostInTheCell::serializeInitBotInput(int bot) const {
	stringstream ss;
	ss << m_factories.size() << " " << m_factories.size()*(m_factories.size() - 1) / 2 << endl;
	for (size_t i = 0; i<m_factories.size(); i++) {
		for (size_t j = i + 1; j<m_factories.size(); j++) {
			ss << i << " " << j << " " << m_factories[i].m_dist[j] << endl;
		}
	}
	return ss.str();
}
string GhostInTheCell::serializeBotInput(int turn, int bot) const {
	string s = to_string(m_factories.size() + m_troops.size() + m_bombs.size())+"\n";
	for (const Factory& f : m_factories) {
		s += f.toBotString(bot);
	}
	for (const Troop& t : m_troops) {
		s += t.toBotString(bot);
	}
	for (const Bomb& b : m_bombs) {
		s += b.toBotString(bot);
	}
	return s;
}

// public

GhostInTheCell::GhostInTheCell(const vector<string>& botNames, long long seed) : Game(botNames, seed), id(0) {
	// no need to swap positions, everything is perfect symmetric
	//default_random_engine generator(seed);
	//uniform_int_distribution<unsigned char> swapDistrib(0, 1);
	//const bool playerSwap{ swapDistrib(generator) == 1 };
	//if (playerSwap) {
	//	swap(botNames[0], botNames[1]);
	//}

	// init bombs
	for (size_t i = 0; i < botNames.size(); i++) {
		m_remainingBombs.push_back(BOMBS_PER_PLAYER);
	}
	// build factories
	generateFactories();
};

int GhostInTheCell::run() {
	// run turns
	int winner;
	bool gameRunning = true;
	size_t botCount = m_bots.size();
	vector<string> outputs(botCount);
	vector<future<long long>> threads(botCount);
	size_t turn;
	for (turn = 0; gameRunning && turn < TURN_LIMIT; turn++) {

		for (size_t i = 0; i < botCount; i++) {
			string input{ serializeBotInput(turn, i) };

			// send init and turn 0 input
			if (turn == 0) {
				string initInput{ serializeInitBotInput(i) };
				threads[i] = async(&Networking::handleInitNetworking, &m_networking, i, FIRST_TURN_TIMEOUT, initInput, input, &outputs[i]);
			}
			// send turn input
			else {
				if (botAlive(i)) {
					threads[i] = async(&Networking::handleTurnNetworking, &m_networking, i, turn, TURN_TIMEOUT, input, &outputs[i]);
				}
			}
		}

		for (size_t i = 0; i < botCount; i++) {
			long long time = threads[i].get();
			if (time == -1 || time == -2) {
				m_networking.killBot(i);
				if (turn == 0) cerr << "INIT " << endl; else cerr << "TURN #" << turn << " " << endl;
				if (time == -1) cerr << "ERROR: Loss game " << m_seed << " (seed) by timeout of bot #" << m_bots[i].m_id << " name: " << m_bots[i].m_name << endl;
				if (time == -2) cerr << "ERROR: Bot #" << m_bots[i].m_id << " name: " << m_bots[i].m_name << " is dead." << endl;
				if (i == 0) {
					return 1;
				} else {
					return 0;
				}
			}
		}

		m_bombActions.clear();
		m_moveActions.clear();
		m_incActions.clear();
		for (size_t i = 0; i < botCount; i++) {
			botActions(outputs[i], i);
		}
		
		updateGame();

		// output
#ifdef DEBUG
		output(turn);
#endif

		// check draw
		if (allDead()) {
			winner = -1;
			break;
		}
		// check
		for (size_t i = 0; i < botCount; i++) {
			if (botWon(i)) {
				winner = i;
				gameRunning = false;
				break;
			}
		}
	}

	// check winner after turn limit reached
	if (turn == TURN_LIMIT) {
		array<int, 2> cyborgs = { 0, 0 };
		for (size_t i = 0; i < botCount; i++) {
			cyborgs[i] = cyborgCount(i);
		}
		if (cyborgs[0] == cyborgs[1]) {
			winner = -1;
		} else if (cyborgs[0]>cyborgs[1]) {
			winner = 0;
		} else {
			winner = 1;
		}
	}

#ifdef DEBUG
	for (size_t i = 0; i < botCount; i++) {
		string logFile = "bot_" + to_string(i) + ".log";
		ofstream file(logFile, ios::app);
		file << m_networking.botLogs[i];
		file.flush();
		file.close();
	}
#endif

	return winner;
}

GhostInTheCell::~GhostInTheCell() {

}
