// To run this instead of a DDD search, add the following line to config.h:
// #define PROBLEM_RELATED Kwirk_vbm_export
// This must be run once for each level, from 0 to 29 consecutively.

int export_vbm()
{
	FILE *solution_in;
	{} {            } solution_in = fopen(STRINGIZE(LEVEL)".txt",         "rt");
	if (!solution_in) solution_in = fopen(STRINGIZE(LEVEL)"_bk2.txt",     "rt");
	if (!solution_in) solution_in = fopen(STRINGIZE(LEVEL)"_vbm_mod.txt", "rt");
	if (!solution_in) solution_in = fopen(STRINGIZE(LEVEL)"_vbm.txt",     "rt");
	if (!solution_in)
	{
		fprintf(stderr, "Error reading solution file\n");
		return -1;
	}
	FILE *vbm_out;
	if (LEVEL==0)
		vbm_out = fopen("Kwirk (UA) [optimized].vbm", "w+b");
	else
	{
		vbm_out = fopen("Kwirk (UA) [optimized].vbm", "r+b");
		fseek(vbm_out, 0, SEEK_END);
	}
	if (!vbm_out)
	{
		fclose(solution_in);
		fprintf(stderr, "Error writing vbm file\n");
		return -1;
	}

	static WORD action_to_vbm[] = {
	/* UP     */ 0x0040,
	/* RIGHT  */ 0x0010,
	/* DOWN   */ 0x0080,
	/* LEFT   */ 0x0020,
	/* SWITCH */ 0x0004,
	/* NONE   */ 0x0000,
	};

	WORD control;

	static BYTE vbm_header[0x100] = {
		0x56,0x42,0x4D,0x1A,0x01,0x00,0x00,0x00,0x62,0xEE,0xB2,0x45,0x00,0x00,0x00,0x00,
		0x0F,0x03,0x00,0x00,0x00,0x01,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,
		0x03,0x00,0x00,0x00,0x4B,0x57,0x49,0x52,0x4B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x0D,0x0A,0x6D,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,
	};

	if (LEVEL == 0)
	{
		strcpy((char*)(vbm_header+0x40), "Nitrodon, ZenicReverie, Alyosha, CyberShadow, Sand, Deadcode");
		fwrite(vbm_header, sizeof(vbm_header), 1, vbm_out);

		control = 0x0000; // no buttons
		for (int i=0; i<27; i++)
			fwrite(&control, sizeof(WORD), 1, vbm_out);

		control = 0x0008; // Start
		for (int i=0; i<10; i++)
			fwrite(&control, sizeof(WORD), 1, vbm_out);

		control = 0x0000; // no buttons
		for (int i=0; i<14; i++)
			fwrite(&control, sizeof(WORD), 1, vbm_out);

		// SELECT GAME -> GOING UP
		control = 0x0001; // A
		fwrite(&control, sizeof(WORD), 1, vbm_out);
		control = 0x0000; // no buttons
		for (int i=0; i<15; i++)
			fwrite(&control, sizeof(WORD), 1, vbm_out);
		// SELECT SKILL -> EASY
		control = 0x0008; // Start
		fwrite(&control, sizeof(WORD), 1, vbm_out);
		control = 0x0000; // no buttons
		for (int i=0; i<23; i++)
			fwrite(&control, sizeof(WORD), 1, vbm_out);
		// SELECT FLOOR -> FL# 1
		control = 0x0001; // A
		fwrite(&control, sizeof(WORD), 1, vbm_out);
		control = 0x0000; // no buttons
		for (int i=0; i<23; i++)
			fwrite(&control, sizeof(WORD), 1, vbm_out);
#ifdef BIRDS_EYE_VIEW
		// BIRD'S-EYE VIEW
		control = 0x0080; // DOWN
		fwrite(&control, sizeof(WORD), 1, vbm_out);
#endif
		control = 0x0000; // no buttons
		fwrite(&control, sizeof(WORD), 1, vbm_out);
		// SELECT DISPLAY -> DIAGONAL VIEW (or BIRD'S-EYE VIEW)
		control = 0x0001; // A
		fwrite(&control, sizeof(WORD), 1, vbm_out);
		control = 0x0000; // no buttons
		for (int i=0; i<24; i++)
			fwrite(&control, sizeof(WORD), 1, vbm_out);
	}

	// GOING UP? -> START
	control = 0x0001; // A
	fwrite(&control, sizeof(WORD), 1, vbm_out);
	control = 0x0000; // no buttons
	{
		int delay = 165;
		if (LEVEL==15)
			delay += 1;
		else
		if (LEVEL==5 || LEVEL==10)
			delay += 2;
		else
		if (LEVEL==18)
			delay += 3;
		else
#ifdef BIRDS_EYE_VIEW
		if (LEVEL==1 || LEVEL==8 || LEVEL==9 || LEVEL==22 || LEVEL==27)
			delay += 1;
		if (LEVEL==6)
			delay += 2;
#else
		if (LEVEL==14 || LEVEL==25 || LEVEL==26 || LEVEL==28)
			delay += 1;
#endif
		for (int i=0; i<delay; i++)
			fwrite(&control, sizeof(WORD), 1, vbm_out);
	}

	int frames = 0;
	int steps = -1;
	int switches = 0;
	int repeatCount = 0;
	Action repeatAction = NONE;
	try
	{
		State state = State::initial;
		Action lastAction = NONE;
		while (state.playersLeft())
		{
			Action action;

			char input[1024];
			fgets(input,sizeof(input),solution_in);
			size_t len=strlen(input);
			if (input[len-1]!='\n') error("Bad input!"); else input[--len]='\0';
			if (input[len-1]=='!' || input[len-1]=='~')
				input[--len]='\0';
			for (action=ACTION_FIRST; action<=NONE; action++)
				if (strcmp(input, actionNames[action])==0)
					goto valid_action;
			error(format("Unrecognized action '%s'", input));
		valid_action:
			steps++;
			if (steps==0 && action!=NONE)
				error("Expected 'None' action");
			if (steps>0 && action==NONE)
				error("Did not expect 'None' action");
			for (int i=0; i<Y; i++)
			{
				fgets(input,sizeof(input),solution_in);
				size_t len=strlen(input);
				if (input[len-1]!='\n') error("Bad input!"); else input[--len]='\0';
				if (input[0] != '#')
					error(format("Unexpected input: '%s'", input));
			}
			if (action==NONE)
				continue;
			if (action==SWITCH)
				switches++;

			int res = state.perform<true,false>(action);
			if (res <= 0)
				error("Bad action!");

			for (int i=0; i<repeatCount; i++)
				fwrite(&action_to_vbm[repeatAction], sizeof(WORD), 1, vbm_out);
#if 1
			if (action==SWITCH)
			{
				int resAdj = res;
				if (lastAction==SWITCH)
				{
					resAdj -= 2;
					for (int i=0; i<2; i++)
						fwrite(&action_to_vbm[NONE], sizeof(WORD), 1, vbm_out);
				}
				fwrite(&action_to_vbm[action], sizeof(WORD), 1, vbm_out);
				repeatCount = resAdj - 1;
			}
			else
			{
				fwrite(&action_to_vbm[NONE], sizeof(WORD), 1, vbm_out);
				fwrite(&action_to_vbm[action], sizeof(WORD), 1, vbm_out);
				repeatCount = res - 2;
			}
#else
			fwrite(&action_to_vbm[action], sizeof(WORD), 1, vbm_out);
			repeatCount = res - 1;
			repeatAction = action;
#endif

			lastAction = action;
			frames += res;
		}
		printf("%s", state.toString());
	}
	catch (const char* s)
	{
		puts(s);
	}
	printf("Total %d+%d steps, %d frames (%1.3f seconds)\n", steps-switches, switches, frames, frames/60.);

	if (LEVEL != 29)
	{
		for (int i=0; i<repeatCount; i++)
			fwrite(&action_to_vbm[repeatAction], sizeof(WORD), 1, vbm_out);

		control = 0x0000; // no buttons
		for (int i=0; i<182; i++)
			fwrite(&control, sizeof(WORD), 1, vbm_out);

		control = 0x0008; // Start
		fwrite(&control, sizeof(WORD), 1, vbm_out);

		if (LEVEL % 10 == 9)
		{
			control = 0x0000; // no buttons
			for (int i=0; i<254; i++)
				fwrite(&control, sizeof(WORD), 1, vbm_out);

			control = 0x0008; // Start
			fwrite(&control, sizeof(WORD), 1, vbm_out);

			control = 0x0000; // no buttons
			for (int i=0; i<31; i++)
				fwrite(&control, sizeof(WORD), 1, vbm_out);
		}
		else
		{
			control = 0x0000; // no buttons
			for (int i=0; i<45; i++)
				fwrite(&control, sizeof(WORD), 1, vbm_out);
#ifndef BIRDS_EYE_VIEW
			fwrite(&control, sizeof(WORD), 1, vbm_out);
#endif
		}
	}

	uint32_t frameCount = (ftell(vbm_out) - sizeof(vbm_header)) / sizeof(WORD) - 1;
	fseek(vbm_out, 0xC, SEEK_SET);
	fwrite(&frameCount, sizeof(frameCount), 1, vbm_out);

	fclose(solution_in);
	fclose(vbm_out);
	return 0;
}

int run_related(int argc, const char* argv[])
{
	return export_vbm();
	return 0;
}
