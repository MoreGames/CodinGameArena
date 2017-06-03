#include "Code4Life.hpp"

// private

void Code4Life::deserializeActions(const vector<string>& botActions, vector<Action>& actions) {
	for (size_t bot = 0; bot < m_bots.size(); bot++) {
		actions.push_back(Action(ACTION_TYPE::WAIT));
	}

	for (size_t bot = 0; bot < m_bots.size(); bot++) {
		Bot* owner = &m_bots[bot];
		string input{ botActions[bot] };
		size_t delim{ 0 };

		while (delim != string::npos) {
			stringstream ss(input.substr(delim, input.find_first_of(';', delim)));
			delim = input.find_first_of(';', delim);
			if (delim != string::npos) {
				++delim;
			}
			string type;
			ss >> type;

			if (type == "WAIT") {
				actions[bot] = Action(ACTION_TYPE::WAIT);
			} else if (type == "GOTO") {
				string target;
				ss >> target;
				if (target == "SAMPLES") {
					actions[bot] = Action(ACTION_TYPE::GOTO, MODULE_TYPE::SAMPLES);
				} else if (target == "DIAGNOSIS") {
					actions[bot] = Action(ACTION_TYPE::GOTO, MODULE_TYPE::DIAGNOSIS);
				} else if (target == "MOLECULES") {
					actions[bot] = Action(ACTION_TYPE::GOTO, MODULE_TYPE::MOLECULES);
				} else if (target == "LABORATORY") {
					actions[bot] = Action(ACTION_TYPE::GOTO, MODULE_TYPE::LABORATORY);
				} else {
					throw(1); // unknown target
				}
			} else if (type == "CONNECT") {
				string type;
				ss >> type;
				int id;
				if (type == "A") {
					id = 0;
				} else if (type == "B") {
					id = 1;
				} else if (type == "C") {
					id = 2;
				} else if (type == "D") {
					id = 3;
				} else if (type == "E") {
					id = 4;
				} else {
					stringstream convert(type);
					if (!(convert >> id)) {
						throw(2); // cant convert to int
					}
				}
				actions[bot] = Action(ACTION_TYPE::CONNECT, id);
			} else if (type == "MSG") {
				continue;
			} else {
				throw(3); // InvalidInputException
			}
		}
	}
}

void Code4Life::output(int turn) {
	printf("######## GAME STATE\n");
	printf("turn #%d\n", turn);
	printf("available molecules:\n");
	printf("A:%d B:%d C:%d D:%d E:%d\n", m_molecules[0], m_molecules[1], m_molecules[2], m_molecules[3], m_molecules[4]);
	printf("bots | samples | projects \n");
	size_t m = max(m_samplesInGame.size(), max(m_scienceProjects.size(), m_botInfos.size()));
	for (size_t i = 0; i < m; i++) {
		if (i < m_botInfos.size()) {
			m_botInfos[i].print();
		} else {
			printf("%58s", "");
		}
		printf(" | ");
		if (i < m_samplesInGame.size()) {
			m_samplesInGame[i].print();
		} else {
			printf("%36s", "");
		}
		printf(" | ");
		if (i < m_scienceProjects.size()) {
			m_scienceProjects[i].print();
		} else {
			printf("%22s", "");
		}
		printf("\n");
	}
	printf("########\n");
}

Sample* Code4Life::getSample(int id) {
	for (Sample& s : m_samplesInGame) {
		if (s.m_id == id)
			return &s;
	}
	return nullptr;
}
Sample* Code4Life::getSample(int bot, int id) {
	for (Sample& s : m_samplesInGame) {
		if (s.m_id == id && s.m_carriedBy != nullptr && s.m_carriedBy->m_bot->m_id == bot)
			return &s;
	}
	return nullptr;
}
void Code4Life::removeSample(Sample* sample) {
	for (auto it = m_samplesInGame.begin(); it != m_samplesInGame.end();) {
		Sample& s{ *it };
		if (s.m_id == sample->m_id) {
			m_samplesInGame.erase(it);
			return;
		}
		it++;
	}
}
int Code4Life::getSampleSize(int bot) {
	int size = 0;
	for (Sample& s : m_samplesInGame) {
		if (s.m_carriedBy->m_bot->m_id == bot) {
			size += 1;
		}
	}
	return size;
}
void Code4Life::requestSample(int bot, int rank) {
	if (getSampleSize(bot) >= MAX_TRAY) {
		throw(6); // trayIsFull
	}

	if (rank < 1 || rank > 3) {
		// throw new LostException("badSampleRank", String.valueOf(rank));
	}

	Sample sample = m_samplePool[rank-1][0];
	m_samplePool[rank-1].erase(m_samplePool[rank-1].begin());
	m_samplePool[rank-1].push_back(sample);

	sample.m_id = id++;
	if (LEAGUE_LEVEL <= 1) {
		sample.m_expertise = -1;
	}
	sample.m_carriedBy = &m_botInfos[bot];
	m_samplesInGame.push_back(sample);
}
void Code4Life::requestDiagnosis(int bot, int id) {
	Sample* sample = getSample(bot, id);
	if (sample != nullptr) { // upload or diagnose
		if (sample->m_discovered) { // upload
			sample->m_carriedBy = nullptr;
		} else if (!sample->m_discovered) { // diagnose
			sample->m_discovered = true;
			sample->m_discoveredBy = &m_botInfos[bot];
		}
	} else { // download
		sample = getSample(id);
		if (sample == nullptr) {
			throw(10); // sample not found
		}
		if (getSampleSize(bot) >= MAX_TRAY) {
			throw(11); // tray is full
		}

		// TODO LEAGUE_LEVEL == 0
		
		if (sample->m_carriedBy == nullptr) {
			sample->m_carriedBy = &m_botInfos[bot];
		} else if (sample->m_discoveredBy->m_bot->m_id == m_botInfos[bot].m_bot->m_id) {
			sample->m_carriedBy = &m_botInfos[bot];
		}
	}
}
void Code4Life::requestMolecule(int bot, int molecule) {
	if (m_molecules[molecule] <= 0) {
		throw (7); // not enough molecules
	}
	if (m_botInfos[bot].getSumStorage() >= MAX_STORAGE) {
		throw (8); // storage is full
	}
	m_botInfos[bot].m_storage[molecule]++;
	m_molecules[molecule]--;
}
bool Code4Life::canAfford(int bot, int cost[5]) {
	for (size_t i = 0; i < 5; i++) {
		if (m_botInfos[bot].m_expertise[i] + m_botInfos[bot].m_storage[i] < cost[i]) {
			return false;
		}
	}
	return true;
}
void Code4Life::requestProduction(int bot, int id) {
	Sample* sample = getSample(bot, id);
	if (sample != nullptr) {
		if (canAfford(bot, sample->m_costs)) {
			for (size_t i = 0; i < 5; i++) {
				int toPay = max(0, sample->m_costs[i] - m_botInfos[bot].m_expertise[i]);
				m_botInfos[bot].m_storage[i] -= toPay;
				m_molecules[i] += toPay;
			}

			m_botInfos[bot].m_score += sample->m_life;
			if (sample->m_expertise != -1) {
				m_botInfos[bot].m_expertise[sample->m_expertise]++;
			}

			removeSample(sample);
		} else {
			throw(9); // cannot afford sample
		}
	} else {
		throw (10); // requested sample not carried by bot
	}
}
bool Code4Life::completedProject(int bot, const ScienceProject& project) {
	for (size_t i = 0; i < 5; i++) {
		if (m_botInfos[bot].m_expertise[i] < project.m_costs[i]) {
			return false;
		}
	}
	return true;
}
int Code4Life::getDistance(MODULE_TYPE from, MODULE_TYPE to) {
	if (LEAGUE_LEVEL >= 2) {
		if (from == MODULE_TYPE::START_POS || to == MODULE_TYPE::START_POS) {
			return 2;
		} else if ((from == MODULE_TYPE::LABORATORY && to == MODULE_TYPE::DIAGNOSIS) ||
			(to == MODULE_TYPE::LABORATORY && from == MODULE_TYPE::DIAGNOSIS)) {
			return 4;
		} else {
			return 3;
		}
	} else {
		return 1;
	}
}
void Code4Life::updateGame(const vector<string>& botActions) {
	vector<Action> actions;
	deserializeActions(botActions, actions);

	for (size_t bot = 0; bot < m_bots.size(); bot++) {
		const Action& action = actions[bot];
		BotInfo& botInfo = m_botInfos[bot];

		if (botInfo.m_eta == 0) {
			if (action.m_type == ACTION_TYPE::GOTO) { // GOTO
				if (action.m_target == MODULE_TYPE::SAMPLES && LEAGUE_LEVEL == 0) {
					throw (4); // InvalidInputException
				}
				if (botInfo.m_target != action.m_target) {
					botInfo.m_eta = getDistance(botInfo.m_target, action.m_target);
					botInfo.m_target = action.m_target;
				}
			} else if (action.m_type == ACTION_TYPE::CONNECT) { // CONNECT
				switch (botInfo.m_target) {
				case MODULE_TYPE::SAMPLES:
					requestSample(bot, action.m_connectionId);
					break;
				case MODULE_TYPE::DIAGNOSIS:
					requestDiagnosis(bot, action.m_connectionId);
					break;
				case MODULE_TYPE::MOLECULES:
					requestMolecule(bot, action.m_connectionId);
					break;
				case MODULE_TYPE::LABORATORY:
					requestProduction(bot, action.m_connectionId);
					break;
				case MODULE_TYPE::START_POS:
					throw(5);
				default:
					break;
				}
			}
		} else {
			botInfo.m_eta--;
		}
	}

	// check for science projects
	vector<int> removes;
	for (size_t bot = 0; bot < m_bots.size(); bot++) {
		for (size_t i = 0; i < m_scienceProjects.size(); i++) {
			ScienceProject& project = m_scienceProjects[i];
			if (completedProject(bot, project)) {
				project.m_remove = true;
				m_botInfos[bot].m_score += SCIENCE_PROJECT_VALUE;
			}
		}

	}
	for (auto it = m_scienceProjects.begin(); it != m_scienceProjects.end();) {
		ScienceProject& project{ *it };
		if (project.m_remove) {
			it = m_scienceProjects.erase(it);
			continue;
		}
		it++;
	}
}

string Code4Life::serializeInitBotInput(int bot) const {
	string scienceProjects{ "" };
	scienceProjects += to_string(m_scienceProjects.size()) + "\n";
	for (const ScienceProject& s : m_scienceProjects) {
		scienceProjects += s.toBotString();
	}
	return scienceProjects;
}
string Code4Life::serializeBotInput(int turn, int bot) const {
	string botInfo{ "" }, enemyInfo{ "" }, molecules{ "" }, samples{ "" };
	for (const BotInfo& info : m_botInfos) {
		if (info.m_bot->m_id == bot) {
			botInfo = info.toBotString();
		} else {
			enemyInfo = info.toBotString();
		}
	}

	molecules = to_string(m_molecules[0]) + " " + to_string(m_molecules[1]) + " " + to_string(m_molecules[2]) + " " + to_string(m_molecules[3]) + " " + to_string(m_molecules[4]) + "\n";

	samples += to_string(m_samplesInGame.size()) + "\n";
	for (const Sample& sample : m_samplesInGame) {
		samples += sample.toBotString(bot);
	}

	string s{ "" };
	s += botInfo;
	s += enemyInfo;
	s += molecules;
	s += samples;

	return s;
}

// public

Code4Life::Code4Life(const vector<string>& botNames, long long seed) : Game(botNames, seed), id(0) {
	for (size_t i = 0; i < 2; i++) {
		m_botInfos.push_back(BotInfo(&m_bots[i]));
	}

	for (size_t i = 0; i < 5; i++) {
		m_molecules[i] = RESOURCE_PER_TYPE_BY_LEAGUE_LEVEL;
	}
	
	int temp[5] = { 0,3,0,0,0 };
	m_samplePool[0].push_back(Sample(temp, 1, 0, 0));
	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 2; temp[4] = 1; m_samplePool[0].push_back(Sample(temp, 1, 0, 0));
	temp[0] = 0; temp[1] = 1; temp[2] = 1; temp[3] = 1; temp[4] = 1; m_samplePool[0].push_back(Sample(temp, 1, 0, 0));
	temp[0] = 0; temp[1] = 2; temp[2] = 0; temp[3] = 0; temp[4] = 2; m_samplePool[0].push_back(Sample(temp, 1, 0, 0));
	temp[0] = 0; temp[1] = 0; temp[2] = 4; temp[3] = 0; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 10, 0, 0));

	temp[0] = 0; temp[1] = 1; temp[2] = 2; temp[3] = 1; temp[4] = 1; m_samplePool[0].push_back(Sample(temp, 1, 0, 0));
	temp[0] = 0; temp[1] = 2; temp[2] = 2; temp[3] = 0; temp[4] = 1; m_samplePool[0].push_back(Sample(temp, 1, 0, 0));
	temp[0] = 3; temp[1] = 1; temp[2] = 0; temp[3] = 0; temp[4] = 1; m_samplePool[0].push_back(Sample(temp, 1, 0, 0));

	temp[0] = 1; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 2; m_samplePool[0].push_back(Sample(temp, 1, 1, 0));
	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 3; m_samplePool[0].push_back(Sample(temp, 1, 1, 0));
	temp[0] = 1; temp[1] = 0; temp[2] = 1; temp[3] = 1; temp[4] = 1; m_samplePool[0].push_back(Sample(temp, 1, 1, 0));
	temp[0] = 0; temp[1] = 0; temp[2] = 2; temp[3] = 0; temp[4] = 2; m_samplePool[0].push_back(Sample(temp, 1, 1, 0));
	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 4; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 10, 1, 0));

	temp[0] = 1; temp[1] = 0; temp[2] = 1; temp[3] = 2; temp[4] = 1; m_samplePool[0].push_back(Sample(temp, 1, 1, 0));
	temp[0] = 1; temp[1] = 0; temp[2] = 2; temp[3] = 2; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 1, 0));
	temp[0] = 0; temp[1] = 1; temp[2] = 3; temp[3] = 1; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 1, 0));

	temp[0] = 2; temp[1] = 1; temp[2] = 0; temp[3] = 0; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 2, 0));
	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 3; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 2, 0));
	temp[0] = 1; temp[1] = 1; temp[2] = 0; temp[3] = 1; temp[4] = 1; m_samplePool[0].push_back(Sample(temp, 1, 2, 0));
	temp[0] = 0; temp[1] = 2; temp[2] = 0; temp[3] = 2; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 2, 0));
	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 4; m_samplePool[0].push_back(Sample(temp, 10, 2, 0));

	temp[0] = 1; temp[1] = 1; temp[2] = 0; temp[3] = 1; temp[4] = 2; m_samplePool[0].push_back(Sample(temp, 1, 2, 0));
	temp[0] = 0; temp[1] = 1; temp[2] = 0; temp[3] = 2; temp[4] = 2; m_samplePool[0].push_back(Sample(temp, 1, 2, 0));	
	temp[0] = 1; temp[1] = 3; temp[2] = 1; temp[3] = 0; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 2, 0));

	temp[0] = 0; temp[1] = 2; temp[2] = 1; temp[3] = 0; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 3, 0));
	temp[0] = 3; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 3, 0));
	temp[0] = 1; temp[1] = 1; temp[2] = 1; temp[3] = 0; temp[4] = 1; m_samplePool[0].push_back(Sample(temp, 1, 3, 0));
	temp[0] = 2; temp[1] = 0; temp[2] = 0; temp[3] = 2; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 3, 0));
	temp[0] = 4; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 10, 3, 0));

	temp[0] = 2; temp[1] = 1; temp[2] = 1; temp[3] = 0; temp[4] = 1; m_samplePool[0].push_back(Sample(temp, 1, 3, 0));
	temp[0] = 2; temp[1] = 0; temp[2] = 1; temp[3] = 0; temp[4] = 2; m_samplePool[0].push_back(Sample(temp, 1, 3, 0));
	temp[0] = 1; temp[1] = 0; temp[2] = 0; temp[3] = 1; temp[4] = 3; m_samplePool[0].push_back(Sample(temp, 1, 3, 0));

	temp[0] = 0; temp[1] = 0; temp[2] = 2; temp[3] = 1; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 4, 0));
	temp[0] = 0; temp[1] = 0; temp[2] = 3; temp[3] = 0; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 4, 0));
	temp[0] = 1; temp[1] = 1; temp[2] = 1; temp[3] = 1; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 4, 0));
	temp[0] = 2; temp[1] = 0; temp[2] = 2; temp[3] = 0; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 4, 0));
	temp[0] = 0; temp[1] = 4; temp[2] = 0; temp[3] = 0; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 10, 4, 0));

	temp[0] = 1; temp[1] = 2; temp[2] = 1; temp[3] = 1; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 4, 0));
	temp[0] = 2; temp[1] = 2; temp[2] = 0; temp[3] = 1; temp[4] = 0; m_samplePool[0].push_back(Sample(temp, 1, 4, 0));
	temp[0] = 0; temp[1] = 0; temp[2] = 1; temp[3] = 3; temp[4] = 1; m_samplePool[0].push_back(Sample(temp, 1, 4, 0));

	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 5; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 20, 0, 1));
	temp[0] = 6; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 30, 0, 1));
	temp[0] = 0; temp[1] = 0; temp[2] = 3; temp[3] = 2; temp[4] = 2; m_samplePool[1].push_back(Sample(temp, 10, 0, 1));
	temp[0] = 0; temp[1] = 0; temp[2] = 1; temp[3] = 4; temp[4] = 2; m_samplePool[1].push_back(Sample(temp, 20, 0, 1));
	temp[0] = 2; temp[1] = 3; temp[2] = 0; temp[3] = 3; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 10, 0, 1));
	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 5; temp[4] = 3; m_samplePool[1].push_back(Sample(temp, 20, 0, 1));

	temp[0] = 0; temp[1] = 5; temp[2] = 0; temp[3] = 0; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 20, 1, 1));
	temp[0] = 0; temp[1] = 6; temp[2] = 0; temp[3] = 0; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 30, 1, 1));
	temp[0] = 0; temp[1] = 2; temp[2] = 2; temp[3] = 3; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 10, 1, 1));
	temp[0] = 2; temp[1] = 0; temp[2] = 0; temp[3] = 1; temp[4] = 4; m_samplePool[1].push_back(Sample(temp, 20, 1, 1));
	temp[0] = 0; temp[1] = 2; temp[2] = 3; temp[3] = 0; temp[4] = 3; m_samplePool[1].push_back(Sample(temp, 20, 1, 1));
	temp[0] = 5; temp[1] = 3; temp[2] = 0; temp[3] = 0; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 20, 1, 1));

	temp[0] = 0; temp[1] = 0; temp[2] = 5; temp[3] = 0; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 20, 2, 1));
	temp[0] = 0; temp[1] = 0; temp[2] = 6; temp[3] = 0; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 30, 2, 1));
	temp[0] = 2; temp[1] = 3; temp[2] = 0; temp[3] = 0; temp[4] = 2; m_samplePool[1].push_back(Sample(temp, 10, 2, 1));
	temp[0] = 3; temp[1] = 0; temp[2] = 2; temp[3] = 3; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 10, 2, 1));
	temp[0] = 4; temp[1] = 2; temp[2] = 0; temp[3] = 0; temp[4] = 1; m_samplePool[1].push_back(Sample(temp, 20, 2, 1));
	temp[0] = 0; temp[1] = 5; temp[2] = 3; temp[3] = 0; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 20, 2, 1));

	temp[0] = 5; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 20, 3, 1));
	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 6; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 30, 3, 1));
	temp[0] = 2; temp[1] = 0; temp[2] = 0; temp[3] = 2; temp[4] = 3; m_samplePool[1].push_back(Sample(temp, 10, 3, 1));
	temp[0] = 1; temp[1] = 4; temp[2] = 2; temp[3] = 0; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 20, 3, 1));
	temp[0] = 0; temp[1] = 3; temp[2] = 0; temp[3] = 2; temp[4] = 3; m_samplePool[1].push_back(Sample(temp, 10, 3, 1));
	temp[0] = 3; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 5; m_samplePool[1].push_back(Sample(temp, 20, 3, 1));

	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 5; m_samplePool[1].push_back(Sample(temp, 20, 4, 1));
	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 6; m_samplePool[1].push_back(Sample(temp, 30, 4, 1));
	temp[0] = 3; temp[1] = 2; temp[2] = 2; temp[3] = 0; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 10, 4, 1));
	temp[0] = 0; temp[1] = 1; temp[2] = 4; temp[3] = 2; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 20, 4, 1));
	temp[0] = 3; temp[1] = 0; temp[2] = 3; temp[3] = 0; temp[4] = 2; m_samplePool[1].push_back(Sample(temp, 10, 4, 1));
	temp[0] = 0; temp[1] = 0; temp[2] = 5; temp[3] = 3; temp[4] = 0; m_samplePool[1].push_back(Sample(temp, 20, 4, 1));

	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 7; m_samplePool[2].push_back(Sample(temp, 40, 0, 2));
	temp[0] = 3; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 7; m_samplePool[2].push_back(Sample(temp, 50, 0, 2));
	temp[0] = 3; temp[1] = 0; temp[2] = 0; temp[3] = 3; temp[4] = 6; m_samplePool[2].push_back(Sample(temp, 40, 0, 2));
	temp[0] = 0; temp[1] = 3; temp[2] = 3; temp[3] = 5; temp[4] = 3; m_samplePool[2].push_back(Sample(temp, 30, 0, 2));

	temp[0] = 7; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 0; m_samplePool[2].push_back(Sample(temp, 40, 1, 2));
	temp[0] = 7; temp[1] = 3; temp[2] = 0; temp[3] = 0; temp[4] = 0; m_samplePool[2].push_back(Sample(temp, 50, 1, 2));
	temp[0] = 6; temp[1] = 3; temp[2] = 0; temp[3] = 0; temp[4] = 3; m_samplePool[2].push_back(Sample(temp, 40, 1, 2));
	temp[0] = 3; temp[1] = 0; temp[2] = 3; temp[3] = 3; temp[4] = 5; m_samplePool[2].push_back(Sample(temp, 30, 1, 2));
	
	temp[0] = 0; temp[1] = 7; temp[2] = 0; temp[3] = 0; temp[4] = 0; m_samplePool[2].push_back(Sample(temp, 40, 2, 2));
	temp[0] = 0; temp[1] = 7; temp[2] = 3; temp[3] = 0; temp[4] = 0; m_samplePool[2].push_back(Sample(temp, 50, 2, 2));
	temp[0] = 3; temp[1] = 6; temp[2] = 3; temp[3] = 0; temp[4] = 0; m_samplePool[2].push_back(Sample(temp, 40, 2, 2));
	temp[0] = 5; temp[1] = 3; temp[2] = 0; temp[3] = 3; temp[4] = 3; m_samplePool[2].push_back(Sample(temp, 30, 2, 2));

	temp[0] = 0; temp[1] = 0; temp[2] = 7; temp[3] = 0; temp[4] = 0; m_samplePool[2].push_back(Sample(temp, 40, 3, 2));
	temp[0] = 0; temp[1] = 0; temp[2] = 7; temp[3] = 3; temp[4] = 0; m_samplePool[2].push_back(Sample(temp, 50, 3, 2));
	temp[0] = 0; temp[1] = 3; temp[2] = 6; temp[3] = 3; temp[4] = 0; m_samplePool[2].push_back(Sample(temp, 40, 3, 2));
	temp[0] = 3; temp[1] = 5; temp[2] = 3; temp[3] = 0; temp[4] = 3; m_samplePool[2].push_back(Sample(temp, 30, 3, 2));

	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 7; temp[4] = 0; m_samplePool[2].push_back(Sample(temp, 40, 4, 2));
	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 7; temp[4] = 3; m_samplePool[2].push_back(Sample(temp, 50, 4, 2));
	temp[0] = 0; temp[1] = 0; temp[2] = 3; temp[3] = 6; temp[4] = 3; m_samplePool[2].push_back(Sample(temp, 40, 4, 2));
	temp[0] = 3; temp[1] = 5; temp[2] = 3; temp[3] = 3; temp[4] = 0; m_samplePool[2].push_back(Sample(temp, 30, 4, 2));
	shuffle(m_samplePool[0], m_random);
	shuffle(m_samplePool[1], m_random);
	shuffle(m_samplePool[2], m_random);

#ifdef DEBUG
	for (size_t j = 0; j < 6; j += 2) {
		Sample& s = m_samplePool[0][j];
		fprintf(stderr, "r:%d, e:%d, l:%2d, cost:%d,%d,%d,%d,%d\n", s.m_rank, s.m_expertise, s.m_life, s.m_costs[0], s.m_costs[1], s.m_costs[2], s.m_costs[3], s.m_costs[4]);
	}
	for (size_t j = 1; j < 6; j += 2) {
		Sample& s = m_samplePool[0][j];
		fprintf(stderr, "r:%d, e:%d, l:%2d, cost:%d,%d,%d,%d,%d\n", s.m_rank, s.m_expertise, s.m_life, s.m_costs[0], s.m_costs[1], s.m_costs[2], s.m_costs[3], s.m_costs[4]);
	}
//	for (size_t i = 0; i < 3; i++) {
//		for (size_t j = 0; j < m_samplePool[i].size(); j++) {
//			Sample& s = m_samplePool[i][j];
//			fprintf(stderr, "r:%d, e:%d, l:%2d, cost:%d,%d,%d,%d,%d\n", s.m_rank, s.m_expertise, s.m_life, s.m_costs[0], s.m_costs[1], s.m_costs[2], s.m_costs[3], s.m_costs[4]);
//		}
//	}
#endif

	// science projects
	vector<ScienceProject> scienceProjectPool;
	temp[0] = 3; temp[1] = 3; temp[2] = 0; temp[3] = 0; temp[4] = 3; scienceProjectPool.push_back(ScienceProject(temp));
	temp[0] = 0; temp[1] = 3; temp[2] = 3; temp[3] = 3; temp[4] = 0; scienceProjectPool.push_back(ScienceProject(temp));
	temp[0] = 3; temp[1] = 0; temp[2] = 0; temp[3] = 3; temp[4] = 3; scienceProjectPool.push_back(ScienceProject(temp));
	temp[0] = 0; temp[1] = 0; temp[2] = 4; temp[3] = 4; temp[4] = 0; scienceProjectPool.push_back(ScienceProject(temp));
	temp[0] = 0; temp[1] = 4; temp[2] = 4; temp[3] = 0; temp[4] = 0; scienceProjectPool.push_back(ScienceProject(temp));
	temp[0] = 0; temp[1] = 0; temp[2] = 0; temp[3] = 4; temp[4] = 4; scienceProjectPool.push_back(ScienceProject(temp));
	temp[0] = 4; temp[1] = 0; temp[2] = 0; temp[3] = 0; temp[4] = 4; scienceProjectPool.push_back(ScienceProject(temp));
	temp[0] = 3; temp[1] = 3; temp[2] = 3; temp[3] = 0; temp[4] = 0; scienceProjectPool.push_back(ScienceProject(temp));
	temp[0] = 0; temp[1] = 0; temp[2] = 3; temp[3] = 3; temp[4] = 3; scienceProjectPool.push_back(ScienceProject(temp));
	temp[0] = 4; temp[1] = 4; temp[2] = 0; temp[3] = 0; temp[4] = 0; scienceProjectPool.push_back(ScienceProject(temp));
	shuffle(scienceProjectPool, m_random);
	for (size_t i = 0; i < SCIENCE_PROJECTS_BY_LEAGUE_LEVEL; i++) {
		ScienceProject project = scienceProjectPool[i];
		m_scienceProjects.push_back(project);
	}

	for (size_t i = 0; i < INIT_DIAGNOSED_SAMPLES_BY_LEAGUE_LEVEL; i++) {
		int rank = 0;
		Sample sample = m_samplePool[rank][0];
		m_samplePool[rank].erase(m_samplePool[rank].begin());
		m_samplePool[rank].push_back(sample);

		sample.m_id = id++;
		sample.m_rank = rank;
		sample.m_discovered = true;
		if (LEAGUE_LEVEL <= 1) {
			sample.m_expertise = -1; // TODO ist das möglich
		}
		m_samplesInGame.push_back(sample);
	}

	// output
#ifdef DEBUG
	output(0);
#endif
};
int Code4Life::run() {
	// run turns
	int winner;
	bool gameRunning = true;
	size_t botCount = m_bots.size();
	vector<string> outputs(botCount);
	vector<future<long long>> threads(botCount);
	size_t turn;
	double time = 0;
	for (turn = 0; turn < TURN_LIMIT; turn++) {

		for (size_t i = 0; i < botCount; i++) {
			string input{ serializeBotInput(turn, i) };

			// send init and turn 0 input
			if (turn == 0) {
				string initInput{ serializeInitBotInput(i) };
#ifdef DEBUG
				cerr << "------- INITINPUT bot: " << i << endl << initInput << "-------" << endl;
				cerr << "------- INPUT bot: " << i << endl << input << "-------" << endl;
#endif
				threads[i] = async(&Networking::handleInitNetworking, &m_networking, i, FIRST_TURN_TIMEOUT, initInput, input, &outputs[i]);
			}
			// send turn input
			else {
#ifdef DEBUG
				cerr << "------- INPUT bot: " << i << endl << input << "-------" << endl;
#endif
				threads[i] = async(&Networking::handleTurnNetworking, &m_networking, i, turn, TURN_TIMEOUT, input, &outputs[i]);
			}
		}

		for (size_t i = 0; i < botCount; i++) {
			long long time = threads[i].get();
			if (time == -1 || time == -2) {
				m_networking.killBot(i);
				if (turn == 0)
					cerr << "INIT " << endl;
				else
					cerr << "TURN #" << turn << " " << endl;

				if (time == -1) cerr << "ERROR: Loss game " << m_seed << " (seed) by timeout of bot #" << m_bots[i].m_id << " name: " << m_bots[i].m_name << endl;
				if (time == -2) cerr << "ERROR: Bot #" << m_bots[i].m_id << " name: " << m_bots[i].m_name << " is dead." << endl;


				for (size_t j = 0; j < botCount; j++) {
					cerr << outputs[j] << endl;
				}

				if (i == 0) {
					return 1;
				} else {
					return 0;
				}
			}
		}

#ifdef DEBUG
		cerr << "********* OUTPUT: " << endl; 
		for (size_t i = 0; i < botCount; i++) {
			cerr << outputs[i] << endl;
		}
		cerr << "*********" << endl;
#endif

		updateGame(outputs);

		// output
#ifdef DEBUG
		output(turn + 1);
#endif

		if (gameIsOver()) {
			winner = evaluateWinner();
			break;
		}
	}

	// check winner after turn limit reached
	if (turn == TURN_LIMIT) {
		winner = evaluateWinner();
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
Code4Life::~Code4Life() {

}