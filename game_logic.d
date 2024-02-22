import std.algorithm.comparison;

import ae.utils.mapset;
import ae.utils.mapset.vars;
import ae.utils.meta;

import common;

alias StateSet = MapSet!(VarName, VarValue);
alias Visitor = MapSetVisitor!(VarName, VarValue);
alias Vars = MapSetVars!(VarName, VarValue);

enum Action : ubyte
{
	right,
	up,
	left,
	down,

	switchCharacter,
}

enum delayMove         =  9; // 1+8
enum delayPush         = 10; // 2+8
enum delayFill         = 26;
enum delayRotate       = 12;
enum delaySwitch       = 30;
enum delaySwitch_Again = 32;
enum delayExit         =  1; // fake delay to prevent grouping into one frame group

int perform(ref const Level level, ref Vars v, Action action)
{
	final switch (action)
	{
		// case Action.none:
		// 	assert(false);

		case Action.switchCharacter:
			if (level.numCharacters == 1)
				return -1;
			else
			{
				assert(false, "TODO Action.switchCharacter");
				// ubyte playerCount = playersLeft();
				// if (playerCount)
				// {
				// 	switchPlayers(playerCount);

				// 	static if (levelDef.characters > 2)
				// 	{
				// 		int res = justSwitched ? DELAY_SWITCH_AGAIN : DELAY_SWITCH;
				// 		if (UPDATE_UNCOMPRESSED)
				// 			justSwitched = true;
				// 		if (UPDATE_COMPRESSED)
				// 			compressed.justSwitched = true;
				// 		return res;
				// 	}
				// 	else
				// 		return DELAY_SWITCH;
				// }
				// else
				// 	return -1;
			}

		case Action.right:
		case Action.up:
		case Action.left:
		case Action.down:
			auto p = v[VarName.character0Coord].resolve().VarValueCharacterCoord;
			auto n = p;
			const d = cast(Direction)((action - Action.right) + Direction.right);
			n.x += dirX[d];
			n.y += dirY[d];

			auto tile = level.map[n.y][n.x];
			final switch (tile)
			{
				case Tile.exit:
					dump(level, v.visitor);
					assert(false, "TODO Tile.exit");
					// players[0] = n;
					// justSwitched = false;
					// uint8_t playerCount = playersLeft();
					// if (playerCount)
					// {
					// 	switchPlayers(playerCount+1);
					// 	return DELAY_MOVE + DELAY_SWITCH;
					// }
					// else
					// 	return DELAY_MOVE + DELAY_EXIT;

				case Tile.wall:
				case Tile.turnstileCenter:
					return -1;

				case Tile.free:
					const cell = v[varNameCell(n.x, n.y)].resolve().VarValueCell;

					// If there is a hole, we cannot step into it,
					// no matter what else is here.
					if (cell.hole)
						return -1;

					final switch (cell.type)
					{
						case VarValueCell.Type.empty:
							v[VarName.character0Coord] = n;
							v[varNameCell(p.x, p.y)] = VarValueCell(VarValueCell.Type.empty);
							v[varNameCell(n.x, n.y)] = VarValueCell(VarValueCell.Type.character);
							return delayMove;

						case VarValueCell.Type.block:
							// Original block coords
							auto ox0 = n.x - cell.block.x;
							auto oy0 = n.y - cell.block.y;
							auto ox1 = ox0 + cell.block.w;
							auto oy1 = oy0 + cell.block.h;

							// New block coords
							auto nx0 = ox0 + dirX[d];
							auto ny0 = oy0 + dirY[d];
							auto nx1 = ox1 + dirX[d];
							auto ny1 = oy1 + dirY[d];

							// Pushable in theory?
							foreach (y; ny0 .. ny1)
								foreach (x; nx0 .. nx1)
								{
									auto inOld = x >= ox0 && x < ox1 && y >= oy0 && y < oy1;
									if (!inOld) // in new but not old, i.e. the area that will be newly occupied
										final switch (level.map[y][x])
										{
											case Tile.free:
												continue;
											case Tile.wall:
											case Tile.turnstileCenter:
											case Tile.exit:
												return -1;
										}
								}

							// Pushable in practice?
							foreach (y; ny0 .. ny1)
								foreach (x; nx0 .. nx1)
								{
									auto inOld = x >= ox0 && x < ox1 && y >= oy0 && y < oy1;
									if (!inOld) // in new but not old, i.e. the area that will be newly occupied
									{
										auto ok = v[varNameCell(x, y)].map((v) {
											auto c = v.VarValueCell;
											final switch (c.type)
											{
												case VarValueCell.Type.empty:
													return true; // regardless of hole
												case VarValueCell.Type.block:
												case VarValueCell.Type.turnstile:
												case VarValueCell.Type.character:
													return false;
											}
										}).resolve();
										if (!ok)
											return -1;
									}
								}

							// Fillable in theory?
							auto fillable = {
								foreach (y; ny0 .. ny1)
									foreach (x; nx0 .. nx1)
										if (!level.initialState[varNameCell(x, y)].VarValueCell.hole)
										{
											// There was never, and thus can never be, a hole here.
											return false;
										}
								return true;
							}();

							// Fillable in practice?
							fillable = fillable && {
								foreach (y; ny0 .. ny1)
									foreach (x; nx0 .. nx1)
										if (!v[varNameCell(x, y)].resolve().VarValueCell.hole)
										{
											// There was a hole here once, but not right now.
											return false;
										}
								return true;
							}();

							foreach (y; min(oy0, ny0) .. max(oy1, ny1))
								foreach (x; min(ox0, nx0) .. max(ox1, nx1))
								{
									auto inOld = x >= ox0 && x < ox1 && y >= oy0 && y < oy1;
									auto inNew = x >= nx0 && x < nx1 && y >= ny0 && y < ny1;
									auto c = v[varNameCell(x, y)].resolve().VarValueCell;
									if (inOld)
										assert(c.type == VarValueCell.Type.block);
									if (inNew)
									{
										if (fillable)
										{
											// Fill it - clear type and hole
											c.type = VarValueCell.Type.empty;
											c.empty = VarValueCell.Empty.init; // Clear vestigial state
											assert(c.hole);
											c.hole = false;
										}
										else
										{
											// Just move it
											c.type = VarValueCell.Type.block;
											c.block.w = cell.block.w;
											c.block.h = cell.block.h;
											c.block.x = cast(ubyte)(x - nx0);
											c.block.y = cast(ubyte)(y - ny0);
										}
									}	
									else
									if (inOld)
									{
										// Clear only type (leaving hole)
										c.type = VarValueCell.Type.empty;
										c.empty = VarValueCell.Empty.init; // Clear vestigial state
									}
									v[varNameCell(x, y)] = c;
								}

							// The way forward is now clear.
							v[VarName.character0Coord] = n;
							v[varNameCell(p.x, p.y)] = VarValueCell(VarValueCell.Type.empty);
							v[varNameCell(n.x, n.y)] = VarValueCell(VarValueCell.Type.character);
							return delayPush;

						case VarValueCell.Type.turnstile:
							assert(false, "TODO VarValueCell.Type.turnstile");

						case VarValueCell.Type.character:
							return -1;
					}
			}
	}
}

void dump(ref const Level level, ref Visitor v)
{
	import std.stdio : write, writeln;

	foreach (y; 0 .. level.h)
	{
		foreach (x; 0 .. level.w)
			final switch (level.map[y][x])
			{
				case Tile.exit:
					write('%');
					break;

				case Tile.wall:
					write('#');
					break;

				case Tile.turnstileCenter:
					write('*');
					break;

				case Tile.free:
					auto cell = v.get(varNameCell(x, y)).VarValueCell;
					final switch (cell.type)
					{
						case VarValueCell.Type.empty:
							if (cell.hole)
								write('O');
							else
								write(' ');
							break;

						case VarValueCell.Type.block:
							write(cast(char)('a' + ((x - cell.block.x) + (y - cell.block.y)) % 26));
							break;

						case VarValueCell.Type.turnstile:
							write(">^<`"[cell.turnstile.thisDirection]);
							break;

						case VarValueCell.Type.character:
							char c = '?';
							foreach (i; 0 .. level.numCharacters)
							{
								auto coord = v.get(varNameCharacterCoord(i)).VarValueCharacterCoord;
								if (coord.x == x && coord.y == y)
									c = cast(char)('1' + i);
							}
							write(c);
							break;
					}
			}
		writeln;
	}
}
