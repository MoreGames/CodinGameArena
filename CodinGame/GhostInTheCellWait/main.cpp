#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

/**
* Auto-generated code below aims at helping you parse
* the standard input according to the problem statement.
**/
int main()
{
	int factoryCount; // the number of factories
	cin >> factoryCount; cin.ignore();
	int linkCount; // the number of links between factories
	cin >> linkCount; cin.ignore();
	for (int i = 0; i < linkCount; i++) {
		int factory1;
		int factory2;
		int distance;
		cin >> factory1 >> factory2 >> distance; cin.ignore();
	}

	// game loop
	while (1) {
		int entityCount; // the number of entities (e.g. factories and troops)
		cin >> entityCount; cin.ignore();
		for (int i = 0; i < entityCount; i++) {
			int entityId;
			string entityType;
			int arg1;
			int arg2;
			int arg3;
			int arg4;
			int arg5;
			cin >> entityId >> entityType >> arg1 >> arg2 >> arg3 >> arg4 >> arg5; cin.ignore();
		}

		// Write an action using cout. DON'T FORGET THE "<< endl"
		// To debug: cerr << "Debug messages..." << endl;


		// Any valid action, such as "WAIT" or "MOVE source destination cyborgs"
		cout << "WAIT" << endl;
	}
}