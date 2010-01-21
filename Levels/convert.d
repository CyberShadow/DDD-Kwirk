import std.file;
import std.string;
import std.stdio;

void main()
{
	int levelNr = -1;
	while (exists(format("%d.txt", ++levelNr)))
		try
		{
			string[] level, userOptions;
			foreach (line; splitlines(cast(string)read(format("%d.txt", levelNr))))
			{
				if (line.length==0)
					continue;
				else
				if (line[0]=='#')
				{
					if (level.length && level[0].length != line.length)
						throw new Exception("Uneven level width");
					level ~= line;
				}
				else
					userOptions ~= line;
			}

			enum Cell { Floor, Wall, Hole, Exit, Player, TurnstileUp, TurnstileRight, TurnstileDown, TurnstileLeft, TurnstileCenter, Block }
			struct Block { int x, y, w, h, ix, iy, index; int opCmp(Block* other) { return w!=other.w ? w-other.w : h!=other.h ? h-other.h : y!=other.y ? y-other.y : x-other.x; } }
			enum TurnstileType { Uni, RightAngle, Straight, Tri, Plus, Max }
			const string turnstileTypeNames[TurnstileType.Max] = ["Uni", "RightAngle", "Straight", "Tri", "Plus"];
			//const int turnstileBits[TurnstileType.Max] = [2, 2, 1, 2, 0];
			struct Turnstile { int x, y; TurnstileType type; }
			struct Hole { int x, y; }
			struct Player { int x, y; }

			int width=level[0].length, height=level.length, xBits = log2(width-2), yBits = log2(height-2);

			Cell[][] map;
			map.length = height;
			foreach (ref line; map)
				line.length = width;

			int[][] indices;
			indices.length = height;
			foreach (ref line; indices)
				line.length = width;

			int[][] holeIndices;
			holeIndices.length = height;
			foreach (ref line; holeIndices)
				line.length = width;

			int exitX, exitY;
			bool[char.max] seenBlocks;
			Block[] blocks;
			Turnstile[] turnstiles;
			Hole[] holes;
			Player[] players;
		
			foreach (y, line; level)
				foreach (x, c; line)
					switch (c)
					{
						case ' ':
							map[y][x] = Cell.Floor;
							break;
						case '#':
						case '+':
							map[y][x] = Cell.Wall;
							break;
						case 'O':
							map[y][x] = Cell.Hole;
							holeIndices[y][x] = holes.length;
							holes ~= Hole(x, y);
							break;
						case '1':
							map[y][x] = Cell.Floor;
							if (players.length<1) players.length = 1;
							players[0] = Player(x, y);
							break;
						case '2':
							map[y][x] = Cell.Exit;
							enforce(exitX==0 && exitY==0, "Multiple exits");
							exitX = x;
							exitY = y;
							break;
						case '3':
							map[y][x] = Cell.Player;
							if (players.length<2) players.length = 2;
							players[1] = Player(x, y);
							break;
						case '4':
							map[y][x] = Cell.Player;
							if (players.length<3) players.length = 3;
							players[2] = Player(x, y);
							break;
						case '5':
							map[y][x] = Cell.Player;
							if (players.length<4) players.length = 4;
							players[3] = Player(x, y);
							break;
						case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
							if (!seenBlocks[c])
							{
								enforce(x>0 && x<width -1 && y>0 && y<height-1, "Misplaced block");
								int x2=x, y2=y;
								while (level[y][x2]==c)
									x2++;
								while (level[y2][x]==c)
									y2++;
								for (int j=y; j<y2; j++)
									for (int i=x; i<x2; i++)
									{
										map    [j][i] = Cell.Block;
										indices[j][i] = blocks.length;
									}
								blocks ~= Block(x, y, x2-x, y2-y);
								seenBlocks[c] = true;
							}
							break;
						case '^':
							map[y][x] = Cell.TurnstileUp;
							break;
						case '>':
							map[y][x] = Cell.TurnstileRight;
							break;
						case '`':
							map[y][x] = Cell.TurnstileDown;
							break;
						case '<':
							map[y][x] = Cell.TurnstileLeft;
							break;
						case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':           case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
						{
							bool isCenter;
							ubyte[] wings;
							for (int d=0; d<4; d++)
							{
								char c2 = level[y+DY[d]][x+DX[d]];
								if (c2 == DR[d])
									isCenter = true;
								if (c2 == c || c2 == DR[d])
									wings ~= d;
							}
							if (!wings.length)
								throw new Exception("Zero-wing turnstile?");
							if (wings.length>1 || isCenter) // are we on center?
							{
								map[y][x] = Cell.TurnstileCenter;
								indices[y][x] = turnstiles.length;
								TurnstileType type;
								switch (wings.length)
								{
									case 1:
										type = TurnstileType.Uni;
										break;
									case 2:
										if (wings[1]-wings[0]==2)
											type = TurnstileType.Straight;
										else
											type = TurnstileType.RightAngle;
										break;
									case 3:
										type = TurnstileType.Tri;
										break;
									case 4:
										type = TurnstileType.Plus;
										break;
									default:
										throw new Exception("Bad turnstile wings: " ~ format("%s", wings));
								}
								turnstiles ~= Turnstile(x, y, type);
							}
							else
								switch (wings[0]^2)
								{
									case 0: map[y][x] = Cell.TurnstileUp   ; break;
									case 1: map[y][x] = Cell.TurnstileRight; break;
									case 2: map[y][x] = Cell.TurnstileDown ; break;
									case 3: map[y][x] = Cell.TurnstileLeft ; break;
								}
							break;
						}
						default:
							enforce(0, format("Unknown character in level: %s", c));
					}

			// 5-bit index limits
			if (blocks.length > 32)
				throw new Exception("Too many blocks");
			if (turnstiles.length > 32)
				throw new Exception("Too many turnstiles");

			// sort blocks by size
			foreach (i, ref block; blocks)
				block.index = i;
			blocks.sort;
			int[] blockMap = new int[blocks.length];
			foreach (i, block; blocks)
				blockMap[block.index] = i;
			foreach (y, line; map)
				foreach (x, c; line)
					if (c == Cell.Block)
						indices[y][x] = blockMap[indices[y][x]];

			foreach (ref block; blocks)
			{
				block.ix = exitX;
				block.iy = exitY;
				while (block.ix + block.w > width -1)
					block.ix--;
				while (block.iy + block.h > height-1)
					block.iy--;
			}

			// **********************************************************************************************************

			string[] options;
			options ~= format("LEVEL %d", levelNr);
			options ~= format("X %d", width);
			options ~= format("Y %d", height);
			options ~= format("PLAYERS %d", players.length);
			options ~= format("BLOCKS %d", blocks.length);
			options ~= format("TURNSTILES %d", turnstiles.length);
			options ~= format("HOLES %d", holes.length);
			options ~= format("EXIT_X %d", exitX);
			options ~= format("EXIT_Y %d", exitY);
			options ~= format("XBITS %d", xBits);
			options ~= format("YBITS %d", yBits);
			options ~= userOptions;

			struct Field
			{
				int size;
				string name;
				int value;
				int opCmp(Field* other) { return size-other.size; }
			}
			Field[] fields;

			if (players.length>1)
				fields ~= Field(log2(players.length), "activePlayer", 0);
			if (players.length>2)
				fields ~= Field(1, "justSwitched", 0);
			foreach (i, player; players)
			{
				fields ~= Field(xBits, format("player%dx", i), player.x-1);
				fields ~= Field(yBits, format("player%dy", i), player.y-1);
			}
			foreach (i, block; blocks)
			{
				fields ~= Field(log2(width -2-(block.w-1)), format("block%dx", i), block.x-1);
				fields ~= Field(log2(height-2-(block.h-1)), format("block%dy", i), block.y-1);
			}
			foreach (i, turnstile; turnstiles)
			{
				bool[4] b;
				b[0] = map[turnstile.y-1][turnstile.x  ] == Cell.TurnstileUp;
				b[1] = map[turnstile.y  ][turnstile.x+1] == Cell.TurnstileRight;
				b[2] = map[turnstile.y+1][turnstile.x  ] == Cell.TurnstileDown;
				b[3] = map[turnstile.y  ][turnstile.x-1] == Cell.TurnstileLeft;
				switch (turnstile.type)
				{
					case TurnstileType.Uni:
						fields ~= Field(1, format("turnstile%dab", i), b[0] || b[1]);
						fields ~= Field(1, format("turnstile%dac", i), b[0] || b[2]);
						break;
					case TurnstileType.RightAngle:
						fields ~= Field(1, format("turnstile%da", i), b[0]);
						fields ~= Field(1, format("turnstile%db", i), b[1]);
						break;
					case TurnstileType.Straight:
						fields ~= Field(1, format("turnstile%da", i), b[0]);
						break;
					case TurnstileType.Tri:
						fields ~= Field(1, format("turnstile%dab", i), b[0] && b[1]);
						fields ~= Field(1, format("turnstile%dac", i), b[0] && b[2]);
						break;
					case TurnstileType.Plus:
						// Stateless!
						break;
				}
			}
			foreach (i, hole; holes)
				fields ~= Field(1, format("hole%d", i), 1);

			// **********************************************************************************************************

			struct Slot { Field[] fields; int size() { int bits; foreach (field; fields) bits += field.size; return bits; } int bitsLeft() { return 32 - size(); } }
			Slot[] slots;
			
			//fields.reverse.sort.reverse;
			foreach (field; fields)
			{
				bool found;
				foreach (ref slot; slots)
					if (slot.bitsLeft >= field.size)
					{
						slot.fields ~= field;
						found = true;
						break;
					}
				if (!found)
					slots ~= Slot([field]);
			}

			options ~= format("COMPRESSED_BITS %d", (slots.length-1)*32 + slots[$-1].size);

			if (slots[$-1].bitsLeft < 8)
				slots ~= Slot([Field(8, "subframe")]);
			else
			{
				if (slots[$-1].bitsLeft >= 24)
					slots[$-1].fields ~= Field(slots[$-1].bitsLeft-16, "_align");
				else
				if (slots[$-1].bitsLeft%8 != 0)
					slots[$-1].fields ~= Field(slots[$-1].bitsLeft% 8, "_align");
				slots[$-1].fields ~= Field(8, "subframe");
			}

			string[] output;
			foreach (option; options)
				output ~= ["#define " ~ option];
			/+output ~= "const char level[Y][X+1] = {";
			foreach (line; level)
				output ~= ['\"' ~ line ~ "\","];
			output ~= "};";+/

			output ~= "";

			output ~= "struct CompressedState";
			output ~= "{";
			foreach (i, slot; slots)
			{
				foreach (field; slot.fields)
					output ~= format("\tunsigned %s : %d;", field.name, field.size);
				if (slot.bitsLeft && i != slots.length-1)
					output ~= format("\tunsigned _align%d : %d;", i, slot.bitsLeft);
			}
			output ~= "";
			output ~= "	const char* toString() const;";
			output ~= "};";
			
			output ~= "";
			write(format("%d.h", levelNr), output.join(\n));

			// **********************************************************************************************************

			output = null;

			output ~= "const State State::initial = {";
			output ~= "	{ // map";
			foreach (y, line; map)
			{
				string[] cells;
				foreach (x, el; line)
				{
					string name, index;

					switch (el)
					{
						case Cell.Floor:
							name = "CELL_EMPTY";
							break;
						case Cell.Wall:
						case Cell.Player:
							name = "CELL_WALL";
							break;
						case Cell.Hole:
							name = "CELL_HOLE";
							break;
						case Cell.Exit:
							name = "CELL_EXIT";
							break;
						case Cell.TurnstileUp:
							name = "CELL_TURNSTILE";
							index = "UP";
							break;
						case Cell.TurnstileRight:
							name = "CELL_TURNSTILE";
							index = "RIGHT";
							break;
						case Cell.TurnstileDown:
							name = "CELL_TURNSTILE";
							index = "DOWN";
							break;
						case Cell.TurnstileLeft:
							name = "CELL_TURNSTILE";
							index = "LEFT";
							break;
						case Cell.TurnstileCenter:
							name = "CELL_WALL";
							index = .toString(indices[y][x]);
							break;
						case Cell.Block:
							name = "CELL_BLOCK";
							index = .toString(indices[y][x]);
							break;
					}
					
					if (index)
						cells ~= format("%-14s | %-5s", name, index);
					else
						cells ~= format("%-14s        ", name);
				}
				output ~= "		{ " ~ cells.join(", ") ~ " },";
			}
			output ~= "	},";
			
			output ~= "	{ // players";
			foreach (player; players)
				output ~= format("		{ %2d, %2d },", player.x, player.y);
			output ~= "	},";
			
			if (players.length > 1)
				output ~= "	0, // activePlayer"; 
			if (players.length>2)
				output ~= "	false, // justSwitched"; 

			output ~= "	{ // compressed";
			foreach (slot; slots)
				foreach (field; slot.fields)
					output ~= format("		%2d, // %s", field.value, field.name);
			output ~= "	},";
			output ~= "#ifdef DEBUG";
			output ~= "	true, // compressedUpdated";
			output ~= "	true, // uncompressedUpdated";
			output ~= "#endif";

			output ~= "};";
			output ~= "";

			// **********************************************************************************************************

			output ~= "const Map State::blanked = {";
			foreach (y, line; map)
			{
				string[] cells;
				foreach (x, el; line)
				{
					string name, index;

					switch (el)
					{
						case Cell.Floor:
						case Cell.Player:
						case Cell.Block:
						case Cell.Hole:
							name = "CELL_EMPTY";
							break;
						case Cell.Wall:
							name = "CELL_WALL";
							break;
						case Cell.Exit:
							name = "CELL_EXIT";
							break;
						case Cell.TurnstileUp:
						case Cell.TurnstileRight:
						case Cell.TurnstileDown:
						case Cell.TurnstileLeft:
							int d = cast(int)(el-Cell.TurnstileUp);
							if (turnstiles[indices[y-DY[d]][x-DX[d]]].type == TurnstileType.Plus) // since Plus turnstiles are stateless, place the wings in the blanked template
							{
								name = "CELL_TURNSTILE";
								switch (el)
								{
									case Cell.TurnstileUp:    index = "UP"   ; break;
									case Cell.TurnstileRight: index = "RIGHT"; break;
									case Cell.TurnstileDown:  index = "DOWN" ; break;
									case Cell.TurnstileLeft:  index = "LEFT" ; break;
								}
							}
							else
								name = "CELL_EMPTY";
							break;
						case Cell.TurnstileCenter:
							name = "CELL_WALL";
							index = .toString(indices[y][x]);
							break;
					}
					
					if (index)
						cells ~= format("%-14s | %-5s", name, index);
					else
						cells ~= format("%-14s        ", name);
				}
				output ~= "		{ " ~ cells.join(", ") ~ " },";
			}
			output ~= "};";
			output ~= "";

			// **********************************************************************************************************

			if (holes)
			{
				output ~= "const uint8_t State::holeIndices[Y][X] = {";
				foreach (line; holeIndices)
				{
					string[] cells;
					foreach (index; line)
						cells ~= format("%2d", index);
					output ~= "	{" ~ cells.join(", ") ~ "},";
				}
				output ~= "};";
				output ~= "";
			}

			if (blocks)
			{
				output ~= "const struct Block State::blocks[BLOCKS] = {";
				foreach (block; blocks)
					output ~= format("	{ %2d, %2d },", block.w, block.h);
				output ~= "};";
				output ~= "";
			}

			// **********************************************************************************************************

			output ~= "const char* CompressedState::toString() const";
			output ~= "{";
			output ~= "	char* s = getTempString();";
			output ~= "	*s = 0;";

			if (players.length > 1)
				output ~= "	sprintf(s+strlen(s), \"activePlayer=%d \", activePlayer);";
			if (players.length > 2)
				output ~= "	sprintf(s+strlen(s), \"justSwitched=%d \", justSwitched);";
			foreach (i, player; players)
				output ~= "	sprintf(s+strlen(s), \"player"~.toString(i)~"=(%d,%d) \", player"~.toString(i)~"x+1, player"~.toString(i)~"y+1);";
			foreach (i, block; blocks)
			{
				output ~= "	if (block"~.toString(i)~"x=="~.toString(block.ix-1)~" && block"~.toString(i)~"y=="~.toString(block.ix-1)~")";
				output ~= "		sprintf(s+strlen(s), \"block"~.toString(i)~"["~.toString(block.w)~","~.toString(block.h)~"]=removed \");";
				output ~= "	else";
				output ~= "		sprintf(s+strlen(s), \"block"~.toString(i)~"["~.toString(block.w)~","~.toString(block.h)~"]=(%d,%d) \", block"~.toString(i)~"x+1, block"~.toString(i)~"y+1);";
			}
			foreach (i, turnstile; turnstiles)
				switch (turnstile.type)
				{
					case TurnstileType.Uni:
						output ~= "	sprintf(s+strlen(s), \"turnstile"~.toString(i)~"["~turnstileTypeNames[turnstile.type]~"@"~.toString(turnstile.x)~","~.toString(turnstile.y)~"]=%d%d \", turnstile"~.toString(i)~"ab, turnstile"~.toString(i)~"ac);"; 
						break;
					case TurnstileType.RightAngle:
						output ~= "	sprintf(s+strlen(s), \"turnstile"~.toString(i)~"["~turnstileTypeNames[turnstile.type]~"@"~.toString(turnstile.x)~","~.toString(turnstile.y)~"]=%d%d \", turnstile"~.toString(i)~"a, turnstile"~.toString(i)~"b);"; 
						break;
					case TurnstileType.Straight:
						output ~= "	sprintf(s+strlen(s), \"turnstile"~.toString(i)~"["~turnstileTypeNames[turnstile.type]~"@"~.toString(turnstile.x)~","~.toString(turnstile.y)~"]=%d \", turnstile"~.toString(i)~"a);"; 
						break;
					case TurnstileType.Tri:
						output ~= "	sprintf(s+strlen(s), \"turnstile"~.toString(i)~"["~turnstileTypeNames[turnstile.type]~"@"~.toString(turnstile.x)~","~.toString(turnstile.y)~"]=%d%d \", turnstile"~.toString(i)~"ab, turnstile"~.toString(i)~"ac);"; 
						break;
					case TurnstileType.Plus:
						break;
				}
			if (holes)
			{
				output ~= "	strcat(s, \"holes=\");"; 
				foreach (i, hole; holes)
					output ~= "	sprintf(s+strlen(s), \"%d\", hole"~.toString(i)~");";
			}

			output ~= "	return s;";
			output ~= "}";
			output ~= "";

			// **********************************************************************************************************

			output ~= "void State::decompress(const CompressedState* s)";
			output ~= "{";
			output ~= "	memcpy(map, blanked, sizeof(Map));";

			if (players.length>1)
			{
				output ~= "	activePlayer = s->activePlayer;";
				foreach (i, player; players)
				{
					output ~= format("	{ // player %d", i);
					output ~= format("		uint8_t x = s->player%dx + 1;", i);
					output ~= format("		uint8_t y = s->player%dy + 1;", i);
					output ~= format("		if (activePlayer != %d)", i);
					output ~= format("			map[y][x] |= CELL_WALL;");
					output ~= format("		players[%d].x = x;", i);
					output ~= format("		players[%d].y = y;", i);
					output ~= format("	}");
				}
			}
			else
				foreach (i, player; players)
				{
					output ~= format("	players[%d].x = s->player%dx + 1;", i, i);
					output ~= format("	players[%d].y = s->player%dy + 1;", i, i);
				}

			if (players.length>2)
				output ~= "	justSwitched = s->justSwitched;";

			foreach (i, block; blocks)
			{
				output ~= format("	{ // block %d - %dx%d", i, block.w, block.h);
				output ~= format("		uint8_t x = s->block%dx;", i);
				output ~= format("		uint8_t y = s->block%dy;", i);
				output ~= format("		if (x != %d || y != %d)", block.ix-1, block.iy-1);
				output ~= format("		{");
				output ~= format("			uint8_t* p = &map[y+1][x+1];");
				for (int y=0; y<block.h; y++)
				{
					if (y)
						output ~= format("			p += X;");
					output ~= format("			memset(p, CELL_BLOCK | %2d, %2d);", i, block.w);
				}
				output ~= format("		}");
				output ~= format("	}");
			}

			foreach (i, turnstile; turnstiles)
			{
				output ~= format("	{ // turnstile %d (%s) at %dx%d", i, turnstileTypeNames[turnstile.type], turnstile.x, turnstile.y);
				switch (turnstile.type)
				{
					case TurnstileType.Uni:
						output ~= format("		uint8_t ab = s->turnstile%dab-1;", i);
						output ~= format("		uint8_t ac = s->turnstile%dac-1;", i);
						output ~= format("		map[%2d][%2d] |= (~ab & ~ac) & (CELL_TURNSTILE | UP   );", turnstile.y+DY[0], turnstile.x+DX[0]);
						output ~= format("		map[%2d][%2d] |= (~ab &  ac) & (CELL_TURNSTILE | RIGHT);", turnstile.y+DY[1], turnstile.x+DX[1]);
						output ~= format("		map[%2d][%2d] |= ( ab & ~ac) & (CELL_TURNSTILE | DOWN );", turnstile.y+DY[2], turnstile.x+DX[2]);
						output ~= format("		map[%2d][%2d] |= ( ab &  ac) & (CELL_TURNSTILE | LEFT );", turnstile.y+DY[3], turnstile.x+DX[3]);
						break;
					case TurnstileType.RightAngle:
						output ~= format("		uint8_t a = s->turnstile%da-1;", i);
						output ~= format("		uint8_t b = s->turnstile%db-1;", i);
						output ~= format("		map[%2d][%2d] |= (~a) & (CELL_TURNSTILE | UP   );", turnstile.y+DY[0], turnstile.x+DX[0]);
						output ~= format("		map[%2d][%2d] |= (~b) & (CELL_TURNSTILE | RIGHT);", turnstile.y+DY[1], turnstile.x+DX[1]);
						output ~= format("		map[%2d][%2d] |= ( a) & (CELL_TURNSTILE | DOWN );", turnstile.y+DY[2], turnstile.x+DX[2]);
						output ~= format("		map[%2d][%2d] |= ( b) & (CELL_TURNSTILE | LEFT );", turnstile.y+DY[3], turnstile.x+DX[3]);
						break;
					case TurnstileType.Straight:
						output ~= format("		uint8_t a = s->turnstile%da-1;", i);
						output ~= format("		map[%2d][%2d] |= (~a) & (CELL_TURNSTILE | UP   );", turnstile.y+DY[0], turnstile.x+DX[0]);
						output ~= format("		map[%2d][%2d] |= ( a) & (CELL_TURNSTILE | RIGHT);", turnstile.y+DY[1], turnstile.x+DX[1]);
						output ~= format("		map[%2d][%2d] |= (~a) & (CELL_TURNSTILE | DOWN );", turnstile.y+DY[2], turnstile.x+DX[2]);
						output ~= format("		map[%2d][%2d] |= ( a) & (CELL_TURNSTILE | LEFT );", turnstile.y+DY[3], turnstile.x+DX[3]);
						break;
					case TurnstileType.Tri:
						output ~= format("		uint8_t ab = s->turnstile%dab-1;", i);
						output ~= format("		uint8_t ac = s->turnstile%dac-1;", i);
						output ~= format("		map[%2d][%2d] |= (~ab | ~ac) & (CELL_TURNSTILE | UP   );", turnstile.y+DY[0], turnstile.x+DX[0]);
						output ~= format("		map[%2d][%2d] |= (~ab |  ac) & (CELL_TURNSTILE | RIGHT);", turnstile.y+DY[1], turnstile.x+DX[1]);
						output ~= format("		map[%2d][%2d] |= ( ab | ~ac) & (CELL_TURNSTILE | DOWN );", turnstile.y+DY[2], turnstile.x+DX[2]);
						output ~= format("		map[%2d][%2d] |= ( ab |  ac) & (CELL_TURNSTILE | LEFT );", turnstile.y+DY[3], turnstile.x+DX[3]);
						break;
					case TurnstileType.Plus:
						output ~= format("		// Stateless - the wings are already in the blanked template");
						break;
				}
				output ~= format("	}");
			}

			foreach (i, hole; holes)
				output ~= format("	map[%2d][%2d] |= s->hole%d << 7;", hole.y, hole.x, i);

			output ~= "	compressed = *s;";
			output ~= "	DEBUG_ONLY(compressedUpdated = true);";
			output ~= "	DEBUG_ONLY(uncompressedUpdated = true);";
			output ~= "}";
			output ~= "";
			
			// **********************************************************************************************************

			output ~= "INLINE void State::updatePlayer(uint8_t x, uint8_t y)";
			output ~= "{";
			output ~= "	x--; y--;";
			if (players.length==1)
			{
				output ~= "	compressed.player0x = x;";
				output ~= "	compressed.player0y = y;";
			}
			else
			{
				output ~= "	switch (activePlayer)";
				output ~= "	{";
				foreach (i, player; players)
				{
					output ~= format("		case %d:", i);
					output ~= format("			compressed.player%dx = x;", i);
					output ~= format("			compressed.player%dy = y;", i);
					output ~= format("			break;");
				}
				output ~= "	}";
			}
			output ~= "}";
			output ~= "";
			
			// **********************************************************************************************************

			if (holes)
			{
				output ~= "INLINE void State::fillHole(int index)";
				output ~= "{";
				output ~= "	switch (index)";
				output ~= "	{";
				foreach (i, hole; holes)
				{
					output ~= format("		case %d:", i);
					output ~= format("			assert(compressed.hole%d == 1);", i);
					output ~= format("			compressed.hole%d = 0;", i);
					output ~= format("			break;");
				}
				output ~= "	}";
				output ~= "}";
				output ~= "";
			}
			
			// **********************************************************************************************************

			if (blocks)
			{
				output ~= "INLINE void State::updateBlock(int index, uint8_t x, uint8_t y)";
				output ~= "{";
				output ~= "	x--; y--;";
				output ~= "	switch (index)";
				output ~= "	{";
				foreach (i, block; blocks)
				{
					output ~= format("		case %d:", i);
					output ~= format("			compressed.block%dx = x;", i);
					output ~= format("			compressed.block%dy = y;", i);
					output ~= format("			break;");
				}
				output ~= "	}";
				output ~= "}";
				output ~= "";

				output ~= "INLINE void State::removeBlock(int index)";
				output ~= "{";
				output ~= "	while (true)";
				output ~= "		switch (index)";
				output ~= "		{";
			nextBlock:
				foreach (i, block; blocks)
				{
					output ~= format("			case %d:", i);
					foreach (ij, block2; blocks[i+1..$])
						if (block2.w == block.w && block2.h == block.h)
						{
							int j = i+1 + ij;
							output ~= format("				compressed.block%dx = compressed.block%dx;", i, j);
							output ~= format("				compressed.block%dy = compressed.block%dy;", i, j);
							if (i+1 == j)
								output ~= format("				// fall through");
							else
							{
								output ~= format("				index = %d;", j);
								output ~= format("				break;");
							}
							continue nextBlock;
						}
					output ~= format("				compressed.block%dx = %d;", i, block.ix-1);
					output ~= format("				compressed.block%dy = %d;", i, block.iy-1);
					output ~= format("				return;");
				}
				output ~= "		}";
				output ~= "}";
				output ~= "";

				output ~= "INLINE void State::sortBlockDown(int index)";
				output ~= "{";
				output ~= "	while (true)";
				output ~= "		switch (index)";
				output ~= "		{";
				foreach (i, block; blocks)
				{
					output ~= format("			case %d:", i);
					output ~= format("			{");
					foreach (ij, block2; blocks[i+1..$])
						if (block2.w == block.w && block2.h == block.h)
						{
							int j = i+1 + ij;
							output ~= format("				uint8_t x2 = compressed.block%dx, y2 = compressed.block%dy;", j, j);
							output ~= format("				if (x2 == %d && y2 == %d)", block2.ix-1, block2.iy-1);
							output ~= format("					return;");
							output ~= format("				uint8_t x1 = compressed.block%dx, y1 = compressed.block%dy;", i, i);
							output ~= format("				if (y1 < y2 || (y1 == y2 && x1 < x2))");
							output ~= format("					return;");
							output ~= format("				compressed.block%dx = x2;", i);
							output ~= format("				compressed.block%dy = y2;", i);
							output ~= format("				compressed.block%dx = x1;", j);
							output ~= format("				compressed.block%dy = y1;", j);
							if (i+1 == j)
								output ~= format("				// fall through");
							else
							{
								output ~= format("				index = %d;", j);
								output ~= format("				break;");
							}
							goto nextBlock2;
						}
					output ~= format("				return;");
				nextBlock2:
					output ~= format("			}");
				}
				output ~= "		}";
				output ~= "}";
				output ~= "";

				output ~= "INLINE void State::sortBlockUp(int index)";
				output ~= "{";
				output ~= "	while (true)";
				output ~= "		switch (index)";
				output ~= "		{";
				foreach_reverse (i, block; blocks)
				{
					output ~= format("			case %d:", i);
					output ~= format("			{");
					foreach_reverse (j, block2; blocks[0..i])
						if (block2.w == block.w && block2.h == block.h)
						{
							output ~= format("				uint8_t x2 = compressed.block%dx, y2 = compressed.block%dy;", j, j);
							output ~= format("				if (x2 == %d && y2 == %d)", block2.ix-1, block2.iy-1);
							output ~= format("					return;");
							output ~= format("				uint8_t x1 = compressed.block%dx, y1 = compressed.block%dy;", i, i);
							output ~= format("				if (y1 > y2 || (y1 == y2 && x1 > x2))");
							output ~= format("					return;");
							output ~= format("				compressed.block%dx = x2;", i);
							output ~= format("				compressed.block%dy = y2;", i);
							output ~= format("				compressed.block%dx = x1;", j);
							output ~= format("				compressed.block%dy = y1;", j);
							if (i-1 == j)
								output ~= format("				// fall through");
							else
							{
								output ~= format("				index = %d;", j);
								output ~= format("				break;");
							}
							goto nextBlock3;
						}
					output ~= format("				return;");
				nextBlock3:
					output ~= format("			}");
				}
				output ~= "		}";
				output ~= "}";
				output ~= "";
			}
			
			// **********************************************************************************************************

			if (turnstiles)
			{
				output ~= "INLINE void State::rotateCW(int index)";
				output ~= "{";
				output ~= "	switch (index)";
				output ~= "	{";
				foreach (i, turnstile; turnstiles)
				{
					output ~= format("		case %d:", i);
					output ~= format("		{");
					switch (turnstile.type)
					{
						case TurnstileType.Uni:
							output ~= format("			compressed.turnstile%dac ^= 1;", i);
							output ~= format("			compressed.turnstile%dab ^= compressed.turnstile%dac;", i, i);
							break;
						case TurnstileType.RightAngle:
							output ~= format("			bool b = compressed.turnstile%db;", i);
							output ~= format("			compressed.turnstile%db = compressed.turnstile%da;", i, i);
							output ~= format("			compressed.turnstile%da = !b;", i);
							break;
						case TurnstileType.Straight:
							output ~= format("			compressed.turnstile%da ^= 1;", i);
							break;
						case TurnstileType.Tri:
							output ~= format("			compressed.turnstile%dab ^= compressed.turnstile%dac;", i, i);
							output ~= format("			compressed.turnstile%dac ^= 1;", i);
							break;
						case TurnstileType.Plus:
							break;
					}
					output ~= format("			break;");
					output ~= format("		}");
				}
				output ~= "	}";
				output ~= "}";
				output ~= "";

				output ~= "INLINE void State::rotateCCW(int index)";
				output ~= "{";
				output ~= "	switch (index)";
				output ~= "	{";
				foreach (i, turnstile; turnstiles)
				{
					output ~= format("		case %d:", i);
					output ~= format("		{");
					switch (turnstile.type)
					{
						case TurnstileType.Uni:
							output ~= format("			compressed.turnstile%dab ^= compressed.turnstile%dac;", i, i);
							output ~= format("			compressed.turnstile%dac ^= 1;", i);
							break;
						case TurnstileType.RightAngle:
							output ~= format("			bool a = compressed.turnstile%da;", i);
							output ~= format("			compressed.turnstile%da = compressed.turnstile%db;", i, i);
							output ~= format("			compressed.turnstile%db = !a;", i);
							break;
						case TurnstileType.Straight:
							output ~= format("			compressed.turnstile%da ^= 1;", i);
							break;
						case TurnstileType.Tri:
							output ~= format("			compressed.turnstile%dac ^= 1;", i);
							output ~= format("			compressed.turnstile%dab ^= compressed.turnstile%dac;", i, i);
							break;
						case TurnstileType.Plus:
							break;
					}
					output ~= format("			break;");
					output ~= format("		}");
				}
				output ~= "	}";
				output ~= "}";
				output ~= "";
			}
			
			// **********************************************************************************************************

			write(format("%d.cpp", levelNr), output.join(\n));
		}
		catch (Object o)
		{
			writefln("Error with level %d: %s", levelNr, o.toString());
		}
}

const byte DX[4] = [0, 1, 0, -1];
const byte DY[4] = [-1, 0, 1, 0];
const char DR[4] = "^>`<";

// log2 of x, rounded up (how many bits do we need to store a number from 0 to x-1)
uint log2(uint x)
{
	assert(x);
	x--;
	uint result;
	while (x)
		result++,
		x >>= 1;
	return result;
}

void enforce(bool condition, lazy string message)
{
	if (!condition)
		throw new Exception(message);
}
