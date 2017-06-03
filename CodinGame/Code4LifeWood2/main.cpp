#ifndef _MSC_VER
#pragma GCC optimize("-O3")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops")
#endif

#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>

using namespace std;
using namespace std::chrono;

#define PROFILE
#define DEBUG
#ifdef DEBUG
//#define DEBUGFILE
#ifdef DEBUGFILE
FILE* debugFile = fopen("debugOutput.txt", "w");
#endif
#endif
//#define STATS

high_resolution_clock::time_point start;
#define NOW high_resolution_clock::now()
#define TIME duration_cast<duration<double>>(NOW - start).count()
double times[10] = { 0 };

constexpr int MAX_SAMPLES{ 50 };
constexpr char MOLECULE[5] = { 'A', 'B', 'C', 'D', 'E' };

int available[5] = { 0 };
struct Entity {
	int sampleId;
	int carriedBy;
	int rank;
	string expertiseGain;
	int health;
	int cost[5];

	void print() {
#ifdef DEBUG
#ifdef DEBUGFILE
		fprintf(debugFile, "sampleId:%d, carriedBy:%d, rank:%d, expertiseGain:%s, health:%d, cost:%d,%d,%d,%d,%d\n", sampleId, carriedBy, rank, expertiseGain.c_str(), health, cost[0], cost[1], cost[2], cost[3], cost[4]);
#else
		fprintf(stderr, "sampleId:%d, carBy:%d, rank:%d, expGain:%s, health:%d, cost:%d,%d,%d,%d,%d\n", sampleId, carriedBy, rank, expertiseGain.c_str(), health, cost[0], cost[1], cost[2], cost[3], cost[4]);
#endif
#endif
	}
};
array<Entity, 59> entities;

struct bot {
	string target;
	int eta;
	int score;
	int storage[5];
	int expertise[5];

	Entity* samples[3];
	int countSamples() {
		int count = 0;
		if (samples[0] != nullptr) count++;
		if (samples[1] != nullptr) count++;
		if (samples[2] != nullptr) count++;
		return count;
	}
	bool missingMolecules(int* missing) {
		int storageTemp[5];
		memcpy(&storageTemp, &storage, sizeof(storage));

		for (size_t j = 0; j < 5; j++) {
			missing[j] = 0;
		}

		for (size_t i = 0; i < 3; i++) {
			if (samples[i] != nullptr) {
				for (size_t j = 0; j < 5; j++) {
					missing[j] += samples[i]->cost[j];
				}
			}
		}

		int sum = 0;
		for (size_t j = 0; j < 5; j++) {
			missing[j] -= storage[j];
			sum += missing[j];
		}

		if (sum == 0) {
			return false;
		}
		return true;
	}

	void print() {
#ifdef DEBUG
#ifdef DEBUGFILE
		fprintf(debugFile, "target:%s, eta:%d, score:%d, storage:%d,%d,%d,%d,%d, expertise:%d,%d,%d,%d,%d\n", target.c_str(), eta, score, storage[0], storage[1], storage[2], storage[3], storage[4], expertise[0], expertise[1], expertise[2], expertise[3], expertise[4]);
#else
		fprintf(stderr, "target:%s, eta:%d, score:%d, storage:%d,%d,%d,%d,%d, expertise:%d,%d,%d,%d,%d\n", target.c_str(), eta, score, storage[0], storage[1], storage[2], storage[3], storage[4], expertise[0], expertise[1], expertise[2], expertise[3], expertise[4]);
#endif
#endif
	}
} myBot, enemyBot;

void printAvailable() {
#ifdef DEBUG
#ifdef DEBUGFILE
	fprintf(debugFile, "available:%d,%d,%d,%d,%d\n", available[0], available[1], available[2], available[3], available[4]);
#else
	fprintf(stderr, "available:%d,%d,%d,%d,%d\n", available[0], available[1], available[2], available[3], available[4]);
#endif
#endif

}

/**
* Bring data on patient samples from the diagnosis machine to the laboratory with enough molecules to produce medicine!
**/
int main()
{
	int projectCount;
	cin >> projectCount; cin.ignore();
	for (int i = 0; i < projectCount; i++) {
		int a, b, c, d, e;
		cin >> a >> b >> c >> d >> e; cin.ignore();
	}

	// game loop
	while (1) {
		cin >> myBot.target >> myBot.eta >> myBot.score >>
			myBot.storage[0] >> myBot.storage[1] >> myBot.storage[2] >> myBot.storage[3] >> myBot.storage[4] >>
			myBot.expertise[0] >> myBot.expertise[1] >> myBot.expertise[2] >> myBot.expertise[3] >> myBot.expertise[4];
		cin.ignore();
		myBot.print();
		cin >> enemyBot.target >> enemyBot.eta >> enemyBot.score >>
			enemyBot.storage[0] >> enemyBot.storage[1] >> enemyBot.storage[2] >> enemyBot.storage[3] >> enemyBot.storage[4] >>
			enemyBot.expertise[0] >> enemyBot.expertise[1] >> enemyBot.expertise[2] >> enemyBot.expertise[3] >> enemyBot.expertise[4];
		cin.ignore();
		enemyBot.print();
		cin >> available[0] >> available[1] >> available[2] >> available[3] >> available[4]; cin.ignore();
		printAvailable();
		int sampleCount;
		cin >> sampleCount; cin.ignore();
		if (sampleCount > MAX_SAMPLES) {
			cerr << "ERROR" << endl;
		}

		myBot.samples[0] = nullptr;
		myBot.samples[1] = nullptr;
		myBot.samples[2] = nullptr;
		int counter = 0;
		for (int i = 0; i < sampleCount; i++) {
			cin >> entities[i].sampleId >> entities[i].carriedBy >> entities[i].rank >> entities[i].expertiseGain >> entities[i].health >> entities[i].cost[0] >> entities[i].cost[1] >> entities[i].cost[2] >> entities[i].cost[3] >> entities[i].cost[4]; cin.ignore();
			if (entities[i].carriedBy == 0) {
				myBot.samples[counter++] = &entities[i];
			}
			entities[i].print();
		}

		int missing[5] = { 0, 0, 0, 0, 0 };
		if (myBot.target.compare("LABORATORY") == 0) {
			if (myBot.countSamples() > 0) {
				int id = 0;
				if (myBot.samples[0] != nullptr) id = myBot.samples[0]->sampleId;
				if (myBot.samples[1] != nullptr) id = myBot.samples[1]->sampleId;
				if (myBot.samples[2] != nullptr) id = myBot.samples[2]->sampleId;
				cout << "CONNECT " << id << endl;
			} else {
				cout << "GOTO DIAGNOSIS" << endl;
			}
		} else if (myBot.target.compare("MOLECULES") == 0) {
			if (myBot.missingMolecules(missing)) {
				for (size_t j = 0; j < 5; j++) {
					if (missing[j] > 0) {
						cout << "CONNECT " << MOLECULE[j] << endl;
						break;
					}
				}
			} else {
				cout << "GOTO LABORATORY" << endl;
			}
		} else if (myBot.target.compare("DIAGNOSIS") == 0) {
			if (myBot.countSamples() < 2) {
				sort(entities.begin(), entities.end(), [](const Entity& a, const Entity& b) -> bool {
					if (a.carriedBy == b.carriedBy) {
						return a.health > b.health;
					} else {
						return a.carriedBy < b.carriedBy;
					}
				});
				cout << "CONNECT " << entities[0].sampleId << endl;
			} else {
				cout << "GOTO MOLECULES" << endl;
			}
		} else {
			cout << "GOTO DIAGNOSIS" << endl;
		}
	}
}