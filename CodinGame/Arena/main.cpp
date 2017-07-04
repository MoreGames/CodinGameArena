#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

//#include "GhostInTheCell.hpp" // identical defines, can't include multiple games
//#include "CodersOfTheCaribbean.hpp"  // identical defines, can't include multiple games
//#include "Code4Life.hpp" // identical defines, can't include multiple games
#include "WondevWoman.hpp"

using namespace std;

/*
int main() {
	vector<string> botNames(0);
	botNames.push_back("D:\\CodinGame\\Release\\GhostInTheCellWait.exe");
	botNames.push_back("D:\\CodinGame\\Release\\GhostInTheCellWood3.exe");
	long long seed = 7441271002587938299;

	int points[] = { 0,0,0 };
	size_t gameCount = 1;
	for (size_t i = 0; i < gameCount; i++) {
		GhostInTheCell game(botNames, seed);
		int winner{ game.run() };
		if (winner == -1) {
			points[0] += 1;
		} else if (winner == 0) {
			points[1] += 1;
		} else {
			points[2] += 1;
		}
	}
	printf("#games: %d\n", gameCount);
	printf("draws:%5d bot1:%5d bot2:%5d\n", points[0], points[1], points[2]);
	printf("draws:%4.1f%% bot1:%4.1f%% bot2:%4.1f%%\n", points[0] * 100. / gameCount, points[1] * 100. / gameCount, points[2] * 100. / gameCount);

	return 0;
}
*/

/*
// hardcoded main (seed, bots)
int main(int argc, char **argv) {
	vector<string> botNames(0);
	botNames.push_back("D:\\CodinGame\\Debug\\CodersOfTheCaribbeanWood3.exe");
	botNames.push_back("D:\\CodinGame\\Debug\\CodersOfTheCaribbeanWait.exe");
	long long seed = 493625741;
	size_t mineCount = 8;
	size_t barrelCount = 15;
	size_t shipsPerPlayer = 3;

	int points[] = { 0,0,0 };
	size_t gameCount = 1;
	for (size_t i = 0; i < gameCount; i++) {
		CodersOfTheCaribbean game(botNames, seed, mineCount, barrelCount, shipsPerPlayer);
		int winner{ game.run() };
		if (winner == -1) {
			points[0] += 1;
		} else if (winner == 0) {
			points[1] += 1;
		} else {
			points[2] += 1;
		}
	}
	printf("#games: %d\n", gameCount);
	
	/*intf("draws:%5d bot1:%5d bot2:%5d\n", points[0], points[1], points[2]);
	printf("draws:%4.1f%% bot1:%4.1f%% bot2:%4.1f%%\n", points[0] * 100. / gameCount, points[1] * 100. / gameCount, points[2] * 100. / gameCount);

	return 0;
}
*/
/*
// hardcoded main (seed, bots)
int main(int argc, char **argv) {
	vector<string> botNames(0);
	botNames.push_back("D:\\CodinGame\\Debug\\Code4LifeWood2.exe");
	botNames.push_back("D:\\CodinGame\\Debug\\Code4LifeWait.exe");
	long long seed = 647466733;

	int points[] = { 0,0,0 };
	size_t gameCount = 1;
	for (size_t i = 0; i < gameCount; i++) {
		Code4Life game(botNames, seed);
		int winner{ game.run() };
		if (winner == -1) {
			points[0] += 1;
		} else if (winner == 0) {
			points[1] += 1;
		} else {
			points[2] += 1;
		}
	}
	printf("#games: %d\n", gameCount);
	printf("draws:%5d bot1:%5d bot2:%5d\n", points[0], points[1], points[2]);
	printf("draws:%4.1f%% bot1:%4.1f%% bot2:%4.1f%%\n", points[0] * 100. / gameCount, points[1] * 100. / gameCount, points[2] * 100. / gameCount);

	return 0;
}
*/

// hardcoded main (seed, bots)
int main(int argc, char **argv) {
	vector<string> botNames(0);
	botNames.push_back("J:\\CodinGame\\x64\\Debug\\WondevWomanWood2.exe");
	botNames.push_back("J:\\CodinGame\\x64\\Debug\\WondevWomanWood3.exe");
	int mapIndex = 2;
	long long seed = 675329364;
	bool symmetric = false;

	int points[] = { 0,0,0 };
	size_t gameCount = 1;
	for (size_t i = 0; i < gameCount; i++) {
		WondevWoman game(botNames, mapIndex, seed, symmetric);
		int winner{ game.run() };
		if (winner == -1) {
			points[0] += 1;
		} else if (winner == 0) {
			points[1] += 1;
		} else {
			points[2] += 1;
		}
	}
	printf("#games: %d\n", gameCount);
	printf("draws:%5d bot1:%5d bot2:%5d\n", points[0], points[1], points[2]);
	printf("draws:%4.1f%% bot1:%4.1f%% bot2:%4.1f%%\n", points[0] * 100. / gameCount, points[1] * 100. / gameCount, points[2] * 100. / gameCount);

	return 0;
}
