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
	StateSet seenStates;

	for (uint frameNumber = 0; ; frameNumber++)
	{
		auto set = statesAtFrame[frameNumber];
		stderr.writefln("Frame %d: %d / %d",
			frameNumber,
			set.count,
			set.uniqueNodes,
		);

		set = set.subtract(seenStates);
		seenStates = seenStates.merge(set);
		seenStates = seenStates.optimize();

		set = set.optimize();
		stderr.writefln("Optimized: %d", set.uniqueNodes);

		foreach (action; Action.init .. enumLength!Action)
		{
			Vars v;
			v.visitor = Visitor(set);

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
