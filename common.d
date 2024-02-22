module common;

import std.bitmanip;

import ae.utils.meta : enumLength;

enum maxCharacters = 4;
enum maxWidth = 32;
enum maxHeight = 32;

// TODO: I think both of these can be packed to one byte each:
// - VarName can be packed by exploiting that levels have at most about 64 non-wall tiles.
//   Then, Level.map can specify the VarName corresponding to that tile.
// - VarValue can be packed by exploiting that there are only 12 unique block shapes in the entire game,
//   and their area adds up to 56, so 56 values are enough to uniquely indicate both the block shape and the coordinate within the block.

enum VarName : uint
{
	character0Coord,

	cell00 = maxCharacters,

	tempVarStart = cell00 + (maxWidth * maxHeight),
	tempVarEnd = tempVarStart + 100,
	length = tempVarEnd,
}

VarName varNameCharacterCoord(uint characterIndex) { return cast(VarName)(VarName.character0Coord + characterIndex); }
VarName varNameCell(size_t x, size_t y) { return cast(VarName)(VarName.cell00 + y * maxWidth + x); }

alias VarValue = uint;

mixin template VarValueCommon()
{
	static assert(typeof(this).sizeof == VarValue.sizeof);

	this(VarValue v) { this = *cast(typeof(this)*)&v; }

	@property VarValue asVarValue() { return *cast(VarValue*)&this; }
	alias asVarValue this;
}

struct VarValueCharacterCoord
{
	mixin VarValueCommon;

align(1):
	ubyte x, y;
	ubyte[2] _padding;
}

enum Direction : ubyte
{
	right,
	up,
	left,
	down,
}

Direction opposite(Direction d) { return cast(Direction)((d + 2) % 4); }

immutable byte[enumLength!Direction] dirX = [1, 0, -1, 0];
immutable byte[enumLength!Direction] dirY = [0, -1, 0, 1];

struct VarValueCell
{
	mixin VarValueCommon;

align(1):
	enum Type : ubyte
	{
		empty,
		block,
		turnstile,
		character,
	}
	Type type;

	union
	{
		struct Empty { ubyte[2] padding; }
		Empty empty;

		struct Block
		{
			mixin(bitfields!(
				// The total width and height of the entire block.
				ubyte, "w",  4,
				ubyte, "h",  4,
				// The coordinates within the block that are on this tile.
				ubyte, "x",  4,
				ubyte, "y",  4,
			));
		}
		Block block;

		struct Turnstile
		{
			ubyte haveDirection; /// bitfield over Direction
			Direction thisDirection; /// direction of the piece in this tile; opposite direction is the turnstile center
		}
		Turnstile turnstile;
	}

	bool hole;
}

enum Tile : ubyte
{
	free, /// empty, block, hole, or character, etc. - consult the current state to see what's here
	wall, /// cannot be interacted with
	turnstileCenter,
	exit,
}

/// Constants.
struct Level
{
	size_t w, h;
	Tile[][] map;

	VarValue[VarName.length] initialState;

	ubyte numCharacters;
}
