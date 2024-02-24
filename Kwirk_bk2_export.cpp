// To run this instead of a DDD search, add the following line to config.h:
// #define PROBLEM_RELATED Kwirk_bk2_export
// This must be run once for each level, from 0 to 29 consecutively.

//#define BIZHAWK_2_3_2 // requires Config -> Cores -> GB -> GBHawk
//#define BIZHAWK_2_5_2
#define BIZHAWK_2_9_1
#define GAMBATTE_CORE // currently only tested with BIZHAWK_2_9_1 with GBC_MODE disabled
//#define GBC_MODE // currently only tested with BIZHAWK_2_9_1

#if defined(BIZHAWK_2_3_2)
	#if defined(BIZHAWK_2_5_2) || defined(BIZHAWK_2_9_1)
		#error Must define exactly one
	#endif
	#define BIZHAWK_VERSION "2.3.2"
#elif defined(BIZHAWK_2_5_2)
	#if defined(BIZHAWK_2_3_2) || defined(BIZHAWK_2_9_1)
		#error Must define exactly one
	#endif
	#define BIZHAWK_VERSION "2.5.2"
#elif defined(BIZHAWK_2_9_1)
	#if defined(BIZHAWK_2_3_2) || defined(BIZHAWK_2_5_2)
		#error Must define exactly one
	#endif
	#ifdef GBC_MODE
		#define BIZHAWK_VERSION "2.9.1 (GBC)"
	#else
		#define BIZHAWK_VERSION "2.9.1"
	#endif
#else
	#error Must define exactly one
#endif

#ifdef GAMBATTE_CORE
	#define BIZHAWK_CORE " (Gambatte)"
#else
	#define BIZHAWK_CORE ""
#endif

int export_bk2()
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
	FILE *bk2_out;
	if (LEVEL==0)
		bk2_out = fopen("Kwirk (UA) [optimized] " BIZHAWK_VERSION BIZHAWK_CORE "/Input Log.txt", "wt");
	else
		bk2_out = fopen("Kwirk (UA) [optimized] " BIZHAWK_VERSION BIZHAWK_CORE "/Input Log.txt", "at");
	if (!bk2_out)
	{
		fclose(solution_in);
		fprintf(stderr, "Error writing bk2 file\n");
		return -1;
	}

	static const char *action_to_bk2[] = {
	/* UP     */ "|U........|\n",
	/* RIGHT  */ "|...R.....|\n",
	/* DOWN   */ "|.D.......|\n",
	/* LEFT   */ "|..L......|\n",
	/* SWITCH */ "|.....s...|\n",
	/* NONE   */ "|.........|\n",
	};
	const char *pressStart = "|....S....|\n", *pressStart_commented = "|....S....| ";
	const char *pressA     = "|.......A.|\n", *pressA_commented     = "|.......A.| ";

	if (LEVEL == 0)
	{
		fputs(
			"[Input]\n"
#ifdef GAMBATTE_CORE
			"LogKey:#Up|Down|Left|Right|Start|Select|B|A|Power|"
#else
			"LogKey:#P1 Up|P1 Down|P1 Left|P1 Right|P1 Start|P1 Select|P1 B|P1 A|P1 Power|"
#endif
			"\n", bk2_out);

		int delay = 378;
#ifdef BIZHAWK_2_3_2
		delay -= 12;
#endif
#ifdef GAMBATTE_CORE
		delay += 15;
#endif
#ifdef GBC_MODE
		delay -= 149;
#endif
		for (int i=0; i<delay; i++)
			fputs(action_to_bk2[NONE], bk2_out);

		fputs(pressStart, bk2_out);

		delay = 18;
#ifdef BIZHAWK_2_3_2
		delay -= 4;
#endif
#ifdef GAMBATTE_CORE
		delay += 2;
#endif
		for (int i=0; i<delay; i++)
			fputs(action_to_bk2[NONE], bk2_out);

		// SELECT GAME -> GOING UP
		fputs(pressA, bk2_out);
#ifdef BIZHAWK_2_3_2
		for (int i=0; i<15; i++)
#else
		for (int i=0; i<16; i++)
#endif
			fputs(action_to_bk2[NONE], bk2_out);
		// SELECT SKILL -> EASY
		fputs(pressStart, bk2_out);
#ifdef BIZHAWK_2_9_1
		for (int i=0; i<24; i++)
#else
		for (int i=0; i<23; i++)
#endif
			fputs(action_to_bk2[NONE], bk2_out);
		// SELECT FLOOR -> FL# 1
		fputs(pressA, bk2_out);
#ifdef BIZHAWK_2_3_2
		for (int i=0; i<22; i++)
#else
		for (int i=0; i<24; i++)
#endif
			fputs(action_to_bk2[NONE], bk2_out);
#ifdef BIRDS_EYE_VIEW
	#ifdef BIZHAWK_2_3_2
		fputs(action_to_bk2[NONE], bk2_out);
	#endif
		// BIRD'S-EYE VIEW
		fputs(action_to_bk2[DOWN], bk2_out);
#endif
		fputs(action_to_bk2[NONE], bk2_out);
		// SELECT DISPLAY -> DIAGONAL VIEW (or BIRD'S-EYE VIEW)
#ifdef BIZHAWK_2_3_2
		fputs(pressStart, bk2_out);
#else
		fputs(pressA, bk2_out);
#endif
		delay = 27;
#ifdef BIZHAWK_2_3_2
		delay -= 4;
#endif
#ifdef GAMBATTE_CORE
		delay += 1;
#endif
		for (int i=0; i<delay; i++)
			fputs(action_to_bk2[NONE], bk2_out);
	}

	// GOING UP? -> START
#ifdef BIZHAWK_2_3_2
	fputs(pressA_commented, bk2_out);
#else
	fputs(pressStart_commented, bk2_out);
#endif
	fprintf(bk2_out, "Level %u-%u\n", LEVEL/10+1, LEVEL%10+1);
	{
		int delay = 167;
#ifdef BIZHAWK_2_3_2
		delay -= 3;
#endif
#ifdef GAMBATTE_CORE
		delay += 1;
#endif
		if (LEVEL != 0)
			delay += 1;
		if (LEVEL==3 || LEVEL==15)
			delay += 1;
		else
		if (LEVEL==2 || LEVEL==6)
			delay += 2;
		else
		if (LEVEL==10 || LEVEL==18)
			delay += 3;
		else
		if (LEVEL==5)
			delay += 4;
#ifdef BIRDS_EYE_VIEW
		if (LEVEL==1 || LEVEL==8 || LEVEL==9 || LEVEL==10 || LEVEL==19 || LEVEL==20 || LEVEL==22 || LEVEL==27)
			delay += 1;
	#ifdef BIZHAWK_2_3_2
		if (LEVEL==6)
			delay += 1;
	#endif
	#ifdef BIZHAWK_2_5_2
		if (LEVEL==28)
			delay += 1;
	#endif
	#ifdef GAMBATTE_CORE
		if (LEVEL==19)
			delay += 1;
	#endif
#else
		delay += 1;
		if (LEVEL != 0)
			delay += 1;
		if (LEVEL==14 || LEVEL==25 || LEVEL==26 || LEVEL==28)
			delay += 1;
#endif
#ifdef BIZHAWK_2_3_2
		if (LEVEL==3)
			delay += 1;
		else
		if (LEVEL==20)
			delay += 2;
#endif
#ifdef BIZHAWK_2_9_1
		if (LEVEL==3)
			delay += 1;
		else
		if (LEVEL==22)
			delay += 2;
#endif
#ifdef GBC_MODE
		if (LEVEL==13)
			delay += 2;
#endif
		for (int i=0; i<delay; i++)
			fputs(action_to_bk2[NONE], bk2_out);
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
				fputs(action_to_bk2[repeatAction], bk2_out);
#if 1
			if (action==SWITCH)
			{
				int resAdj = res;
				if (lastAction==SWITCH)
				{
					resAdj -= 2;
					for (int i=0; i<2; i++)
						fputs(action_to_bk2[NONE], bk2_out);
				}
				fputs(action_to_bk2[action], bk2_out);
				repeatCount = resAdj - 1;
			}
			else
			{
				fputs(action_to_bk2[NONE], bk2_out);
				fputs(action_to_bk2[action], bk2_out);
				repeatCount = res - 2;
			}
#else
			fputs(action_to_bk2[action], bk2_out);
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
	printf("Total %d+%d steps, %d frames (%1.3f seconds)\n", steps-switches, switches, frames, frames/59.7275005696058);

	if (LEVEL == 29)
		fputs("[/Input]\n", bk2_out);
	else
	{
		for (int i=0; i<repeatCount; i++)
			fputs(action_to_bk2[repeatAction], bk2_out);

		int delay = 186;
#ifdef BIZHAWK_2_3_2
		delay -= 4;
#else
		if (LEVEL==3)
			delay += 2;
		else
		if (LEVEL==17 || LEVEL==20)
			delay += 1;
		else
		if (LEVEL==19)
			delay += 4;
		else
		if (LEVEL==9)
			delay += 5;
	#ifdef BIRDS_EYE_VIEW
		#ifndef BIZHAWK_2_9_1
		else
		if (LEVEL==28)
			delay += 1;
		#endif
	#else
		else
		if (LEVEL==6)
			delay += 1;
	#endif
	#ifdef BIZHAWK_2_9_1
		if (LEVEL==6 || LEVEL==19 || LEVEL==23)
			delay += 1;
		else
		if (LEVEL==16)
			delay += 3;
	#endif
#endif
#ifdef GAMBATTE_CORE
		delay += 2;
		if (LEVEL==6 || LEVEL==17 || LEVEL==20 || LEVEL==23)
			delay -= 1;
		else
		if (LEVEL==3)
			delay -= 2;
		else
		if (LEVEL==16)
			delay -= 3;
#endif
#ifdef GBC_MODE
		if (LEVEL==12 || LEVEL==26)
			delay += 1;
#endif
		for (int i=0; i<delay; i++)
			fputs(action_to_bk2[NONE], bk2_out);

		fputs(pressStart, bk2_out);

		if (LEVEL % 10 == 9)
		{
			delay = 254;
#ifdef BIZHAWK_2_3_2
			delay -= 1;
#endif
#ifndef BIRDS_EYE_VIEW
			delay += 1;
#endif
			for (int i=0; i<delay; i++)
				fputs(action_to_bk2[NONE], bk2_out);

			fputs(pressStart, bk2_out);

			delay = 33;
#ifdef BIZHAWK_2_3_2
			delay -= 2;
#endif
#ifdef GAMBATTE_CORE
			delay += 3;
#endif
			for (int i=0; i<delay; i++)
				fputs(action_to_bk2[NONE], bk2_out);
		}
		else
		{
			delay = 51;
#ifdef BIZHAWK_2_3_2
			delay -= 6;
#else
			if (LEVEL != 0)
				delay += 1;
#endif
#ifndef BIRDS_EYE_VIEW
			delay += 1;
#endif
#ifdef GAMBATTE_CORE
			delay += 3;
#endif
			for (int i=0; i<delay; i++)
				fputs(action_to_bk2[NONE], bk2_out);
		}
	}

	fclose(solution_in);
	fclose(bk2_out);
	return 0;
}

int run_related(int argc, const char* argv[])
{
	return export_bk2();
	return 0;
}
