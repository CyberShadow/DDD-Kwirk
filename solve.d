import std.format;
import std.stdio : stderr;

import ae.utils.funopt;
import ae.utils.main;
import ae.utils.meta;

import common;
import game_logic;
import load;

void program(
	string levelFileName,
)
{
	auto level = loadLevel(levelFileName);

	StateSet initialSet = StateSet.unitSet;
	foreach (VarName name, VarValue value; level.initialState)
		if (value != VarValue.init)
			initialSet = initialSet.set(name, value);

	StateSet[] statesAtFrame = [initialSet];
	StateSet seenStates;

	for (uint frameNumber = 0; ; frameNumber++)
	{
		assert(frameNumber < statesAtFrame.length, "No more states.");
		auto set = statesAtFrame[frameNumber];

		stderr.writefln("Frame %d: %d states, %d nodes",
			frameNumber,
			set.count,
			set.uniqueNodes,
		);

		set = set.subtract(seenStates);
		stderr.writefln("  Deduplicated: %d states, %d nodes", set.count, set.uniqueNodes);
		set = set.optimize();
		stderr.writefln("  Optimized: %d nodes", set.uniqueNodes);

		seenStates = seenStates.merge(set);
		stderr.writefln("  Total: %d states, %d nodes", seenStates.count, seenStates.uniqueNodes);
		seenStates = seenStates.optimize();
		stderr.writefln("    Optimized: %d nodes", seenStates.uniqueNodes);

		foreach (action; Action.init .. enumLength!Action)
		{
			Vars v;
			v.visitor = Visitor(set);

			ulong numIterations;
			while (v.next())
			{
				numIterations++;
				auto duration = perform(level, v, action);
				if (duration == performImpossible)
					continue;
				if (duration == performComplete)
					return;
				auto nextFrame = frameNumber + duration;
				if (statesAtFrame.length < nextFrame + 1)
					statesAtFrame.length = nextFrame + 1;
				statesAtFrame[nextFrame] = statesAtFrame[nextFrame].merge(v.visitor.currentSubset);
			}
			stderr.writefln("  Processed %s in %d iterations.", action, numIterations);
		}
	}
}

mixin main!(funopt!program);
