import std.format;
import std.stdio : stderr;

import ae.utils.funopt;
import ae.utils.main;
import ae.utils.meta;

import common;
import game_logic;
import load;

void program(
	int levelNumber,
)
{
	auto fn = format("levels/%d.txt", levelNumber);
	auto level = loadLevel(fn);

	StateSet initialSet = StateSet.unitSet;
	foreach (VarName name, VarValue value; level.initialState)
		if (value != VarValue.init)
			initialSet = initialSet.set(name, value);

	StateSet[] statesAtFrame = [initialSet];

	for (uint frameNumber = 0; ; frameNumber++)
	{
		stderr.writeln("Frame ", frameNumber, ": ", statesAtFrame[frameNumber].count);
		foreach (action; Action.init .. enumLength!Action)
		{
			Vars v;
			v.visitor = Visitor(statesAtFrame[frameNumber]);

			while (v.next())
			{
				auto duration = perform(level, v, action);
				if (duration < 0)
					continue;
				auto nextFrame = frameNumber + duration;
				if (statesAtFrame.length < nextFrame + 1)
					statesAtFrame.length = nextFrame + 1;
				statesAtFrame[nextFrame] = statesAtFrame[nextFrame].merge(v.visitor.currentSubset);
			}
		}
	}
}

mixin main!(funopt!program);
