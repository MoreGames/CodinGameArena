#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

constexpr int GOT_PUSHED = 2;
constexpr int DID_PUSH = 1;
constexpr int NO_PUSH = 0;
constexpr int FINAL_HEIGHT = 4;
constexpr int VIEW_DISTANCE = 1;
constexpr int GENERATED_MAP_SIZE = 6;
constexpr bool WIN_ON_MAX_HEIGHT = true;
constexpr bool FOG_OF_WAR = false;
constexpr bool CAN_PUSH = false;
constexpr int UNITS_PER_PLAYER = 1;

constexpr int MAX_SIZE = 7;
constexpr int MAX_UNITS_PER_PLAYER = 2;

// size: 4 * (490+4+4+1+1) = 2000 bytes
struct state {
	// cell numbering
    // x0,y0 x1,y0 x2,y0 ...
    // x0,y1 1,y1, x2,y1 ...
    // x0,y2 ...
	int board[MAX_SIZE*MAX_SIZE][10]; // level, size of neighbours, neigbour index (N, NE, E, SE, S, SW, W, NW)
	int me[MAX_UNITS_PER_PLAYER][2];
	int other[MAX_UNITS_PER_PLAYER][2];
	int size;
	int unitsPerPlayer;
};

void printState(state& s) {
	fprintf(stderr, "board:\n");
	fprintf(stderr, "  ");
	for (size_t i = 0; i < s.size; i++) {
		fprintf(stderr, "| %2d ", i);
	}
	fprintf(stderr, "|\n");
	fprintf(stderr, "---");
	for (size_t i = 0; i < s.size; i++) {
		fprintf(stderr, "-----");
	}
	fprintf(stderr, "-\n");
	for (size_t y = 0; y < s.size; y++) {
		fprintf(stderr, "%d ", y);
		for (size_t x = 0; x < s.size; x++) {
			int index = y*s.size + x;
			fprintf(stderr, "| %2d ", s.board[index][0]);
		}
		fprintf(stderr, "|\n");
		
		fprintf(stderr, "---");
		for (size_t x = 0; x < s.size; x++) {
			fprintf(stderr, "-----");
		}
		fprintf(stderr, "-\n");
	}
	
	for (size_t index = 0; index < s.size*s.size; index++) {
		fprintf(stderr, "%2d|", index);
		for (size_t i = 0; i < 10; i++) {
			fprintf(stderr, "%2d ", s.board[index][i]);
		}
		fprintf(stderr, "\n");
	}
	
	fprintf(stderr, "players:\n");
	for (size_t i = 0; i < s.unitsPerPlayer; i++) {
		fprintf(stderr, "me:%2d %2d\n", s.me[i][0], s.me[i][1]);
	}
	for (size_t i = 0; i < s.unitsPerPlayer; i++) {
		fprintf(stderr, "other:%2d %2d\n", s.other[i][0], s.other[i][1]);
	}
}
void removeCell(state& s, int index) {
	// update all neighbours
	for (size_t i = 2; i < 10; i++) {
		int n = s.board[index][i];
		//fprintf(stderr, "index:%2d n:%2d\n", index, n);
		if (n != -1) {
			for (size_t j = 2; j < 10; j++) {
				if (s.board[n][j] == index) {
					s.board[n][j] = -1;
					s.board[n][1] -= 1;
					break;
				}
			}
		}
	}
	
	// remove cell
	for (size_t i = 0; i < 10; i++) {
		s.board[index][i] = -1;
	}
	s.board[index][1] = 0;
}
void removeCell(state& s, int x, int y) {
	int index = y*s.size + x;
	//fprintf(stderr, "remove index:%2d %2d,%2d\n", index, x, y);
	removeCell(s, index);
}
string direction(int pos, int tar, int size) {
	int diff = pos-tar;
	if (diff == size + 1) return "NW";
	if (diff == 1) return "W";
	if (diff == -size+1) return "SW";
	if (diff == -size) return "S";
	if (diff == -size-1) return "SE";
	if (diff == -1) return "E";
	if (diff == size-1) return "NE";
	if (diff == size) return "N";
}

int main() {
	state base;
	
	// default state
    for (size_t i = 0; i < MAX_SIZE*MAX_SIZE; i++) {
        for (int j = 0; j < 10; j++) {
            base.board[i][j] = -1;   
        }
		base.board[i][1] = 0; 
    }
	for (size_t i = 0; i < MAX_UNITS_PER_PLAYER; i++) {
		base.me[i][0] = -1;
		base.me[i][1] = -1;
		base.other[i][0] = -1;
		base.other[i][1] = -1;
	}
	base.size = 0;
	
	// init input
    int size;
    cin >> size; cin.ignore();
    int unitsPerPlayer;
    cin >> unitsPerPlayer; cin.ignore();
	fprintf(stderr, "size:%d, unitsPerPlayer:%d\n", size, unitsPerPlayer);

	// init state
	base.size = size;
	base.unitsPerPlayer = unitsPerPlayer;
    for (size_t index = 0; index < size*size; index++) {
        base.board[index][0] = 0;
        
        int y = index / size;
        int x = index % size;
        
        int n = 0;
        if (x > 0 && y > 0) { // NW
            base.board[index][9] = index - size - 1;
            n++;
        }
        if (x > 0) { // W
            base.board[index][8] = index - 1;
            n++;
        }
        if (x > 0 && y < size-1) { // SW
            base.board[index][7] = index + size - 1;
            n++;
        }
        if (y < size-1) { // S
            base.board[index][6] = index + size;
            n++;
        }
		
        if (x < size-1 && y < size-1) { // SE
            base.board[index][5] = index + size + 1;
            n++;
        }
        if (x < size-1) { // E
            base.board[index][4] = index + 1;
            n++;
        }
        if (x < size-1 && y > 0) { // NE
            base.board[index][3] = index - size + 1;
            n++;
        }
        if (y > 0) { // N
            base.board[index][2] = index - size;
            n++;
        }
		    
        base.board[index][1] = n; 
		// eigentlich sinnlos, weil sowieso immer alle 8 Richtungen auf Gültigkeit abgefragt werden müssen
		// enthält somit nur die Information, ob nicht alle Richtungen möglich sind
    }
	
	int oldPos, newPos;
	string oldDir, newDir;
    int turn = 0;
    // game loop
    while (1) {
        for (int y = 0; y < size; y++) {
            string row;
            cin >> row; cin.ignore();
		    for (int x = 0; x < row.size(); x++) {
		        if (row[x] == '.')
				removeCell(base, x, y);
		    }
        }
        
        for (int i = 0; i < unitsPerPlayer; i++) {
            int unitX;
            int unitY;
            cin >> unitX >> unitY; cin.ignore();
			base.me[i][0] = unitX;
            base.me[i][1] = unitY;
			
            fprintf(stderr, "me:%d %d\n", unitX, unitY);
        }
        for (int i = 0; i < unitsPerPlayer; i++) {
            int otherX;
            int otherY;
            cin >> otherX >> otherY; cin.ignore();
            base.other[i][0] = otherX;
            base.other[i][1] = otherY;
			
            fprintf(stderr, "enemy:%d %d\n", otherX, otherY);
        }
        int legalActions;
        cin >> legalActions; cin.ignore();
        for (int i = 0; i < legalActions; i++) {
            string atype;
            int index;
            string dir1;
            string dir2;
            cin >> atype >> index >> dir1 >> dir2; cin.ignore();
            
            fprintf(stderr, "actions:%s %d %s %s\n", atype.c_str(), index, dir1.c_str(), dir2.c_str());
        }
		
		printState(base);

        // Write an action using cout. DON'T FORGET THE "<< endl"
        // To debug: cerr << "Debug messages..." << endl;

		// find neigbour
		if (turn == 0) {
			oldPos = base.me[0][0] + base.size*base.me[0][1];
			for (size_t i = 2; i < 10; i++) {
				if (base.board[oldPos][i] != -1) {
					newPos = base.board[oldPos][i];
				}
			}
			oldDir = direction(oldPos, newPos, size);
			newDir = direction(newPos, oldPos, size);
		}
		
		if (turn % 2 == 0) {
			cout << "MOVE&BUILD 0 " << oldDir <<  " " << newDir << endl;
		} else {
			cout << "MOVE&BUILD 0 " << newDir <<  " " << oldDir << endl;
		}
		
		turn++;
    }
}