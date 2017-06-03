#include "CodersOfTheCaribbean.hpp"

// private

double times[11] = { 0 };

ACTION_TYPE CodersOfTheCaribbean::moveTo(const Ship& ship, int x, int y) {
	Coord currentPosition = ship.m_position;
	Coord targetPosition = Coord(x, y);

	if (currentPosition == targetPosition) {
		return ACTION_TYPE::SLOWER;
	}

	double targetAngle, angleStraight, anglePort, angleStarboard, centerAngle, anglePortCenter, angleStarboardCenter;
	ACTION_TYPE ret;
	switch (ship.m_speed) {
	case 2:
		ret = ACTION_TYPE::SLOWER;
		break;
	case 1: {
		// Suppose we've moved first
		currentPosition = currentPosition.neighbor(ship.m_orientation);
		if (!currentPosition.isInsideMap()) {
			return ACTION_TYPE::SLOWER;
		}

		// Target reached at next turn
		if (currentPosition == targetPosition) {
			return ACTION_TYPE::EMPTY;
		}

		// For each neighbor cell, find the closest to target
		targetAngle = currentPosition.angle(targetPosition);
		angleStraight = min(abs(ship.m_orientation - targetAngle), 6 - abs(ship.m_orientation - targetAngle));
		anglePort = min(abs((ship.m_orientation + 1) - targetAngle), abs((ship.m_orientation - 5) - targetAngle));
		angleStarboard = min(abs((ship.m_orientation + 5) - targetAngle), abs((ship.m_orientation - 1) - targetAngle));

		centerAngle = currentPosition.angle(Coord(MAP_WIDTH / 2, MAP_HEIGHT / 2));
		anglePortCenter = min(abs((ship.m_orientation + 1) - centerAngle), abs((ship.m_orientation - 5) - centerAngle));
		angleStarboardCenter = min(abs((ship.m_orientation + 5) - centerAngle), abs((ship.m_orientation - 1) - centerAngle));

		// Next to target with bad angle, slow down then rotate (avoid to turn around the target!)
		if (currentPosition.distanceTo(targetPosition) == 1 && angleStraight > 1.5) {
			return ACTION_TYPE::SLOWER;
		}

		int distanceMin = -1;

		// Test forward
		Coord nextPosition = currentPosition.neighbor(ship.m_orientation);
		if (nextPosition.isInsideMap()) {
			distanceMin = nextPosition.distanceTo(targetPosition);
			ret = ACTION_TYPE::EMPTY;
		}

		// Test port
		nextPosition = currentPosition.neighbor((ship.m_orientation + 1) % 6);
		if (nextPosition.isInsideMap()) {
			int distance = nextPosition.distanceTo(targetPosition);
			if (distanceMin == -1 || distance < distanceMin || distance == distanceMin && anglePort < angleStraight - 0.5) {
				distanceMin = distance;
				ret = ACTION_TYPE::PORT;
			}
		}

		// Test starboard
		nextPosition = currentPosition.neighbor((ship.m_orientation + 5) % 6);
		if (nextPosition.isInsideMap()) {
			int distance = nextPosition.distanceTo(targetPosition);
			if (distanceMin == -1 || distance < distanceMin
				|| (distance == distanceMin && angleStarboard < anglePort - 0.5 && ret == ACTION_TYPE::PORT)
				|| (distance == distanceMin && angleStarboard < angleStraight - 0.5 && ret == ACTION_TYPE::EMPTY)
				|| (distance == distanceMin && ret == ACTION_TYPE::PORT && angleStarboard == anglePort && angleStarboardCenter < anglePortCenter)
				|| (distance == distanceMin && ret == ACTION_TYPE::PORT && angleStarboard == anglePort	&& angleStarboardCenter == anglePortCenter && (ship.m_orientation == 1 || ship.m_orientation == 4))) {
				distanceMin = distance;
				ret = ACTION_TYPE::STARBOARD;
			}
		}
		break;
	}
	case 0: {
		// Rotate ship towards target
		targetAngle = currentPosition.angle(targetPosition);
		angleStraight = min(abs(ship.m_orientation - targetAngle), 6 - abs(ship.m_orientation - targetAngle));
		anglePort = min(abs((ship.m_orientation + 1) - targetAngle), abs((ship.m_orientation - 5) - targetAngle));
		angleStarboard = min(abs((ship.m_orientation + 5) - targetAngle), abs((ship.m_orientation - 1) - targetAngle));

		centerAngle = currentPosition.angle(Coord(MAP_WIDTH / 2, MAP_HEIGHT / 2));
		anglePortCenter = min(abs((ship.m_orientation + 1) - centerAngle), abs((ship.m_orientation - 5) - centerAngle));
		angleStarboardCenter = min(abs((ship.m_orientation + 5) - centerAngle), abs((ship.m_orientation - 1) - centerAngle));

		Coord forwardPosition = currentPosition.neighbor(ship.m_orientation);

		ret = ACTION_TYPE::EMPTY;

		if (anglePort <= angleStarboard) {
			ret = ACTION_TYPE::PORT;
		}

		if (angleStarboard < anglePort || angleStarboard == anglePort && angleStarboardCenter < anglePortCenter
			|| angleStarboard == anglePort && angleStarboardCenter == anglePortCenter && (ship.m_orientation == 1 || ship.m_orientation == 4)) {
			ret = ACTION_TYPE::STARBOARD;
		}

		if (forwardPosition.isInsideMap() && angleStraight <= anglePort && angleStraight <= angleStarboard) {
			ret = ACTION_TYPE::FASTER;
		}
		break;
	}
	}
	return ret;
}
void CodersOfTheCaribbean::deserializeActions(const vector<string>& botActions, vector<Action>& actions) {
	for (Ship& ship : m_ships) { // TODO check
		actions.push_back(Action(ACTION_TYPE::EMPTY, &ship));
	}

	for (size_t bot = 0; bot < m_bots.size(); bot++) {
		Bot* owner = &m_bots[bot];
		string input{ botActions[bot] };
		size_t delim{ 0 };

		size_t i;
		for (i = 0; i < m_ships.size(); i++) {
			if (m_ships[i].m_owner->m_id == bot) {
				break;
			}
		}
		while (delim != string::npos) {
			stringstream ss(input.substr(delim, input.find_first_of(';', delim)));
			delim = input.find_first_of(';', delim);
			if (delim != string::npos) {
				++delim;
			}
			string type;
			ss >> type;

			Ship* ship = &m_ships[i];

			if (type == "WAIT") {
				actions[i++] = Action(ACTION_TYPE::EMPTY, ship);
			} else if (type == "MOVE") {
				int x, y;
				ss >> x >> y;
				actions[i++] = Action(moveTo(*ship, x, y), ship, x, y);
			} else if (type == "FASTER") {
				actions[i++] = Action(ACTION_TYPE::FASTER, ship);
			} else if (type == "SLOWER") {
				actions[i++] = Action(ACTION_TYPE::SLOWER, ship);
			} else if (type == "PORT") {
				actions[i++] = Action(ACTION_TYPE::PORT, ship);
			} else if (type == "STARBOARD") {
				actions[i++] = Action(ACTION_TYPE::STARBOARD, ship);
			} else if (type == "FIRE") {
				int x, y;
				ss >> x >> y;
				if (CANNONS_ENABLED) {
					actions[i++] = Action(ACTION_TYPE::FIRE, ship, x, y);
				} else {
					actions[i++] = Action(ACTION_TYPE::EMPTY, ship);
				}
			} else if (type == "MINE") {
				actions[i++] = Action(ACTION_TYPE::MINE, ship);
			} else {
				throw(3);
			}
		}
	}
}

void CodersOfTheCaribbean::output(int turn) {
	printf("turn #%d\n", turn);
	printf("ships | mines | barrels | balls:\n");
	size_t m = max(m_ships.size(), max(m_mines.size(), max(m_cannonballs.size(), m_barrels.size())));
	for (size_t i = 0; i < m; i++) {
		if (i < m_ships.size()) {
			m_ships[i].print();
		} else {
			printf("%31s", "");
		}
		printf(" | ");
		if (i < m_mines.size()) {
			m_mines[i].print();
		} else {
			printf("%12s", "");
		}
		printf(" | ");
		if (i < m_barrels.size()) {
			m_barrels[i].print();
		} else {
			printf("%22s", "");
		}
		if (i < m_cannonballs.size()) {
			m_cannonballs[i].print();
		} else {
			printf("%22s", "");
		}
		printf("\n");
	}
}

void CodersOfTheCaribbean::moveCannonballs(vector<Coord>& cannonBallExplosions) {
	for (auto it = m_cannonballs.begin(); it != m_cannonballs.end();) {
		Cannonball& ball{ *it };
		if (ball.m_remainingTurns == 0) {
			it = m_cannonballs.erase(it);
			continue;
		} else if (ball.m_remainingTurns > 0) {
			ball.m_remainingTurns--;
		}

		if (ball.m_remainingTurns == 0) {
			cannonBallExplosions.push_back(ball.m_position);
		}
		it++;
	}
}
void CodersOfTheCaribbean::decrementRum() {
	for (Ship& ship : m_ships) {
		ship.damage(1);
	}
}
void CodersOfTheCaribbean::updateInitialRum() {
	for (Ship& ship : m_ships) {
		ship.m_initialHealth = ship.m_health;
	}
}
bool CodersOfTheCaribbean::cellIsFreeOfBarrels(const Coord& target) {
	for (const RumBarrel& barrel : m_barrels) {
		if (barrel.m_position == target) {
			return false;
		}
	}
	return true;
}
bool CodersOfTheCaribbean::cellIsFreeOfMines(const Coord& target) {
	for (const Mine& mine : m_mines) {
		if (mine.m_position == target) {
			return false;
		}
	}
	return true;
}
bool CodersOfTheCaribbean::cellIsFreeOfShips(const Coord& target) {
	for (const Ship& ship : m_ships) {
		if (ship.m_position == target) {
			return false;
		}
	}
	return true;
}
void CodersOfTheCaribbean::applyActions(const vector<Action>& actions) {
	// one action for each ship, order of actions is order of ships
	for (size_t i = 0; i < actions.size(); i++) {
		Ship& ship = m_ships[i];
		if (ship.m_mineCooldown > 0) {
			ship.m_mineCooldown--;
		}
		if (ship.m_cannonCooldown > 0) {
			ship.m_cannonCooldown--;
		}

		ship.m_newOrientation = ship.m_orientation;

		Action action = actions[i];
		if (action.m_type != ACTION_TYPE::EMPTY) {
			switch (action.m_type) {
			case ACTION_TYPE::FASTER:
				if (ship.m_speed < MAX_SHIP_SPEED) {
					ship.m_speed++;
				}
				break;
			case ACTION_TYPE::SLOWER:
				if (ship.m_speed > 0) {
					ship.m_speed--;
				}
				break;
			case ACTION_TYPE::PORT:
				ship.m_newOrientation = (ship.m_orientation + 1) % 6;
				break;
			case ACTION_TYPE::STARBOARD:
				ship.m_newOrientation = (ship.m_orientation + 5) % 6;
				break;
			case ACTION_TYPE::MINE:
				if (ship.m_mineCooldown == 0) {
					Coord target = ship.stern().neighbor((ship.m_orientation + 3) % 6);

					if (target.isInsideMap()) {
						if (cellIsFreeOfBarrels(target) && cellIsFreeOfShips(target) && cellIsFreeOfMines(target)) {
							ship.m_mineCooldown = COOLDOWN_MINE;
							Mine mine = Mine(id++, target.m_x, target.m_y);
							m_mines.push_back(mine);
						}
					}

				}
				break;
			case ACTION_TYPE::FIRE: {
				int distance = ship.bow().distanceTo(action.m_target);
				if (action.m_target.isInsideMap() && distance <= FIRE_DISTANCE_MAX && ship.m_cannonCooldown == 0) {
					int travelTime = (int)(1 + round(ship.bow().distanceTo(action.m_target) / 3.0));
					m_cannonballs.push_back(Cannonball(id++, action.m_target.m_x, action.m_target.m_y, ship.m_id, ship.bow().m_x, ship.bow().m_y, travelTime));
					ship.m_cannonCooldown = COOLDOWN_CANNON;
				}
				break;
			}
			default:
				break;
			}
		}
	}
}
void CodersOfTheCaribbean::checkBarrelCollisions(Ship& ship) {
	Coord bow = ship.bow();
	Coord stern = ship.stern();
	Coord center = ship.m_position;

	// Collision with the barrels
	for (auto it = m_barrels.begin(); it != m_barrels.end();) {
		RumBarrel& barrel{ *it };
		if (barrel.m_position == bow || barrel.m_position == stern || barrel.m_position == center) {
			ship.heal(barrel.m_health);
			it = m_barrels.erase(it);
		} else {
			it++;
		}
	}
}
void CodersOfTheCaribbean::checkMineCollisions() {
	for (auto it = m_mines.begin(); it != m_mines.end();) {
		Mine& mine{ *it };
		bool damage = mine.explode(m_ships, false);

		if (damage) {
			it = m_mines.erase(it);
		} else {
			it++;
		}
	}
}
void CodersOfTheCaribbean::checkCollisions() {
	// Check collisions with Barrels
	for (Ship& ship : m_ships) {
		checkBarrelCollisions(ship);
	}

	// Check collisions with Mines
	checkMineCollisions();
}
void CodersOfTheCaribbean::moveShips() {
	// ---
	// Go forward
	// ---
	for (int i = 1; i <= MAX_SHIP_SPEED; i++) {
		for (Ship& ship : m_ships) {
			ship.m_newPosition = ship.m_position;
			ship.m_newBowCoordinate = ship.bow();
			ship.m_newSternCoordinate = ship.stern();

			if (i > ship.m_speed) {
				continue;
			}

			Coord newCoordinate = ship.m_position.neighbor(ship.m_orientation);

			if (newCoordinate.isInsideMap()) {
				// Set new coordinate.
				ship.m_newPosition = newCoordinate;
				ship.m_newBowCoordinate = newCoordinate.neighbor(ship.m_orientation);
				ship.m_newSternCoordinate = newCoordinate.neighbor((ship.m_orientation + 3) % 6);
			} else {
				// Stop ship!
				ship.m_speed = 0;
			}
		}

		// Check ship and obstacles collisions
		vector<Ship*> collisions(0);
		bool collisionDetected = true;
		while (collisionDetected) {
			collisionDetected = false;

			for (Ship& ship : m_ships) {
				if (ship.newBowIntersect(m_ships)) {
					collisions.push_back(&ship);
				}
			}

			for (Ship* ship : collisions) {
				// Revert last move
				ship->m_newPosition = ship->m_position;
				ship->m_newBowCoordinate = ship->bow();
				ship->m_newSternCoordinate = ship->stern();

				// Stop ships
				ship->m_speed = 0;

				collisionDetected = true;
			}
			collisions.clear();
		}

		// Move ships to their new location
		for (Ship& ship : m_ships) {
			ship.m_position = ship.m_newPosition;
		}

		// Check collisions
		checkCollisions();
	}
}
void CodersOfTheCaribbean::rotateShips() {
	// Rotate
	for (Ship& ship : m_ships) {
		ship.m_newPosition = ship.m_position;
		ship.m_newBowCoordinate = ship.newBow();
		ship.m_newSternCoordinate = ship.newStern();
	}

	// Check collisions
	bool collisionDetected = true;
	vector<Ship*> collisions(0);
	while (collisionDetected) {
		collisionDetected = false;

		for (Ship& ship : m_ships) {
			if (ship.newPositionsIntersect(m_ships)) {
				collisions.push_back(&ship);
			}
		}

		for (Ship* ship : collisions) {
			ship->m_newOrientation = ship->m_orientation;
			ship->m_newBowCoordinate = ship->newBow();
			ship->m_newSternCoordinate = ship->newStern();
			ship->m_speed = 0;
			collisionDetected = true;
		}

		collisions.clear();
	}

	// Apply rotation
	for (Ship& ship : m_ships) {
		ship.m_orientation = ship.m_newOrientation;
	}

	// Check collisions 
	checkCollisions();
}
void CodersOfTheCaribbean::explodeShips(vector<Coord>& cannonBallExplosions) {
	// TODO
	for (auto it = cannonBallExplosions.begin(); it != cannonBallExplosions.end();) {
		const Coord& position{ *it };
		bool removed = false;
		for (Ship ship : m_ships) {
			if (position == ship.bow() || position == ship.stern()) {
				ship.damage(LOW_DAMAGE);
				it = cannonBallExplosions.erase(it);
				removed = true;
				break;
			} else if (position == ship.m_position) {
				ship.damage(HIGH_DAMAGE);
				it = cannonBallExplosions.erase(it);
				removed = true;
				break;
			}
		}

		if (!removed) {
			it++;
		}
	}
}
void CodersOfTheCaribbean::explodeMines(vector<Coord>& cannonBallExplosions) {
	for (auto itBall = cannonBallExplosions.begin(); itBall != cannonBallExplosions.end();) {
		const Coord& position{ *itBall };
		bool removed = false;
		for (auto it = m_mines.begin(); it != m_mines.end(); it++) {
			Mine& mine{ *it };
			if (mine.m_position == position) {
				mine.explode(m_ships, true);
				itBall = cannonBallExplosions.erase(itBall);
				m_mines.erase(it);
				removed = true;
				break;
			}
		}

		if (!removed) {
			itBall++;
		}
	}
}
void CodersOfTheCaribbean::explodeBarrels(vector<Coord>& cannonBallExplosions) {
	for (auto itBall = cannonBallExplosions.begin(); itBall != cannonBallExplosions.end();) {
		const Coord& position{ *itBall };
		bool removed = false;
		for (auto it = m_barrels.begin(); it != m_barrels.end(); it++) {
			RumBarrel& barrel{ *it };
			if (barrel.m_position == position) {
				itBall = cannonBallExplosions.erase(itBall);
				m_barrels.erase(it);
				removed = true;
				break;
			}
		}

		if (!removed) {
			itBall++;
		}
	}
}
void CodersOfTheCaribbean::updateGame(const vector<string>& botActions) {
	vector<Coord> cannonBallExplosions;

	high_resolution_clock::time_point s = high_resolution_clock::now();
	moveCannonballs(cannonBallExplosions);
	times[0] =+ duration_cast<duration<double>>(high_resolution_clock::now() - s).count();
	decrementRum();
	times[1] = +duration_cast<duration<double>>(high_resolution_clock::now() - s).count();
	updateInitialRum();
	times[2] = +duration_cast<duration<double>>(high_resolution_clock::now() - s).count();

	vector<Action> actions;
	deserializeActions(botActions, actions);
	times[3] = +duration_cast<duration<double>>(high_resolution_clock::now() - s).count();
	applyActions(actions);
	times[4] = +duration_cast<duration<double>>(high_resolution_clock::now() - s).count();
	moveShips();
	times[5] = +duration_cast<duration<double>>(high_resolution_clock::now() - s).count();
	rotateShips();
	times[6] = +duration_cast<duration<double>>(high_resolution_clock::now() - s).count();

	explodeShips(cannonBallExplosions);
	times[7] = +duration_cast<duration<double>>(high_resolution_clock::now() - s).count();
	explodeMines(cannonBallExplosions);
	times[8] = +duration_cast<duration<double>>(high_resolution_clock::now() - s).count();
	explodeBarrels(cannonBallExplosions);
	times[9] = +duration_cast<duration<double>>(high_resolution_clock::now() - s).count();
	// For each sunk ship, create a new rum barrel with the amount of rum the ship had at the begin of the turn (up to 30).
	for (Ship ship : m_ships) {
		if (ship.m_health <= 0) {
			int reward = min(REWARD_RUM_BARREL_VALUE, ship.m_initialHealth);
			if (reward > 0) {
				m_barrels.push_back(RumBarrel(id++, ship.m_position.m_x, ship.m_position.m_y, reward));
			}
		}
	}

	for (auto it = m_ships.begin(); it != m_ships.end();) {
		const Ship& ship{ *it };
		if (ship.m_health <= 0) {
			it = m_ships.erase(it);
		} else {
			it++;
		}
	}
	times[10] = +duration_cast<duration<double>>(high_resolution_clock::now() - s).count();
}

string CodersOfTheCaribbean::serializeInitBotInput(int bot) const {
	return serializeBotInput(0, bot);
}
string CodersOfTheCaribbean::serializeBotInput(int turn, int bot) const {
	string botShips{ "" }, enemyShips{ "" }, visibleMines{ "" }, balls{ "" }, barrels{ "" };
	size_t shipCount{ 0 }, mineCount{ 0 };
	for (const Ship& ship : m_ships) {
		if (ship.m_owner->m_id == bot) {
			shipCount++;
			botShips += ship.toBotString(bot);
		} else {
			enemyShips += ship.toBotString(bot);
		}
	}
	// Visible mines
	for (const Mine& mine : m_mines) {
		bool visible = false;
		for (const Ship& ship : m_ships) {
			if (ship.m_position.distanceTo(mine.m_position) <= MINE_VISIBILITY_RANGE) {
				visible = true;
				break;
			}
		}
		if (visible) {
			mineCount++;
			visibleMines += mine.toBotString(bot);
		}
	}
	for (const Cannonball& ball : m_cannonballs) {
		balls += ball.toBotString(bot);
	}
	for (const RumBarrel& barrel : m_barrels) {
		barrels += barrel.toBotString(bot);
	}

	string s = to_string(shipCount) + "\n";
	s += to_string(m_ships.size() + mineCount + m_cannonballs.size() + m_barrels.size()) + "\n";
	s += botShips;
	s += enemyShips;
	s += visibleMines;
	s += balls;
	s += barrels;

	return s;
}

// public

CodersOfTheCaribbean::CodersOfTheCaribbean(const vector<string>& botNames, long long seed, size_t mineCount, size_t barrelCount, size_t shipsPerPlayer) : Game(botNames, seed), id(0) {
	if (shipsPerPlayer == 0) {
		shipsPerPlayer = m_random.nextInt(1 + MAX_SHIPS - MIN_SHIPS) + MIN_SHIPS;
	} else {
		m_random.nextInt(1 + MAX_SHIPS - MIN_SHIPS);
		shipsPerPlayer = max(MIN_SHIPS, min(MAX_SHIPS, shipsPerPlayer));
	}

	if (MAX_MINES > MIN_MINES) {
		if (mineCount == 0) {
			mineCount = m_random.nextInt(MAX_MINES - MIN_MINES) + MIN_MINES;
		} else {
			m_random.nextInt(MAX_MINES - MIN_MINES);
			mineCount = max(MIN_MINES, min(MAX_MINES, mineCount));
		}
	} else {
		mineCount = MIN_MINES;
	}

	if (barrelCount == 0) {
		barrelCount = m_random.nextInt(MAX_RUM_BARRELS - MIN_RUM_BARRELS) + MIN_RUM_BARRELS;
	} else {
		m_random.nextInt(MAX_RUM_BARRELS - MIN_RUM_BARRELS);
		barrelCount = max(MIN_RUM_BARRELS, min(MAX_RUM_BARRELS, barrelCount));
	}

	// Generate Ships
	for (size_t i = 0; i < shipsPerPlayer; i++) {
		int xMin = 1 + i * MAP_WIDTH / shipsPerPlayer;
		int xMax = (i + 1) * MAP_WIDTH / shipsPerPlayer - 2;

		int y = 1 + m_random.nextInt(MAP_HEIGHT / 2 - 2);
		int x = xMin + m_random.nextInt(1 + xMax - xMin);
		int orientation = m_random.nextInt(6);

		Ship ship0 = Ship(id++, &m_bots[0], x, y, orientation);
		Ship ship1 = Ship(id++, &m_bots[1], x, MAP_HEIGHT - 1 - y, (6 - orientation) % 6);

		m_ships.push_back(ship0);
		m_ships.push_back(ship1);
	}

	// Generate mines
	while (m_mines.size() < mineCount) {
		int x = 1 + m_random.nextInt(MAP_WIDTH - 2);
		int y = 1 + m_random.nextInt(MAP_HEIGHT / 2);

		Mine m = Mine(id++, x, y);
		if (cellIsFreeOfShips(m.m_position) && cellIsFreeOfMines(m.m_position)) {
			if (y != MAP_HEIGHT - 1 - y) {
				m_mines.push_back(Mine(id++, x, MAP_HEIGHT - 1 - y));
			}
			m_mines.push_back(m);
		}
	}
	mineCount = m_mines.size();

	// Generate supplies
	while (m_barrels.size() < barrelCount) {
		int x = 1 + m_random.nextInt(MAP_WIDTH - 2);
		int y = 1 + m_random.nextInt(MAP_HEIGHT / 2);
		int h = MIN_RUM_BARREL_VALUE + m_random.nextInt(1 + MAX_RUM_BARREL_VALUE - MIN_RUM_BARREL_VALUE);

		RumBarrel m = RumBarrel(id++, x, y, h);
		if (cellIsFreeOfShips(m.m_position) && cellIsFreeOfMines(m.m_position) && cellIsFreeOfBarrels(m.m_position)) {
			if (y != MAP_HEIGHT - 1 - y) {
				m_barrels.push_back(RumBarrel(id++, x, MAP_HEIGHT - 1 - y, h));
			}
			m_barrels.push_back(m);
		}
	}
	barrelCount = m_barrels.size();

	// output
#ifdef DEBUG
	output(0);
#endif
};
int CodersOfTheCaribbean::run() {
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
			//cerr << "INPUT: " << input << endl;
			threads[i] = async(&Networking::handleTurnNetworking, &m_networking, i, turn, TURN_TIMEOUT, input, &outputs[i]);
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
		for (size_t i = 0; i < botCount; i++) {
			cerr << outputs[i] << endl;
		}
#endif

		high_resolution_clock::time_point start = high_resolution_clock::now();
		updateGame(outputs);
		time =+ duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

		// output
#ifdef DEBUG
		output(turn+1);
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

	cerr << std::setw(10) << std::fixed << std::setprecision(6) << times[0] / (turn + 1) * 1000 << "ms" << endl;
	for (size_t i = 1; i < 11; i++) {
		cerr << std::setw(10) << std::fixed << std::setprecision(6) << (times[i]-times[i-1]) / (turn + 1) * 1000 << "ms" << endl;
	}
	cerr << std::setw(10) << std::fixed << std::setprecision(6) << time / (turn + 1) * 1000 << "ms, turns " << turn << endl;

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
CodersOfTheCaribbean::~CodersOfTheCaribbean() {

}