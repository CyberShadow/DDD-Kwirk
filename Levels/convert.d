import std.file;
import std.string;

void main()
{
	for (int level=0; level<30; level++)
	{
		string[] map, userOptions;
		foreach (line; splitlines(cast(string)read(format("%d.txt", level))))
		{
			if (line.length==0)
				continue;
			else
			if (line[0]=='#')
			{
				if (map.length && map[0].length != line.length)
					throw new Exception("Uneven level width");
				map ~= line;
			}
			else
				userOptions ~= line;
		}

		int players=1, blocks, rotators, holes, exitX, exitY, blockX, blockY;
		bool[char.max] seenBlocks, seenRotators;

		foreach (y, line; map)
			foreach (x, c; line)
				if (c>='3' && c<='5')
				{
					int n = c-'2' + 1;
					if (players < n)
						players = n;
				}
				else
				if (c>='a' && c<='z' && !seenBlocks[c])
				{
					int x2=x, y2=y;
					while (map[y][x2]==c)
						x2++;
					while (map[y2][x]==c)
						y2++;
					if (blockX < x2-x)
						blockX = x2-x;
					if (blockY < y2-y)
						blockY = y2-y;
					blocks++;
					seenBlocks[c] = true;
				}
				else
				if (c>='A' && c<='Z' && c!='O' && !seenRotators[c])
				{
					rotators++;
					seenRotators[c] = true;
				}
				else
				if (c=='O')
					holes++;
				else
				if (c=='2')
				{
					if (exitX!=0 || exitY!=0)
						throw new Exception("Multiple exits");
					exitX = x;
					exitY = y;
				}

		string[] options;
		options ~= format("LEVEL %d", level);
		options ~= format("X %d", map[0].length);
		options ~= format("Y %d", map.length);
		options ~= format("PLAYERS %d", players);
		if (blocks)
		{
			options ~= format("BLOCKS %d", blocks);
			options ~= format("BLOCKX %d", blockX);
			options ~= format("BLOCKY %d", blockY);
		}
		if (rotators)
			options ~= format("ROTATORS %d", rotators);
		if (holes)
			options ~= format("HOLES %d", holes);
		options ~= format("EXIT_X %d", exitX);
		options ~= format("EXIT_Y %d", exitY);
		options ~= userOptions;

		string output;
		foreach (option; options)
			output ~= "#define " ~ option ~ \n;
		output ~= "const char level[Y][X+1] = {\n";
		foreach (line; map)
			output ~= '\"' ~ line ~ "\",\n";
		output ~= "};\n";

		write(format("%d.h", level), output);
	}
}
