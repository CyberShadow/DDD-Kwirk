import ae.utils.meta;

import std.algorithm.comparison;
import std.algorithm.searching;
import std.algorithm.sorting;
import std.conv;
import std.exception;
import std.file;
import std.string;
import std.stdio;

import common;

Level loadLevel(string fileName)
{
	Level level;

	string[] levelLines;
	foreach (line; splitLines(readText(fileName)))
	{
		if (line.length==0)
			continue;
		else
		if (line.skipOver("MAX_FRAMES "))
			continue; // ignore
		else
		if (line.skipOver("MAX_STEPS "))
			continue; // ignore
		else
		if (line == "HAVE_VALIDATOR")
			continue; // ignore
		else
		if (line[0]=='#')
		{
			if (levelLines.length && levelLines[0].length != line.length)
				throw new Exception("Uneven level width");
			levelLines ~= line;
		}
		else
			throw new Exception("Unknown level line: " ~ line);
	}

	level.w = levelLines[0].length;
	level.h = levelLines.length;

	level.map = new Tile[][](level.h, level.w);

	foreach (y, line; levelLines)
		foreach (x, c; line)
			switch (c)
			{
				// Empty
				case ' ':
					level.map[y][x] = Tile.free;
					break;

				// Wall
				case '#':
				case '+':
					level.map[y][x] = Tile.wall;
					break;

				// Exit
				case '%':
					level.map[y][x] = Tile.exit;
					break;

				// Hole
				case 'O':
					level.map[y][x] = Tile.free;

					VarValueCell cell;
					cell.type = VarValueCell.Type.empty;
					cell.hole = true;
					level.initialState[varNameCell(x, y)] = cell;
					break;

				// Character starting position
				case '1':
					..
				case '4':
					level.map[y][x] = Tile.free;
					auto characterIndex = c - '1';

					VarValueCell cell;
					cell.type = VarValueCell.Type.character;
					level.initialState[varNameCell(x, y)] = cell;

					VarValueCharacterCoord coord;
					coord.x = x.to!ubyte;
					coord.y = y.to!ubyte;
					level.initialState[varNameCharacterCoord(characterIndex)] = coord;

					level.numCharacters = cast(ubyte)max(level.numCharacters, characterIndex + 1);
					break;

				// Block
				case 'a':
					..
				case 'z':
					auto xMin = x, yMin = y, xMax = x, yMax = y;
					while (levelLines[y][xMin - 1] == c) xMin--;
					while (levelLines[y][xMax + 1] == c) xMax++;
					while (levelLines[yMin - 1][x] == c) yMin--;
					while (levelLines[yMax + 1][x] == c) yMax++;
					auto bw = xMax - xMin + 1;
					auto bh = yMax - yMin + 1;

					VarValueCell cell;
					cell.type = VarValueCell.Type.block;
					cell.block.w = bw.to!ubyte;
					cell.block.h = bh.to!ubyte;
					cell.block.x = (x - xMin).to!ubyte;
					cell.block.y = (y - yMin).to!ubyte;
					level.initialState[varNameCell(x, y)] = cell;
					break;

				// Turnstile center
				case '*':
					level.map[y][x] = Tile.turnstileCenter;
					break;

				// Turnstile
				case '>':
				case '^':
				case '<':
				case '`':
					static immutable turnstileWingChars = ">^<`";
					auto d = cast(Direction) turnstileWingChars.indexOf(c);
					auto cx = x + dirX[d.opposite];
					auto cy = y + dirY[d.opposite];
					enforce(levelLines[cy][cx] == '*', "Turnstile wing not attached to center");

					VarValueCell cell;
					cell.type = VarValueCell.Type.turnstile;
					cell.turnstile.thisDirection = d;
					foreach (wd; Direction.init .. enumLength!Direction)
						if (levelLines[cy + dirY[wd]][cx + dirX[wd]] == turnstileWingChars[wd])
							cell.turnstile.haveDirection |= (1 << wd);
					level.initialState[varNameCell(x, y)] = cell;
					break;

				default:
					throw new Exception(format("Unknown character in level: %s", c));
			}

	// Validation

	foreach (y; 0 .. level.h)
		foreach (x; 0 .. level.w)
			switch (level.map[y][x])
			{
				case Tile.wall:
					enforce(level.initialState[varNameCell(x, y)] == VarValue.init);
					break;

				case Tile.turnstileCenter:
					enforce(level.initialState[varNameCell(x, y)] == VarValue.init);

					auto numWings = 0;
					foreach (d; Direction.init .. enumLength!Direction)
					{
						auto dx = x + dirX[d];
						auto dy = y + dirY[d];
						auto dTile = level.initialState[varNameCell(dx, dy)].VarValueCell;
						if (dTile.type == VarValueCell.Type.turnstile && dTile.turnstile.thisDirection == d)
							numWings++;
					}
					enforce(numWings > 0, "Turnstile center without wings");
					break;

				default:
					break;
			}

	return level;
}
