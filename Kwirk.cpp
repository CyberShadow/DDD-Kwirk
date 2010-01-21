// Kwirk DDD module
// Configuration:
// LEVEL - sets the level to solve, from 0 (1-1) to 29 (3-10)

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include STRINGIZE(Levels/LEVEL.h)

#define CELL_HOLE         0x80

#define CELL_MASK         0x60
#define CELL_EMPTY        0x00
#define CELL_BLOCK        0x20 // index contains block index
#define CELL_ROTATOR      0x40 // rotator blades - index contains the direction
#define CELL_WALL         0x60 // also rotator center and inactive players - index contains rotator index
#define CELL_EXIT         0xE0 // wall+hole

#define INDEX_MASK        0x1F

enum Action
#ifndef __GNUC__
 : uint8_t
#endif
{
	UP,
	RIGHT,
	DOWN,
	LEFT,
	SWITCH,
	NONE,
	
	ACTION_FIRST=UP,
	ACTION_LAST =SWITCH
};

inline Action operator++(Action &rs, int) {return rs = (Action)(rs + 1);}
const char* actionNames[] = {"Up", "Right", "Down", "Left", "Switch", "None"};

enum
{
	DELAY_MOVE         =  9, // 1+8
	DELAY_PUSH         = 10, // 2+8
	DELAY_FILL         = 26,
	DELAY_ROTATE       = 12,
	DELAY_SWITCH       = 30,
	DELAY_SWITCH_AGAIN = 32,
	DELAY_EXIT         =  1, // fake delay to prevent grouping into one frame group
};

const char DX[4] = {0, 1, 0, -1};
const char DY[4] = {-1, 0, 1, 0};
const char DR[4+1] = "^>`<";

struct Player
{
	uint8_t x, y;
	
	INLINE void set(int x, int y) { this->x = x; this->y = y; }
	INLINE bool exited() const { return x==EXIT_X && y==EXIT_Y; }
};

struct Block
{
	uint8_t w, h;
};

typedef uint8_t Map[Y][X];

struct State
{
	const static State initial;
	const static Map blanked;
	const static uint8_t holeIndices[Y][X];
	const static Block blocks[BLOCKS];

	Map map;
	Player players[PLAYERS];
	
#if (PLAYERS==1)
	enum { activePlayer = 0 };
#else
	uint8_t activePlayer;
#endif
#if (PLAYERS>2)
	bool justSwitched;
#endif
	
	CompressedState compressed;
#ifdef DEBUG
	bool compressedUpdated;
	bool uncompressedUpdated;
#endif

	/// Returns frame delay, 0 if move is invalid and the state was altered, -1 if move is invalid and the state was not altered
	template<bool UPDATE_UNCOMPRESSED, bool UPDATE_COMPRESSED>
	int perform(Action action)
#ifdef DEBUG
	{
		State s1 = *this;

		if (compressedUpdated && uncompressedUpdated)
		{
			State s2;
			s2.decompress(&compressed);
			if (!(s1 == s2))
			{
				printf("Initial state:\n%s%s\n%s\n", s1.toString(), s1.compressed.toString(), hexDump(&s1, sizeof(s1), X));
				printf("After decompressing:\n%s%s\n\n", s2.toString(), hexDump(&s2, sizeof(s2), X));
				error("Decompression failure");
			}
		}

		int r = realPerform<UPDATE_UNCOMPRESSED, UPDATE_COMPRESSED>(action);

		if (UPDATE_COMPRESSED)
		{
			State s2 = s1;
			if (r>0)
			{
				s2.realPerform<true, false>(action);
				State s3;
				s3.decompress(&compressed);
				if (!(s2 == s3))
				{
					printf("Initial state:\n%s%s\n%s\n", s1.toString(), s1.compressed.toString(), hexDump(&s1, sizeof(s1), X));
					printf("Action: %s\n\n", actionNames[action]);
					printf("After compressed perform:\n%s\n%s\n\n", compressed.toString(), hexDump(&compressed, sizeof(compressed)));
					printf("After decompressing:\n%s%s\n\n", s3.toString(), hexDump(&s3, sizeof(s3), X));
					printf("After uncompressed perform:\n%s%s\n\n", s2.toString(), hexDump(&s2, sizeof(s2), X));
					error("Compressed/decompressed perform result mismatch");
				}
			}
		}
		
		if (r >= 0)
		{
			if (!UPDATE_UNCOMPRESSED)
				uncompressedUpdated = false;
			if (!UPDATE_COMPRESSED)
				compressedUpdated = false;
		}
		else // not changed
		{
			if (!(*this == s1))
			{
				printf("Initial state:\n%s%s\n%s\n", s1.toString(), s1.compressed.toString(), hexDump(&s1, sizeof(s1)));
				printf("Action: %s\n\n", actionNames[action]);
				printf("Current state:\n%s\n%s\n\n", toString(), compressed.toString(), hexDump(this, sizeof(*this)));
				printf("Return value: %d\n", r);
				error("State was changed, contradicting return value!");
			}
			assert(compressed == s1.compressed);
		}
		
		return r;
	}

	template<bool UPDATE_UNCOMPRESSED, bool UPDATE_COMPRESSED>
	int realPerform(Action action)
#endif
	{
		assert(action <= ACTION_LAST);
		debug_assert(uncompressedUpdated);
		if (UPDATE_COMPRESSED)
			debug_assert(compressedUpdated);
		if (action == SWITCH)
		{
#if (PLAYERS==1)
			return -1;
#else
			if (playersLeft())
			{
				switchPlayers<UPDATE_UNCOMPRESSED, UPDATE_COMPRESSED>();
#if (PLAYERS>2)
				int res = justSwitched ? DELAY_SWITCH_AGAIN : DELAY_SWITCH;
				if (UPDATE_UNCOMPRESSED)
					justSwitched = true;
				if (UPDATE_COMPRESSED)
					compressed.justSwitched = true;
				return res;
#else
				return DELAY_SWITCH;
#endif
			}
			else
				return -1;
#endif
		}
		Player n, p = players[activePlayer];
		n.x = p.x + DX[action];
		n.y = p.y + DY[action];
		uint8_t dmap = map[n.y][n.x];
		if (dmap == CELL_EXIT)
		{
			players[activePlayer] = n;
			if (UPDATE_COMPRESSED)
				updatePlayer(EXIT_X, EXIT_Y);
			clearJustSwitched<UPDATE_UNCOMPRESSED, UPDATE_COMPRESSED>();
			if (playersLeft())
			{
				switchPlayers<UPDATE_UNCOMPRESSED, UPDATE_COMPRESSED>();
				return DELAY_MOVE + DELAY_SWITCH;
			}
			else
				return DELAY_MOVE + DELAY_EXIT;
		}
		uint8_t dcell = dmap & CELL_MASK;
		if (dcell == CELL_WALL || (dmap & CELL_HOLE)) // wall or hole
			return -1;
		if (dmap == 0)
		{
			if (UPDATE_UNCOMPRESSED)
				players[activePlayer] = n;
			if (UPDATE_COMPRESSED)
				updatePlayer(n.x, n.y);
			clearJustSwitched<UPDATE_UNCOMPRESSED, UPDATE_COMPRESSED>();
			return DELAY_MOVE;
		}
		uint8_t index = dmap & INDEX_MASK;
		if (dcell == CELL_BLOCK)
		{
#if (BLOCKS > 0)
			assert(dmap == (CELL_BLOCK | index));
			uint8_t x1=n.x, y1=n.y, x2=x1, y2=y1;
			// check if destination is free, clear source row/column
			switch (action)
			{
				case UP:
					do x1--; while ((map[y1][x1]&~CELL_HOLE)==dmap);
					x1++; x2=x1+blocks[index].w; // here be dragons
					      y1=y2-blocks[index].h;
					for (int x=x1; x<x2; x++) if (map[y1][x] & CELL_MASK) return -1;
					for (int x=x1; x<x2; x++) map[y1][x] |= dmap;
					for (int x=x1; x<x2; x++) map[n.y][x] &= CELL_HOLE;
					break;
				case DOWN:
					do x1--; while ((map[y1][x1]&~CELL_HOLE)==dmap);
					x1++; x2=x1+blocks[index].w;
					      y2=y1+blocks[index].h; y1++;
					for (int x=x1; x<x2; x++) if (map[y2][x] & CELL_MASK) return -1;
					for (int x=x1; x<x2; x++) map[y2][x] |= dmap;
					for (int x=x1; x<x2; x++) map[n.y][x] &= CELL_HOLE;
					y2++;
					break;
				case LEFT:
					do y1--; while ((map[y1][x1]&~CELL_HOLE)==dmap);
					y1++; y2=y1+blocks[index].h;
					      x1=x2-blocks[index].w;
					for (int y=y1; y<y2; y++) if (map[y][x1] & CELL_MASK) return -1;
					for (int y=y1; y<y2; y++) map[y][x1] |= dmap;
					for (int y=y1; y<y2; y++) map[y][n.x] &= CELL_HOLE;
					break;
				case RIGHT:
					do y1--; while ((map[y1][x1]&~CELL_HOLE)==dmap);
					y1++; y2=y1+blocks[index].h;
					      x2=x1+blocks[index].w; x1++; 
					for (int y=y1; y<y2; y++) if (map[y][x2] & CELL_MASK) return -1;
					for (int y=y1; y<y2; y++) map[y][x2] |= dmap;
					for (int y=y1; y<y2; y++) map[y][n.x] &= CELL_HOLE;
					x2++;
					break;
			}
			// move player
			if (UPDATE_UNCOMPRESSED)
				players[activePlayer] = n;
			if (UPDATE_COMPRESSED)
				updatePlayer(n.x, n.y);
#if (HOLES > 0)
			// check for holes
			for (int y=y1; y<y2; y++)
				for (int x=x1; x<x2; x++)
					if (!(map[y][x] & CELL_HOLE))
						goto noDrop;
			// fill holes
			for (int y=y1; y<y2; y++)
				for (int x=x1; x<x2; x++)
				{
					if (UPDATE_UNCOMPRESSED)
						map[y][x] = 0;
					if (UPDATE_COMPRESSED)
						fillHole(holeIndices[y][x]);
				}
			if (UPDATE_COMPRESSED)
				removeBlock(index);
			if (UPDATE_UNCOMPRESSED)
				removeUncompressedBlock(index);
			clearJustSwitched<UPDATE_UNCOMPRESSED, UPDATE_COMPRESSED>();
			return DELAY_PUSH + DELAY_FILL;
		noDrop:
#endif
			updateBlock(index, x1, y1);
			if (action == UP)
			{
				if (UPDATE_COMPRESSED)
					sortBlockUp(index);
				if (UPDATE_UNCOMPRESSED)
					sortUncompressedBlockUp(index, x1, y1);
			}
			else
			if (action == DOWN)
			{
				if (UPDATE_COMPRESSED)
					sortBlockDown(index);
				if (UPDATE_UNCOMPRESSED)
					sortUncompressedBlockDown(index, x1, y1);
			}
			clearJustSwitched<UPDATE_UNCOMPRESSED, UPDATE_COMPRESSED>();
			return DELAY_PUSH;
#else
			assert(0);
#endif
		}
		else // rotator
		{
#if (ROTATORS > 0)
			assert(dcell == CELL_ROTATOR);
			char rd = index; // direction of pushed blade from rotator
			if (rd%2 == action%2)
				return -1;
			char dd = (char)action - rd; // rotation direction: 1=clockwise, -1=CCW
			if (dd<0) dd+=4;
			char rd2 = rd^2;          // direction from blade to rotator
			uint8_t rx = n.x+DX[rd2]; // rotator center coords
			uint8_t ry = n.y+DY[rd2];
			// check for obstacles
			bool oldFlippers[4], newFlippers[4];
			for (char d=0;d<4;d++)
			{
				uint8_t d2 = (d+dd)&3; // rotated direction
				if ((map[ry+DY[d]][rx+DX[d]] & ~CELL_HOLE) == (CELL_ROTATOR | d))
				{
					if (UPDATE_UNCOMPRESSED)
						oldFlippers[d ] =
						newFlippers[d2] = true;
					if (map[ry+DY[d]+DY[d2]][rx+DX[d]+DX[d2]] & ~CELL_HOLE)                   // no object/wall in corner
						return -1;
					uint8_t d2m = 
						map[ry+      DY[d2]][rx+      DX[d2]] & ~CELL_HOLE;
					if (d2m != (CELL_ROTATOR | d2) &&       // no object in destination (other than part of the rotator)
					    d2m != CELL_EMPTY)
						return -1;
				}
				else
					if (UPDATE_UNCOMPRESSED)
						oldFlippers[d ] =
						newFlippers[d2] = false;
			}
			if (UPDATE_UNCOMPRESSED)
			{
				// rotate it
				for (char d=0;d<4;d++)
					if (!oldFlippers[d] && newFlippers[d])
					{
						uint8_t* m = &map[ry+DY[d]][rx+DX[d]];
						assert((*m & ~CELL_HOLE)==0);
						*m |= CELL_ROTATOR | d;
					}
					else
					if (oldFlippers[d] && !newFlippers[d])
						map[ry+DY[d]][rx+DX[d]] &= CELL_HOLE;
				if (map[n.y][n.x]) // full push
				{
					n.x += DX[action];
					n.y += DY[action];
					if (map[n.y][n.x] != CELL_EMPTY)
						return 0;
				}
				players[activePlayer] = n;
			}
			if (UPDATE_COMPRESSED)
			{
				uint8_t rotIndex = map[ry][rx] & INDEX_MASK;
				if (dd==1)
					rotateCW (rotIndex);
				else
					rotateCCW(rotIndex);
				if (!UPDATE_UNCOMPRESSED)
				{
					char bd = (rd2+dd)&3; // direction of blade behind player from rotator
					if (map[ry+DY[bd]][rx+DX[bd]] == (CELL_ROTATOR | bd)) // full push
					{
						n.x += DX[action];
						n.y += DY[action];
						if (map[n.y][n.x] != CELL_EMPTY)
							return 0;
					}
				}
				updatePlayer(n.x, n.y);
			}
			return DELAY_ROTATE;
#else
			assert(0);
#endif					
		}
		return -1; // unreachable
	}

	template <bool UPDATE_UNCOMPRESSED, bool UPDATE_COMPRESSED>
	INLINE void clearJustSwitched()
	{
#if (PLAYERS>2)
		if (UPDATE_UNCOMPRESSED)
			justSwitched = false;
		if (UPDATE_COMPRESSED)
			compressed.justSwitched = false;
#endif
	}

	void removeUncompressedBlock(int index)
	{
		int w = blocks[index].w, h = blocks[index].h;
		for (int y=1; y<=Y-1; y++)
			for (int x=1; x<=X-1; x++)
				if ((map[y][x] & CELL_MASK)==CELL_BLOCK)
				{
					int index2 = map[y][x] & INDEX_MASK;
					if (blocks[index2].w == w && blocks[index2].h == h && index2 > index)
						map[y][x]--;
				}
	}

	void sortUncompressedBlockUp(int index, int x1, int y1)
	{
		int w1 = blocks[index].w, h1 = blocks[index].h, x=x1+w1, y=y1;
		for (int l=0; l<X-w1; l++)
		{
			if ((map[y][x] & CELL_MASK)==CELL_BLOCK)
			{
				int index2 = map[y][x] & INDEX_MASK;
				int w2 = blocks[index2].w, h2 = blocks[index2].h;
				if (w1 == w2 && h1 == h2 && index > index2)
				{
					int x2=x, y2=y;
					if (map[y-1][x] != map[y][x]) // begins on that row
					{
						assert(map[y2][x2-1] != map[y2][x2]);
						for (int j=y1; j<y1+h1; j++)
							for (int i=x1; i<x1+w1; i++)
								map[j][i] = (map[j][i] & ~INDEX_MASK) | index2;
						for (int j=y2; j<y2+h2; j++)
							for (int i=x2; i<x2+w2; i++)
								map[j][i] = (map[j][i] & ~INDEX_MASK) | index;
						x1 = x2; y1 = y2;
					}
				}
			}
			x++; if (x==X) { x=0; y++; }
		}
	}

	void sortUncompressedBlockDown(int index, int x1, int y1)
	{
		int w1 = blocks[index].w, h1 = blocks[index].h, x=x1-1, y=y1;
		for (int l=0; l<X-w1; l++)
		{
			if ((map[y][x] & CELL_MASK)==CELL_BLOCK)
			{
				int index2 = map[y][x] & INDEX_MASK;
				int w2 = blocks[index2].w, h2 = blocks[index2].h;
				if (w1 == w2 && h1 == h2 && index < index2)
				{
					int x2=x-w2+1, y2=y;
					//printf("%d,%d/%d,%d\n", x1, y1, x2, y2);
					if (map[y-1][x] != map[y][x]) // begins on that row
					{
						assert(map[y2-1][x2] != map[y2][x2]);
						assert(map[y2][x2-1] != map[y2][x2]);
						for (int j=y1; j<y1+h1; j++)
							for (int i=x1; i<x1+w1; i++)
								map[j][i] = (map[j][i] & ~INDEX_MASK) | index2;
						for (int j=y2; j<y2+h2; j++)
							for (int i=x2; i<x2+w2; i++)
								map[j][i] = (map[j][i] & ~INDEX_MASK) | index;
						x1 = x2; y1 = y2;
					}
				}
			}
			if (x==0) { x=X-1; y--; } else x--;
		}
	}

	template<bool UPDATE_UNCOMPRESSED, bool UPDATE_COMPRESSED>
	void switchPlayers()
	{
#if (PLAYERS==1)
		assert(0);
#else
		Player p = players[activePlayer];
		if (!p.exited())
		{
			assert(map[p.y][p.x]==0 || map[p.y][p.x]==CELL_EXIT);
			if (UPDATE_UNCOMPRESSED)
				map[p.y][p.x] = CELL_WALL;
		}
		if (UPDATE_UNCOMPRESSED)
		{
			do { activePlayer = (activePlayer+1)%PLAYERS; } while (players[activePlayer].exited());
			p = players[activePlayer];
			assert(map[p.y][p.x]==CELL_WALL);
			map[p.y][p.x] = CELL_EMPTY;
		}
		if (UPDATE_COMPRESSED)
			do { compressed.activePlayer = (compressed.activePlayer+1)%PLAYERS; } while (players[compressed.activePlayer].exited());
#endif
	}

	INLINE uint8_t playersLeft() const
	{
		return (uint8_t)!players[0].exited()
#if (PLAYERS>1)
			+  (uint8_t)!players[1].exited()
#endif
#if (PLAYERS>2)
			+  (uint8_t)!players[2].exited()
#endif
#if (PLAYERS>3)
			+  (uint8_t)!players[3].exited()
#endif
			;
	}

	INLINE bool isFinish() const
	{
		return playersLeft()==0;
	}

	void compress(CompressedState* s) const
	{
		debug_assert(compressedUpdated, "Inner compressed state is not up to date");
		*s = compressed;
	}

	void decompress(const CompressedState* s);
	INLINE void updatePlayer(uint8_t x, uint8_t y);
	INLINE void fillHole(int index);
	INLINE void updateBlock(int index, uint8_t x, uint8_t y);
	INLINE void removeBlock(int index);
	INLINE void sortBlockUp(int index);
	INLINE void sortBlockDown(int index);
	INLINE void rotateCW(int index);
	INLINE void rotateCCW(int index);

	#ifdef HAVE_VALIDATOR
	bool validate() const;
	#endif

	/// Optimize state for decompression
	void blank()
	{
		for (int y=0;y<Y;y++)
			for (int x=0;x<X;x++)
				map[y][x] &= ~0x1F; // clear blocks and rotators
		#if (PLAYERS>1)
		for (int p=1; p<PLAYERS; p++)
			map[players[p].y][players[p].x] = 0;
		#endif
	}

	const char* toString() const
	{
		debug_assert(uncompressedUpdated);
		char level[Y][X];

		for (int y=0; y<Y; y++)
			for (int x=0; x<X; x++)
			{
				uint8_t c = map[y][x];
				switch (c & CELL_MASK)
				{
					case CELL_WALL:
						if (c & CELL_HOLE)
							level[y][x] = 'X';
						else
						{
							bool rotatorCenter = false;
							if (x>0 && x<X-1 && y>0 && y<Y-1)
								for (int d=0; d<4; d++)
									if (map[y+DY[d]][x+DX[d]]==(CELL_ROTATOR | d))
										rotatorCenter = true;
							level[y][x] = rotatorCenter	? '+' : '#';
						}
						break;
					case CELL_ROTATOR:
						level[y][x] = DR[c & INDEX_MASK];
						break;
					case CELL_BLOCK:
						level[y][x] = 'a' + (c & INDEX_MASK);
						break;
					case CELL_EMPTY:
						if (c & CELL_HOLE)
							level[y][x] = 'O';
						else
							level[y][x] = ' ';
						break;
				}
			}
		for (int p=0;p<PLAYERS;p++)
			if (!players[p].exited())
				level[players[p].y][players[p].x] = p==activePlayer ? '@' : '&';
		
		static char levelstr[Y*(X+1)+1];
		levelstr[Y*(X+1)] = 0;
		for (int y=0; y<Y; y++)
		{
			for (int x=0; x<X; x++)
				levelstr[y*(X+1) + x] = level[y][x];
			levelstr[y*(X+1)+X ] = '\n';
		}
		return levelstr;
	}
};

#include STRINGIZE(Levels/LEVEL.cpp)

#ifdef HAVE_VALIDATOR
#include STRINGIZE(Levels/LEVEL-validator.h)
#endif

INLINE bool operator==(const State& a, const State& b)
{
	debug_assert(a.uncompressedUpdated);
	debug_assert(b.uncompressedUpdated);
	return memcmp(&a.map, &b.map, sizeof(Map))==0 && memcmp(&a.players, &b.players, sizeof(a.players))==0
#if (PLAYERS>1)
	 && a.activePlayer == b.activePlayer
#endif
	;
}

// ******************************************************************************************************

#ifndef MAX_FRAMES
#define MAX_FRAMES (MAX_STEPS*18)
#endif

#define COMPRESSED_BYTES ((COMPRESSED_BITS + 7) / 8)

#define GROUP_FRAMES
#define FRAMES_PER_GROUP 10 // minimal distance between two states

// ******************************************************************************************************

// Defines a move within the graph. x and y are the player's position after movement (used to collapse multiple movement steps that don't change the level layout)
//#pragma pack(1)
struct Step
{
	Action action;
	uint8_t x;
	uint8_t y;
	uint8_t extraSteps;

	const char* toString()
	{
		return format("@%u,%u: %s", x, y, actionNames[action]);
	}
};

INLINE int replayStep(State* state, FRAME* frame, Step step)
{
	Player* p = &state->players[state->activePlayer];
	int nx = step.x;
	int ny = step.y;
	int steps = abs((int)p->x - nx) + abs((int)p->y - ny) + step.extraSteps;
	p->x = nx;
	p->y = ny;
	assert(state->map[ny][nx]==0, "Bad coordinates");
	DEBUG_ONLY(state->updatePlayer(nx, ny)); // needed to pass decompression check
	int res = state->perform<true, false>((Action)step.action);
	assert(res>0, "Replay failed");
	*frame += steps * DELAY_MOVE + res;
	return steps; // not counting actual action
}

// ******************************************************************************************************

template <class CHILD_HANDLER>
void expandChildren(FRAME frame, const State* state)
{
	struct Coord { uint8_t x, y; };
	const int QUEUELENGTH = X+Y;
	Coord queue[QUEUELENGTH];
	uint8_t distance[Y-2][X-2];
	uint32_t queueStart=0, queueEnd=1;
	memset(distance, 0xFF, sizeof(distance));
	
	uint8_t x0 = state->players[state->activePlayer].x;
	uint8_t y0 = state->players[state->activePlayer].y;
	queue[0].x = x0;
	queue[0].y = y0;
	distance[y0-1][x0-1] = 0;

	State newState = *state;
	Player* np = &newState.players[newState.activePlayer];
	while(queueStart != queueEnd)
	{
		Coord c = queue[queueStart];
		queueStart = (queueStart+1) % QUEUELENGTH;
		uint8_t dist = distance[c.y-1][c.x-1];
		Step step;
		step.x = c.x;
		step.y = c.y;
		step.extraSteps = dist - (abs((int)c.x - (int)x0) + abs((int)c.y - (int)y0));

#if (PLAYERS>1)
		np->x = c.x;
		np->y = c.y;
		DEBUG_ONLY(newState.updatePlayer(c.x, c.y)); // needed to pass decompression check
		int res;
		if (CHILD_HANDLER::PREFERRED==PREFERRED_STATE_UNCOMPRESSED)
			res = newState.perform<true, false>(SWITCH);
		else
			res = newState.perform<false, true>(SWITCH);
		assert(res == DELAY_SWITCH || res == DELAY_SWITCH_AGAIN);
		step.action = SWITCH;
		if (CHILD_HANDLER::PREFERRED==PREFERRED_STATE_UNCOMPRESSED)
			CHILD_HANDLER::handleChild(state, frame, step, &newState           , frame + dist * DELAY_MOVE + DELAY_SWITCH);
		else
			CHILD_HANDLER::handleChild(state, frame, step, &newState.compressed, frame + dist * DELAY_MOVE + DELAY_SWITCH);
		newState = *state;
#endif

		for (Action action = ACTION_FIRST; action < SWITCH; action++)
		{
			uint8_t nx = c.x + DX[action];
			uint8_t ny = c.y + DY[action];
			uint8_t m = newState.map[ny][nx];
			if (m == 0) // free
			{
				if (distance[ny-1][nx-1] == 0xFF)
				{
					distance[ny-1][nx-1] = dist+1;
					queue[queueEnd].x = nx;
					queue[queueEnd].y = ny;
					queueEnd = (queueEnd+1) % QUEUELENGTH;
					assert(queueEnd != queueStart, "Queue overflow");
				}
			}
			else
			{
				uint8_t cell = m & CELL_MASK;
				if (((m & CELL_HOLE)!=0) == (cell == CELL_WALL))
				{
					np->x = c.x;
					np->y = c.y;
					DEBUG_ONLY(newState.updatePlayer(c.x, c.y)); // needed to pass decompression check
					int res;
					if (CHILD_HANDLER::PREFERRED==PREFERRED_STATE_UNCOMPRESSED)
						res = newState.perform<true, false>(action);
					else
						res = newState.perform<false, true>(action);
					if (res > 0)
					{
						step.action = action;
						if (CHILD_HANDLER::PREFERRED==PREFERRED_STATE_UNCOMPRESSED)
							CHILD_HANDLER::handleChild(state, frame, step, &newState           , frame + dist * DELAY_MOVE + res);
						else
							CHILD_HANDLER::handleChild(state, frame, step, &newState.compressed, frame + dist * DELAY_MOVE + res);
					}
					if (res >= 0)
						newState = *state;
				}
			}
		}
	}
}

// ******************************************************************************************************

const char* formatProblemFileName(const char* name, const char* detail, const char* ext)
{
	return format("%s%s%u%s%s.%s", name ? name : "", name ? "-" : "", LEVEL, detail ? "-" : "", detail ? detail : "", ext);
}

// ******************************************************************************************************

void writeSolution(const State* initialState, Step steps[], int stepNr)
{
	FILE* f = fopen(formatProblemFileName(NULL, NULL, "txt"), "wt");
	steps[stepNr].action = NONE;
	steps[stepNr].x = initialState->players[0].x-1;
	steps[stepNr].y = initialState->players[0].y-1;
	unsigned int totalSteps = 0;
	State state = *initialState;
	FRAME frame = 0;
	while (stepNr>=0)
	{
		fprintf(f, "[%u] %s\n", frame, steps[stepNr].toString());
		fprintf(f, "%s", state.toString());
		if (--stepNr>=0)
			totalSteps += (steps[stepNr].action<SWITCH ? 1 : 0) + replayStep(&state, &frame, steps[stepNr]);
	}
	fprintf(f, "Total steps: %u", totalSteps);
	fclose(f);
}

// ******************************************************************************************************

const State* initialStates = &State::initial;
int initialStateCount = 1;

void initProblem()
{
	printf("Kwirk Level %u (%d-%d): %ux%u, %u players\n", LEVEL, LEVEL/10+1, LEVEL%10+1, X, Y, PLAYERS);

#ifdef HAVE_VALIDATOR
	printf("Level state validator present\n");
#endif
}
