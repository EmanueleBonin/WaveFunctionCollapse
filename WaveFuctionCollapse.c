#include <Windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>

/*
	0	1	2	3	4	5	6	7	8	9	A	B	C	D	E	F
U+250x	─	━	│	┃	┄	┅	┆	┇	┈	┉	┊	┋	┌	┍	┎	┏
U+251x	┐	┑	┒	┓	└	┕	┖	┗	┘	┙	┚	┛	├	┝	┞	┟
U+252x	┠	┡	┢	┣	┤	┥	┦	┧	┨	┩	┪	┫	┬	┭	┮	┯
U+253x	┰	┱	┲	┳	┴	┵	┶	┷	┸	┹	┺	┻	┼	┽	┾	┿
U+254x	╀	╁	╂	╃	╄	╅	╆	╇	╈	╉	╊	╋	╌	╍	╎	╏
U+255x	═	║	╒	╓	╔	╕	╖	╗	╘	╙	╚	╛	╜	╝	╞	╟
U+256x	╠	╡	╢	╣	╤	╥	╦	╧	╨	╩	╪	╫	╬	╭	╮	╯
U+257x	╰	╱	╲	╳	╴	╵	╶	╷	╸	╹	╺	╻	╼	╽	╾	╿
*/



wchar_t* Sy = L"╭╮╰╯┌┐└┘─│┴┬┤├┼*";

#define ULCURVE  0
#define URCURVE  1
#define BLCURVE  2
#define BRCURVE  3
#define ULANGLE  4
#define URANGLE  5
#define BLANGLE  6
#define BRANGLE  7
#define HLINE    8
#define VLINE    9
#define UTJUNC  10
#define BTJUNC  11
#define LTJUNC  12
#define RTJUNC  13
#define CROSS   14
#define BOUND   15



#define B_ULCURVE      1
#define B_URCURVE      2
#define B_BLCURVE      4
#define B_BRCURVE      8
#define B_ULANGLE     16
#define B_URANGLE     32
#define B_BLANGLE     64
#define B_BRANGLE    128
#define B_HLINE      256
#define B_VLINE      512
#define B_UTJUNC    1024
#define B_BTJUNC    2048
#define B_LTJUNC    4096
#define B_RTJUNC    8192
#define B_CROSS    16384

#define B_ALL B_ULCURVE | B_URCURVE | B_BLCURVE | B_BRCURVE | B_ULANGLE | B_URANGLE | B_BLANGLE | B_BRANGLE | B_HLINE | B_VLINE | B_UTJUNC | B_BTJUNC | B_LTJUNC | B_RTJUNC | B_CROSS 


#define UPVOID	B_BLCURVE | B_BRCURVE | B_BLANGLE | B_BRANGLE | B_HLINE | B_UTJUNC 
#define RGVOID	B_ULCURVE | B_BLCURVE | B_ULANGLE | B_BLANGLE | B_VLINE | B_RTJUNC 
#define DWVOID	B_ULCURVE | B_URCURVE | B_ULANGLE | B_URANGLE | B_HLINE | B_BTJUNC 
#define LFVOID	B_URCURVE | B_BRCURVE | B_URANGLE | B_BRANGLE | B_LTJUNC

#define UPJUNC	B_ULCURVE | B_URCURVE | B_ULANGLE | B_URANGLE | B_VLINE | B_BTJUNC | B_LTJUNC | B_RTJUNC | B_CROSS
#define RGJUNC	B_URCURVE | B_BRCURVE | B_BRANGLE | B_URANGLE | B_HLINE | B_UTJUNC | B_BTJUNC | B_LTJUNC | B_CROSS
#define DWJUNC	B_BLCURVE | B_BRCURVE | B_BLANGLE | B_BRANGLE | B_VLINE | B_UTJUNC | B_LTJUNC | B_RTJUNC | B_CROSS
#define LFJUNC	B_ULCURVE | B_BLCURVE | B_ULANGLE | B_BLANGLE | B_HLINE | B_UTJUNC | B_BTJUNC | B_RTJUNC | B_CROSS

#define MAXTRYWFC 10

// Rules definition
// Strat Top clock-wise direction
// 1st Dim = cell
// 2nd Dim = eligible joints bitmap

int Rules[16][4] =
{   //   UP   RIGHT   DOWN    LEFT
	// ULCURVE
	{UPVOID, RGJUNC, DWJUNC, LFVOID},
	// URCURVE
   {UPVOID, RGVOID, DWJUNC, LFJUNC},
   // BLCURVE
   {UPJUNC, RGJUNC, DWVOID, LFVOID},
   // BRCURVE
   {UPJUNC, RGVOID, DWVOID, LFJUNC},
   // ULANGLE
   {UPVOID, RGJUNC, DWJUNC, LFVOID},
   // URANGLE
  {UPVOID, RGVOID, DWJUNC, LFJUNC},
  // BLANGLE
  {UPJUNC, RGJUNC, DWVOID, LFVOID},
  // BRANGLE
  {UPJUNC, RGVOID, DWVOID, LFJUNC},
  // HLINE
  {UPVOID, RGJUNC, DWVOID, LFJUNC},
  // VLINE
  {UPJUNC, RGVOID, DWJUNC, LFVOID},
  // UTJUNC
  {UPJUNC, RGJUNC, DWVOID, LFJUNC},
  // BTJUNC
  {UPVOID, RGJUNC, DWJUNC, LFJUNC},
  // LTJUNC
  {UPJUNC, RGVOID, DWJUNC, LFJUNC},
  // RTJUNC
  {UPJUNC, RGJUNC, DWJUNC, LFVOID},
  // CROSS
  {UPJUNC, RGJUNC, DWJUNC, LFJUNC}
};



#define ROWS	6
#define COLS	12
// 2 Columns/Rows added to avoid the check of the out of bound
#define RMAX	ROWS+2 
#define CMAX	COLS+2


int M[RMAX][CMAX] = { 0 };
int dx[4] = { 0, 1, 0,-1 };
int dy[4] = { -1, 0, 1, 0 };

void InitMatrix() {
	int c;
	for (int i = 0; i < CMAX; ++i) {
		M[0][i] = -2;
		M[RMAX - 1][i] = -2;
	}
	for (int j = 0; j < RMAX; ++j) {
		M[j][0] = -2;
		M[j][CMAX - 1] = -2;
	}
	for (int i = 1; i < RMAX - 1; ++i) {
		for (int j = 1; j < CMAX - 1; ++j) {
			M[i][j] = -1;
		}
	}
}

void Initialize() {
	// Configurazione completa della console
	_setmode(_fileno(stdout), _O_U16TEXT);	// Modalità UTF-16
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);


	//Init random generator
	srand(time(NULL));

}


void PrintMatrix() {
	int c;
	wprintf(L"\x1b[;H"); //<--- Go Home 
	for (int i = 0; i < RMAX; ++i) {
		for (int j = 0; j < CMAX; ++j) {
			c = M[i][j];
			if (c == -2)
				wprintf(L"%c", *(Sy + BOUND));
			else if (c == -1)
				wprintf(L" ");


		}
		wprintf(L"\n");
	}

}

int GetTiles(int* Tiles, int* Count, int x, int y) {
	int Eligibles = B_ALL, Cell, Rule, Power = 1;
	// Loop Neighborhood
	for (int i = 0; i < 4; ++i) {
		Cell = M[y + dy[i]][x + dx[i]];
		if (Cell >= 0) {
			// Take the rules of neighboor face ...
			Rule = Rules[Cell][(i + 2) % 4];
			// ... AND-ing it with other eligibles
			Eligibles = Eligibles & Rule;
		}
	}
	*Count = 0;
	if (Eligibles != 0) {
		// count the number of eligible cells
		for (int i = 0; i < BOUND; ++i) {
			if (Eligibles & Power) Tiles[(*Count)++] = i;
			Power = Power << 1; // * 2
		}
	}
	return Eligibles;
}

int PlaceXY(int Counter, int x, int y) {
	int Eligibles, Count = 0, TryCount = 0, Placed = 0, r, rfrom, xnew, ynew, NeighborPlaced;
	int Tiles[BOUND]; // Array that contains indexes to the eligible tile.
	while (TryCount < MAXTRYWFC && Placed == 0) {
		Eligibles = GetTiles(Tiles, &Count, x, y);
		// There are some eligiblles cells
		if (Eligibles != 0) {
			rfrom = rand() % 4; // random Start position ClockWise
			//rfrom = 0; // fixed 0 seem to be quicker with less enjoy ...
			r = rand() % Count;
			
			M[y][x] = Tiles[r];
			// Print cell at x,y (VT100)
			wprintf(L"\x1b[%d;%dH%c", y + 1, x + 1, *(Sy + Tiles[r]));
			Placed = 1;
			if (Counter + 1 < ROWS * COLS) {
				NeighborPlaced = 1;
				for (int i = 0; i < 4 && NeighborPlaced != 0; ++i) {
					xnew = x + dx[(i + rfrom) % 4]; ynew = y + dy[(i + rfrom) % 4];
					if (M[ynew][xnew] == -1) NeighborPlaced = PlaceXY(++Counter, xnew, ynew);
				}
				if (NeighborPlaced == 0) {
					//Back Track
					Placed = 0;
					M[y][x] = -1;
					wprintf(L"\x1b[%d;%dH%c", y + 1, x + 1, L' ');
				} // Cell Placed and his neighbors are all ok
			} //else Last Cell Placed!
		}
		TryCount++;
	}
	return Placed;
}


void hidecursor() {
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO info;
	info.dwSize = 100;
	info.bVisible = FALSE;
	SetConsoleCursorInfo(consoleHandle, &info);
}

int main() {
	int c = 0, startx, starty;
	clock_t start,end;
	double tempo;

	getch();

	Initialize();

	hidecursor();
	
	// while ESC is not pressed, continue!
	while (c != 27) {
		system("cls"); // system("clear"); // LINUX
		InitMatrix();
		// here it used to print the boundary
		PrintMatrix();
		startx = (rand() % COLS) + 1;
		starty = (rand() % ROWS) + 1;

		start=clock();
		PlaceXY(0, startx, starty);
		end=clock();
		tempo=((double)(end-start))/CLOCKS_PER_SEC;
		wprintf(L"\x1b[%d;%dHTime sec.: %f", RMAX + 1, 0, tempo);
		wprintf(L"\x1b[%d;%dHStart x,y: %d, %d", RMAX + 2, 0, startx, starty);
		c = getch();
	}



	return 0;
}


